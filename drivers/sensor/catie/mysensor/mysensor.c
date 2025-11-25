/*
 * Copyright (c) 2025, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT catie_mysensor

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "mysensor.h"

LOG_MODULE_REGISTER(MYSENSOR, CONFIG_SENSOR_LOG_LEVEL);

static int mysensor_attr_set(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	return -ENOTSUP;
}

static int mysensor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mysensor_data *data = dev->data;
	const struct mysensor_config *config = dev->config;

	return -ENOTSUP;
}

static int mysensor_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct mysensor_data *data = dev->data;

	// TODO: Update val with the sensor value
	val->val1 = 0;
	val->val2 = 0;

	return 0;
}

static int mysensor_init(const struct device *dev)
{
	const struct mysensor_config *config = dev->config;
	struct mysensor_data *data = dev->data;

	return 0;
}

static DEVICE_API(sensor, mysensor_driver_api) = {
	.attr_set = mysensor_attr_set,
	.sample_fetch = mysensor_sample_fetch,
	.channel_get = mysensor_channel_get,
};

#define MYSENSOR_INIT(n)  \
	static const struct mysensor_config mysensor_config_##n = {};  \
	static struct mysensor_data mysensor_data_##n;  \
	SENSOR_DEVICE_DT_INST_DEFINE(n, mysensor_init, NULL, &mysensor_data_##n, &mysensor_config_##n,  \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &mysensor_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MYSENSOR_INIT)
