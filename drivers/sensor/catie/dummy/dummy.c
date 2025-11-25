/*
 * Copyright (c) 2025, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT catie_dummy

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "dummy.h"

LOG_MODULE_REGISTER(DUMMY, CONFIG_SENSOR_LOG_LEVEL);

static int dummy_attr_set(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	return -ENOTSUP;
}

static int dummy_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct dummy_data *data = dev->data;
	const struct dummy_config *config = dev->config;

	return -ENOTSUP;
}

static int dummy_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct dummy_data *data = dev->data;

	// TODO: Update val with the sensor value
	val->val1 = 0;
	val->val2 = 0;
	
	return 0;
}

static int dummy_init(const struct device *dev)
{
	const struct dummy_config *config = dev->config;
	struct dummy_data *data = dev->data;

	return 0;
}

static DEVICE_API(sensor, dummy_driver_api) = {
	.attr_set = dummy_attr_set,
	.sample_fetch = dummy_sample_fetch,
	.channel_get = dummy_channel_get,
};

#define DUMMY_INIT(n)  \
	static const struct dummy_config dummy_config_##n = {};  \
	static struct dummy_data dummy_data_##n;  \
	SENSOR_DEVICE_DT_INST_DEFINE(n, dummy_init, NULL, &dummy_data_##n, &dummy_config_##n,  \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &dummy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DUMMY_INIT)
