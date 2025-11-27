/*
 * Copyright (c) 2025, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(mysensor));
	struct sensor_value temp_val, press_val, hum_val;
	int err;

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return 1;
	}

	k_sleep(K_MSEC(1000));

	while (1) {
		err = sensor_sample_fetch(dev);
		if (err < 0) {
			printk("Could not fetch sample (%d)\n", err);
				return 0;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_val)) {
			printk("Could not get sample\n");
			return 0;
		}
				
		if (sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press_val)) {
			printk("Could not get sample\n");
			return 0;
		}
			
		if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum_val)) {
			printk("Could not get sample\n");
			return 0;
		}

		printk("temperature value: %d\n", temp_val.val1);
		printk("pressure value: %d\n", press_val.val1);
		printk("humidity value: %d\n", hum_val.val1); 

		k_sleep(K_MSEC(2000));
	}
}
