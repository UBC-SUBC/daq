/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>


// constants:
#define SAMPLE_RATE 5 	// in milliseconds
#define MV_THRESHOLD 2300	// mV threshold for RPM calculation (PLACEHOLDER - MODIFY AS NEEDED)
#define RPM_CALC_WINDOW 60000	// Calculate RPM every 60 seconds (in milliseconds)

/* STEP 3.2 - Define a variable of type adc_dt_spec for each channel */
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

LOG_MODULE_REGISTER(Lesson6_Exercise1, LOG_LEVEL_DBG);

int main(void)
{
	int err;
	uint32_t count = 0;
	
	// RPM calculation variables
	int last_reading = 0;
	uint32_t threshold_crossings = 0;
	int64_t last_rpm_calc_time = k_uptime_get();
	int64_t current_time = 0;
	uint32_t rpm = 0;
	bool was_above_threshold = false;

	/* STEP 4.1 - Define a variable of type adc_sequence and a buffer of type uint16_t */
	int16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
		// Optional
		//.calibrate = true,
	};

	/* STEP 3.3 - validate that the ADC peripheral (SAADC) is ready */
	if (!adc_is_ready_dt(&adc_channel)) {
		LOG_ERR("ADC controller devivce %s not ready", adc_channel.dev->name);
		return 0;
	}
	/* STEP 3.4 - Setup the ADC channel */
	err = adc_channel_setup_dt(&adc_channel);
	if (err < 0) {
		LOG_ERR("Could not setup channel #%d (%d)", 0, err);
		return 0;
	}
	/* STEP 4.2 - Initialize the ADC sequence */
	err = adc_sequence_init_dt(&adc_channel, &sequence);
	if (err < 0) {
		LOG_ERR("Could not initalize sequnce");
		return 0;
	}

	while (1) {
		int val_mv;

		/* STEP 5 - Read a sample from the ADC */
		err = adc_read(adc_channel.dev, &sequence);
		if (err < 0) {
			LOG_ERR("Could not read (%d)", err);
			continue;
		}

		val_mv = (int)buf;
		// LOG_INF("ADC reading[%u]: %s, channel %d: Raw: %d", count++, adc_channel.dev->name,
		// 	adc_channel.channel_id, val_mv);

		/* STEP 6 - Convert raw value to mV*/
		err = adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
		/* conversion to mV may not be supported, skip if not */

		if (err < 0) {
			LOG_WRN(" (value in mV not available)\n");
		} else {
			// LOG_INF(" = %d mV", val_mv);
		}

		// ===== RPM CALCULATION LOGIC =====
		// Detect threshold crossing (transition from below to above threshold)
		bool is_above_threshold = (val_mv >= MV_THRESHOLD);
		if (is_above_threshold && !was_above_threshold) {
			threshold_crossings++;
			LOG_DBG("Threshold crossed! Total crossings: %u", threshold_crossings);
		}
		was_above_threshold = is_above_threshold;
		last_reading = val_mv;

		// Calculate RPM every RPM_CALC_WINDOW milliseconds
		current_time = k_uptime_get();
		if ((current_time - last_rpm_calc_time) >= RPM_CALC_WINDOW) {
			// RPM calculation formula:
			// - Each crossing represents a half-cycle (signal goes above threshold)
			// - 2 crossings = 1 complete cycle (revolution)
			// - RPM = (crossings / 2) * (60000 / time_window_ms)
			
			int64_t elapsed_time = current_time - last_rpm_calc_time;
			if (elapsed_time > 0) {
				rpm = (threshold_crossings * 60000)/(elapsed_time);
				LOG_INF("===== RPM CALCULATION =====");
				LOG_INF("Threshold crossings in %lld ms: %u", elapsed_time, threshold_crossings);
				LOG_INF("Calculated RPM: %u", rpm);
				LOG_INF("==========================");
			}

			// Reset counters for next window
			threshold_crossings = 0;
			last_rpm_calc_time = current_time;
		}

		k_sleep(K_MSEC(SAMPLE_RATE));
	}
	return 0;
}
