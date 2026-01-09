/*
 * Copyright (c) 2025, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT catie_mysensor

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include "mysensor.h"

LOG_MODULE_REGISTER(MYSENSOR, CONFIG_SENSOR_LOG_LEVEL);

#define MYSENSOR_START_UP_TIME_MS 2

static inline int mysensor_bus_check(const struct device *dev)
{
	const struct mysensor_config *cfg = dev->config;

	return device_is_ready(cfg->i2c.bus) ? 0 : -ENODEV;
}

static inline int mysensor_reg_read(const struct device *dev,
				  uint8_t start, uint8_t *buf, int size)
{
	const struct mysensor_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->i2c, start, buf, size);
}

static inline int mysensor_reg_write(const struct device *dev, uint8_t reg,
				   uint8_t val)
{
	const struct mysensor_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, reg, val);
}

static int mysensor_wait_until_ready(const struct device *dev, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	uint8_t status;
	int ret;

	/* Wait for relevant flags to clear */
	while (1) {
		ret = mysensor_reg_read(dev, MYSENSOR_REG_STATUS, &status, 1);
		if (ret < 0) {
			return ret;
		}
		if (!(status & (MYSENSOR_STATUS_MEASURING | MYSENSOR_STATUS_IM_UPDATE))) {
			break;
		}
		/* Check if waiting has timed out */
		if (sys_timepoint_expired(end)) {
			return -EAGAIN;
		}
		k_sleep(K_MSEC(3));
	}

	return 0;
}

static int mysensor_attr_set(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	return -ENOTSUP;
}

static int mysensor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mysensor_data *data = dev->data;
	const struct mysensor_config *config = dev->config;

	uint8_t buf[8];
	int size = 8;
	int err;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	err = mysensor_wait_until_ready(dev, K_MSEC(150));
	if (err < 0) {
		return err;
	}


	err = mysensor_reg_read(dev, MYSENSOR_REG_PRESS_MSB, buf, size);
	if (err < 0) {
		return err;
	}

	// get raw data
	data->raw_pressure = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
	data->raw_temp = (buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);
	data->raw_humidity = (buf[6] << 8) | buf[7];

	return 0;
}

static int mysensor_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct mysensor_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		// convert to Degree C
		val->val1 = (int32_t)((float)data->raw_temp * 125.0f / 1048576.0f) - 40.0f;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_PRESS:
		// convert to hPa
		val->val1 = (int32_t)(300.0f + ((float)data->raw_pressure * 800.0f / 1048576.0f));
		val->val2 = 0;
		break;
	case SENSOR_CHAN_HUMIDITY:
		// convert to %
		val->val1 = data->raw_humidity * 100 / 65535;
		val->val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mysensor_init(const struct device *dev)
{
	const struct mysensor_config *config = dev->config;
	struct mysensor_data *data = dev->data;
	int err;

	err = mysensor_bus_check(dev);
	if (err < 0) {
		printk("bus check failed: %d\n", err);
		return err;
	}

	k_msleep(MYSENSOR_START_UP_TIME_MS);

	err = mysensor_reg_read(dev, MYSENSOR_REG_ID, &data->chip_id, 1);
	if (err < 0) {
		printk("ID read failed: %d\n", err);
		return err;
	}

	if (data->chip_id == MYSENSOR_CHIP_ID) {
		printk("ID OK\n");
	} else {
		printk("bad chip id 0x%x", data->chip_id);
		return -ENOTSUP;
	}

	/* reset the sensor. This will put the sensor is sleep mode */
	err = mysensor_reg_write(dev, MYSENSOR_REG_RESET, MYSENSOR_CMD_SOFT_RESET);
	if (err < 0) {
		LOG_DBG("Soft-reset failed: %d", err);
	}

	/* The only mention of a soft reset duration is 2ms from the self test timeouts */
	k_sleep(K_MSEC(2));


	err = mysensor_reg_write(dev, MYSENSOR_REG_CTRL_HUM,
					MYSENSOR_HUMIDITY_OVER);
	if (err < 0) {
		LOG_DBG("CTRL_HUM write failed: %d", err);
		return err;
	}
	
	
	/* Writes to "config" register may be ignored in normal
	 * mode, but never in sleep mode [datasheet 5.4.6].
	 *
	 * So perform "config" write before "ctrl_meas", as "ctrl_meas"
	 * could cause the sensor to transition from sleep to normal mode.
	 */
	printk("0x%X\n", MYSENSOR_CONFIG_VAL);
	 err = mysensor_reg_write(dev, MYSENSOR_REG_CONFIG, MYSENSOR_CONFIG_VAL);
	if (err < 0) {
		LOG_DBG("CONFIG write failed: %d", err);
		return err;
	}

	printk("0x%X\n", MYSENSOR_CTRL_MEAS_VAL);
	err = mysensor_reg_write(dev, MYSENSOR_REG_CTRL_MEAS, MYSENSOR_CTRL_MEAS_VAL);
	if (err < 0) {
		LOG_DBG("CTRL_MEAS write failed: %d", err);
		return err;
	}

	/* Wait for the sensor to be ready */
	k_sleep(K_MSEC(1));

	printk("\"%s\" OK\n", dev->name);
	return 0;
}

static DEVICE_API(sensor, mysensor_driver_api) = {
	.attr_set = mysensor_attr_set,
	.sample_fetch = mysensor_sample_fetch,
	.channel_get = mysensor_channel_get,
};

#define MYSENSOR_INIT(n)  \
	static const struct mysensor_config mysensor_config_##n = {  \
		.i2c = I2C_DT_SPEC_INST_GET(n),  \
	};  \
	static struct mysensor_data mysensor_data_##n;  \
	SENSOR_DEVICE_DT_INST_DEFINE(n, mysensor_init, NULL, &mysensor_data_##n, &mysensor_config_##n,  \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &mysensor_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MYSENSOR_INIT)