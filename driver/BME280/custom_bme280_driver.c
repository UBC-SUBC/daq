#include "custom_bme280_driver.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define BME280_SPI_NODE DT_NODELABEL(spi3)
#define CUSTOM_BME280_FILTER BME280_FILTER_COEFF_2
#define CUSTOM_BME280_OVERSAMPLING BME280_OVERSAMPLING_1X
#define CUSTOM_BME280_STANDBY_TIME BME280_STANDBY_TIME_0_5_MS

static uint32_t bme280_measurement_delay_us;

static const struct spi_dt_spec bme280_spi = {
	.bus = DEVICE_DT_GET(BME280_SPI_NODE),
	.config = {
		.frequency = 1000000,
		.operation = SPI_WORD_SET(8),
		.slave = 0,
		.cs = {
			.gpio = GPIO_DT_SPEC_GET_BY_IDX(BME280_SPI_NODE, cs_gpios, 0),
			.delay = 0,
			.cs_is_gpio = true,
		},
	},
};

static int8_t custom_bme280_spi_read(uint8_t reg_addr,
				     uint8_t *reg_data,
				     uint32_t len,
				     void *intf_ptr)
{
	ARG_UNUSED(intf_ptr);

	struct spi_config config;
	int ret;
	const struct spi_buf tx_buf = {
		.buf = &reg_addr,
		.len = 1,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct spi_buf rx_bufs[2] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = reg_data,
			.len = len,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = 2,
	};

	config = bme280_spi.config;
	config.cs.gpio.port = NULL;
	config.cs.cs_is_gpio = false;

	if (gpio_pin_set_raw(bme280_spi.config.cs.gpio.port, bme280_spi.config.cs.gpio.pin, 0) != 0) {
		return BME280_E_COMM_FAIL;
	}

	ret = spi_transceive(bme280_spi.bus, &config, &tx, &rx);
	(void)gpio_pin_set_raw(bme280_spi.config.cs.gpio.port, bme280_spi.config.cs.gpio.pin, 1);

	return ret == 0 ? BME280_INTF_RET_SUCCESS : BME280_E_COMM_FAIL;
}

static int8_t custom_bme280_spi_write(uint8_t reg_addr,
				      const uint8_t *reg_data,
				      uint32_t len,
				      void *intf_ptr)
{
	ARG_UNUSED(intf_ptr);

	struct spi_config config;
	int ret;
	const struct spi_buf tx_bufs[2] = {
		{
			.buf = &reg_addr,
			.len = 1,
		},
		{
			.buf = (void *)reg_data,
			.len = len,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = 2,
	};

	config = bme280_spi.config;
	config.cs.gpio.port = NULL;
	config.cs.cs_is_gpio = false;

	if (gpio_pin_set_raw(bme280_spi.config.cs.gpio.port, bme280_spi.config.cs.gpio.pin, 0) != 0) {
		return BME280_E_COMM_FAIL;
	}

	ret = spi_write(bme280_spi.bus, &config, &tx);
	(void)gpio_pin_set_raw(bme280_spi.config.cs.gpio.port, bme280_spi.config.cs.gpio.pin, 1);

	return ret == 0 ? BME280_INTF_RET_SUCCESS : BME280_E_COMM_FAIL;
}

static void custom_bme280_delay_us(uint32_t period, void *intf_ptr)
{
	ARG_UNUSED(intf_ptr);

	k_usleep(period);
}

int8_t custom_bme280_init(struct bme280_dev *dev)
{
	struct bme280_settings settings;
	int8_t rslt;

	if (dev == NULL) {
		return BME280_E_NULL_PTR;
	}

	if (!spi_is_ready_dt(&bme280_spi)) {
		return BME280_E_COMM_FAIL;
	}

	if (gpio_pin_configure(bme280_spi.config.cs.gpio.port,
			       bme280_spi.config.cs.gpio.pin,
			       GPIO_OUTPUT_HIGH) != 0) {
		return BME280_E_COMM_FAIL;
	}

	dev->intf = BME280_SPI_INTF;
	dev->intf_ptr = NULL;
	dev->read = custom_bme280_spi_read;
	dev->write = custom_bme280_spi_write;
	dev->delay_us = custom_bme280_delay_us;

	rslt = bme280_init(dev);
	if (rslt != BME280_OK) {
		return rslt;
	}

	rslt = bme280_get_sensor_settings(&settings, dev);
	if (rslt != BME280_OK) {
		return rslt;
	}

	settings.filter = CUSTOM_BME280_FILTER;
	settings.osr_h = CUSTOM_BME280_OVERSAMPLING;
	settings.osr_p = CUSTOM_BME280_OVERSAMPLING;
	settings.osr_t = CUSTOM_BME280_OVERSAMPLING;
	settings.standby_time = CUSTOM_BME280_STANDBY_TIME;

	rslt = bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS, &settings, dev);
	if (rslt != BME280_OK) {
		return rslt;
	}

	rslt = bme280_cal_meas_delay(&bme280_measurement_delay_us, &settings);
	if (rslt != BME280_OK) {
		return rslt;
	}

	return bme280_set_sensor_mode(BME280_POWERMODE_SLEEP, dev);
}

int8_t custom_bme280_read_sensor_data(struct bme280_dev *dev, struct bme280_data *data)
{
	int8_t rslt;

	if ((dev == NULL) || (data == NULL)) {
		return BME280_E_NULL_PTR;
	}

	rslt = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, dev);
	if (rslt != BME280_OK) {
		return rslt;
	}

	dev->delay_us(bme280_measurement_delay_us, dev->intf_ptr);

	/* Bosch API reads raw registers and returns compensated temperature, pressure, and humidity. */
	return bme280_get_sensor_data(BME280_ALL, data, dev);
}
