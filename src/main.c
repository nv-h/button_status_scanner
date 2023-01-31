/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <device.h>
#include <devicetree.h>
#include <kernel.h>
#include <zephyr.h>

#include <bluetooth/hci.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/scan.h>

#define MODULE main

#include <logging/log.h>
LOG_MODULE_REGISTER(scanner);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

static void app_work_handler(struct k_work *work);
static void app_timer_handler(struct k_timer *dummy);

K_TIMER_DEFINE(app_timer, app_timer_handler, NULL);
K_WORK_DEFINE(app_work, app_work_handler);

struct bt_le_scan_param scan_param = {
    .type       = BT_HCI_LE_SCAN_PASSIVE,
    .options    = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
    .interval   = BT_GAP_SCAN_FAST_INTERVAL,
    .window     = BT_GAP_SCAN_FAST_WINDOW,
};

typedef struct {
    uint8_t name[BT_ADDR_LE_STR_LEN];
    uint8_t manufacturer_data[BT_ADDR_LE_STR_LEN];
} app_parsed_data_t;

static void app_work_handler(struct k_work *work)
{
    LOG_INF("app_work(): ");
}

static void app_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&app_work);
}

static bool data_callback(struct bt_data *data, void *user_data)
{
    app_parsed_data_t *buf = user_data;
    uint8_t len;

    switch (data->type) {
    case BT_DATA_NAME_SHORTENED:
    case BT_DATA_NAME_COMPLETE:
        len = MIN(data->data_len, BT_ADDR_LE_STR_LEN);
        memcpy(buf->name, data->data, len);
        return false;
    case BT_DATA_MANUFACTURER_DATA:
        len = MIN(data->data_len, BT_ADDR_LE_STR_LEN);
        memcpy(buf->manufacturer_data, data->data, len);
        return false;
    default:
        return true;
    }
}

static void scan_callback(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type, struct net_buf_simple *buf)
{
    app_parsed_data_t app_parsed_data = {0};

    bt_data_parse(buf, data_callback, &app_parsed_data);
    LOG_INF("name: %s, status: %d", app_parsed_data.name, app_parsed_data.manufacturer_data[0]);
}

void main(void)
{
    LOG_MODULE_DECLARE(scanner);
    int ret;

    ret = bt_enable(NULL);
    if (ret) {
        LOG_ERR("Failed to bt_enable() [%d]", ret);
        return;
    }

    bt_le_scan_start(&scan_param, scan_callback);

    /* start periodic timer that expires once every `SLEEP_TIME_MS` milliseconds */
    k_timer_start(&app_timer, K_SECONDS(1), K_MSEC(SLEEP_TIME_MS));

    // do nothing
}
