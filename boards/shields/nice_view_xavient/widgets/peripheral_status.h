/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

#include "util.h"

#ifndef PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_MS
#define PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_MS 2000
#endif

#ifndef PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_STEP_MS
#define PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_STEP_MS 500
#endif

#ifndef PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_MIN_MS
#define PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_MIN_MS 500
#endif

#ifndef PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_MAX_MS
#define PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_MAX_MS 10000
#endif

struct zmk_slideshow_speed_increase {
    struct zmk_event_header header;
};

struct zmk_slideshow_speed_decrease {
    struct zmk_event_header header;
};

struct zmk_slideshow_speed_reset {
    struct zmk_event_header header;
};

ZMK_EVENT_DECLARE(zmk_slideshow_speed_increase);
ZMK_EVENT_DECLARE(zmk_slideshow_speed_decrease);
ZMK_EVENT_DECLARE(zmk_slideshow_speed_reset);

struct zmk_widget_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *art;
    lv_timer_t *slideshow_timer;
    lv_color_t cbuf[CANVAS_SIZE * CANVAS_SIZE];
    struct status_state state;
    uint8_t slide_index;
    bool align_left;
    uint32_t slideshow_interval_ms;
};

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget);