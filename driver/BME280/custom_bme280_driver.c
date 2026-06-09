#include "custom_bme280_driver.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

static int8_t custom_bme280_spi_read(uint8_t reg_addr,
				     uint8_t *reg_data,
				     uint32_t len,
				     void *intf_ptr)
{
	const struct spi_dt_spec *spi = intf_ptr;
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

	if (spi == NULL) {
		return BME280_E_COMM_FAIL;
	}

	return spi_transceive_dt(spi, &tx, &rx) == 0 ?
	       BME280_INTF_RET_SUCCESS : BME280_E_COMM_FAIL;
}

static int8_t custom_bme280_spi_write(uint8_t reg_addr,
				      const uint8_t *reg_data,
				      uint32_t len,
				      void *intf_ptr)
{
	const struct spi_dt_spec *spi = intf_ptr;

	if (spi == NULL) {
		return BME280_E_COMM_FAIL;
	}

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

	return spi_write_dt(spi, &tx) == 0 ? BME280_INTF_RET_SUCCESS : BME280_E_COMM_FAIL;
}

static void custom_bme280_delay_us(uint32_t period, void *intf_ptr)
{
	ARG_UNUSED(intf_ptr);

	k_usleep(period);
}

int8_t custom_bme280_init(struct bme280_dev *dev, const struct spi_dt_spec *spi)
{
	if ((dev == NULL) || (spi == NULL)) {
		return BME280_E_NULL_PTR;
	}

	if (!spi_is_ready_dt(spi)) {
		return BME280_E_COMM_FAIL;
	}

	dev->intf = BME280_SPI_INTF;
	dev->intf_ptr = (void *)spi;
	dev->read = custom_bme280_spi_read;
	dev->write = custom_bme280_spi_write;
	dev->delay_us = custom_bme280_delay_us;

	return bme280_init(dev);
}

int8_t custom_bme280_read_sensor_data(struct bme280_dev *dev, struct bme280_data *data)
{
	return bme280_get_sensor_data(BME280_ALL, data, dev);
}
