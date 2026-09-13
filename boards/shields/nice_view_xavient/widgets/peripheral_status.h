/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include "util.h"

#define PERIPHERAL_STATUS_SLIDESHOW_INTERVAL_MS CONFIG_NICE_VIEW_WIDGET_SLIDESHOW_INTERVAL_MS


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
