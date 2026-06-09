#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <errno.h>
#include <string.h>
#include <ff.h>

#include "sd_card.h"
#include "custom_bme280_driver.h"
#include "main.h"

#define SD_SPI_NODE DT_NODELABEL(spi1)
#define BME280_SPI_NODE DT_NODELABEL(bme280)

// ========== Configuration ==========
#define DEBUG 0
//====================================

static const struct device *const sd_spi = DEVICE_DT_GET(SD_SPI_NODE);
static const struct gpio_dt_spec sd_cs = GPIO_DT_SPEC_GET_BY_IDX(SD_SPI_NODE, cs_gpios, 0);
static struct bme280_dev bme280;

#if DT_NODE_EXISTS(BME280_SPI_NODE)
static const struct spi_dt_spec bme280_spi = SPI_DT_SPEC_GET(BME280_SPI_NODE, SPI_WORD_SET(8));
#endif

dev_status all_dev_status = 
{
	.BME280 = pending,
	.xxx_sensor = pending,
	.six_seven_sensor = pending,
	.hall_effect_sensor = pending,
	.SD_card = pending
};

/*
* static functions declarations
*/
static int onboard_peripherals_init(void);
static int external_peripherals_init(void);
static int bme280_sensor_init(void);

int main(void)
{
	// static const char data[] = "SD card write test\r\n";
	int ret;
	int close_ret;
	
	//init the data stuct and status struct:
	sd_data_struct data = {0};
	struct bme280_data bme280_data = {0};

	printk("g");
	ret = onboard_peripherals_init();
	if (ret != 0) {
		return ret;
	}

	printk("ggg");
	ret = external_peripherals_init();
	if (ret != 0) {
		return ret;
	}

	printk("gggg");
	ret = custom_bme280_read_sensor_data(&bme280, &bme280_data);
	if (ret != BME280_OK) {
		all_dev_status.BME280 = pending;
		// printk("Failed to read BME280 sensor data: %d\n", ret);
		return ret;
	}

	data.temp = (uint8_t)bme280_data.temperature;
	data.pressure = (uint8_t)bme280_data.pressure;
	data.rpm = 3;
	data.xxx = 4;
	data.yyy = 5;
	data.ccc = 6;

	ret = add_sensor_data_to_file(&data);
	if (ret != 0) {
		all_dev_status.SD_card = pending;
	} else {
		all_dev_status.SD_card = complete;
	}

	/*
	*close the SD card at very end of the firmware
	*/
	close_ret = sd_card_cleanup();
	if (close_ret != 0) {
		ret = close_ret;
	}

	return ret;
}


/*
* static functions definitions
*/

//onboard peripherals initialization gose here, for example the SD card SPI interface
static int sd_card_spi_init(void)
{
	if (!device_is_ready(sd_spi)) {
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&sd_cs)) {
		return -ENODEV;
	}

	return 0;
}

static int onboard_peripherals_init(void){
	int ret = 0;
	//SPI1 for sd card, cs is P1.12
	ret = sd_card_spi_init();
	if (ret != 0) {
		return ret;
	}

	return ret;
}
//external devices/sensors initialization gose here, for example the BME280 sensor and SD card
static int external_peripherals_init(void)
{
	int ret = 0;

	ret = bme280_sensor_init();
	if (ret != 0) {
		return ret;
	}

	ret = sd_card_init();
	if (ret != 0) {
		return ret;
	}

	return ret;
}

static int bme280_sensor_init(void)
{
#if DT_NODE_EXISTS(BME280_SPI_NODE)
	int ret = custom_bme280_init(&bme280, &bme280_spi);

	printk("stuck here?");
	if (ret != BME280_OK) {
		all_dev_status.BME280 = pending;
		printk("Failed to initialize BME280 sensor: %d\n", ret);
		return ret;
	}

	all_dev_status.BME280 = complete;
	return 0;
#else
	return -ENODEV;
#endif
}
