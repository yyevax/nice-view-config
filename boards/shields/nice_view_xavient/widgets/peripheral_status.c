/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

LV_IMAGE_DECLARE(image1);
LV_IMAGE_DECLARE(image2);
LV_IMAGE_DECLARE(image3);
LV_IMAGE_DECLARE(image4);
LV_IMAGE_DECLARE(image5);
LV_IMAGE_DECLARE(image6);
LV_IMAGE_DECLARE(image1);
LV_IMAGE_DECLARE(image2);

const lv_image_dsc_t *left_anim_imgs[] = {
    &image1,
    &image2,
    &image3,
    &image4,
    &image5,
    &image6,
};

const lv_image_dsc_t *right_anim_imgs[] = {
    &image1,
    &image2,
};




#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>

#include "peripheral_status.h"

// Custom events for slideshow control
ZMK_EVENT_DECLARE(zmk_slideshow_speed_increase);
ZMK_EVENT_DECLARE(zmk_slideshow_speed_decrease);

#if IS_ENABLED(CONFIG_SHIELD_XAVIEN_LEFT)
LV_IMAGE_DECLARE(left);
#define PERIPHERAL_IMAGE left
#else
LV_IMAGE_DECLARE(right);
#define PERIPHERAL_IMAGE right
#endif

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void slideshow_speed_increase_cb(const zmk_event_t *eh) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->slideshow_interval_ms = MIN(widget->slideshow_interval_ms + PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_STEP_MS,
                                           PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_MAX_MS);
        lv_timer_set_period(widget->slideshow_timer, widget->slideshow_interval_ms);
    }
}

static void slideshow_speed_decrease_cb(const zmk_event_t *eh) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->slideshow_interval_ms = MAX(widget->slideshow_interval_ms - PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_STEP_MS,
                                           PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_MIN_MS);
        lv_timer_set_period(widget->slideshow_timer, widget->slideshow_interval_ms);
    }
}

ZMK_LISTENER(widget_slideshow_speed_increase, slideshow_speed_increase_cb);
ZMK_LISTENER(widget_slideshow_speed_decrease, slideshow_speed_decrease_cb);

ZMK_SUBSCRIPTION(widget_slideshow_speed_increase, zmk_slideshow_speed_increase);
ZMK_SUBSCRIPTION(widget_slideshow_speed_decrease, zmk_slideshow_speed_decrease);

static void peripheral_status_slideshow_cb(lv_timer_t *timer) {
    struct zmk_widget_status *widget = (struct zmk_widget_status *)lv_timer_get_user_data(timer);
    const lv_image_dsc_t **current_imgs = widget->align_left ? left_anim_imgs : right_anim_imgs;
    const size_t slide_count = widget->align_left ?
        (sizeof(left_anim_imgs) / sizeof(left_anim_imgs[0])) :
        (sizeof(right_anim_imgs) / sizeof(right_anim_imgs[0]));

    if (slide_count == 0) {
        return;
    }

    widget->slide_index = (widget->slide_index + 1) % slide_count;
    widget->align_left = !widget->align_left;
    lv_image_set_src(widget->art, current_imgs[widget->slide_index]);
    lv_obj_align(widget->art,
                 widget->align_left ? LV_ALIGN_TOP_LEFT : LV_ALIGN_TOP_RIGHT,
                 0, 0);
}

struct peripheral_status_state {
    bool connected;
};

static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);

    // Fill background
    canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // Draw battery
    pct_battery(canvas, state);

    // Draw output status
    canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc,
                     state->connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);

    // Rotate canvas
    rotate_canvas(canvas);
}

static void set_battery_status(struct zmk_widget_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

    widget->state.battery = state.level;

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    return (struct battery_status_state){
        .level = zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

static struct peripheral_status_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void set_connection_status(struct zmk_widget_status *widget,
                                  struct peripheral_status_state state) {
    widget->state.connected = state.connected;

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void output_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_connection_status(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);

    lv_obj_t *art = lv_image_create(widget->obj);
    const size_t slide_count = sizeof(left_anim_imgs) / sizeof(left_anim_imgs[0]) + sizeof(right_anim_imgs) / sizeof(right_anim_imgs[0]);
    uint32_t rand = sys_rand32_get();
    widget->slide_index = slide_count ? rand % slide_count : 0;
    widget->align_left = rand & 0x1;
    widget->art = art;
    widget->slideshow_interval_ms = PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_MS;

    const lv_img_dsc_t **initial_imgs = widget->align_left ? left_anim_imgs : right_anim_imgs;
    const size_t initial_count = widget->align_left ? (sizeof(left_anim_imgs) / sizeof(left_anim_imgs[0])) : (sizeof(right_anim_imgs) / sizeof(right_anim_imgs[0]));
    if (initial_count > 0) {
        lv_image_set_src(art, initial_imgs[widget->slide_index % initial_count]);
    } else {
        lv_image_set_src(art, &PERIPHERAL_IMAGE); // fallback
    }
    lv_obj_align(art, widget->align_left ? LV_ALIGN_TOP_LEFT : LV_ALIGN_TOP_RIGHT, 0, 0);
    widget->slideshow_timer = lv_timer_create(peripheral_status_slideshow_cb,
                                              widget->slideshow_interval_ms,
                                              widget);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_peripheral_status_init();

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
