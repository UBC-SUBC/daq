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

// ========== Configuration ==========
#define DEBUG 1
//====================================

static const struct device *const sd_spi = DEVICE_DT_GET(SD_SPI_NODE);
static const struct gpio_dt_spec sd_cs = GPIO_DT_SPEC_GET_BY_IDX(SD_SPI_NODE, cs_gpios, 0);
static struct bme280_dev bme280;

dev_status all_dev_status = 
{
	.BME280 = un_init,
	.Barometer = un_init,
	.IMU = un_init,
	.hall_effect_sensor = un_init,
	.SD_card = un_init
};

/*
* static functions declarations
*/
static int onboard_peripherals_init(void);
static int external_peripherals_init(void);

int main(void)
{
	// static const char data[] = "SD card write test\r\n";
	int ret;
	int close_ret;
	
	//init the data stuct and status struct:
	sd_data_struct data = {0};
	struct bme280_data bme280_data = {0};

	ret = onboard_peripherals_init();
	if (ret != 0) {
		#if DEBUG
		printk("onboard peripheral init failed");
		#endif
		return ret;
	}

	ret = external_peripherals_init();
	if (ret != 0) {
		#if DEBUG
		printk("external peripheral init failed");
		#endif
		return ret;
	}

	/*
	if code got here all peripheral should be initialized
	*/
	all_dev_status.BME280 = pending;
	all_dev_status.Barometer = pending;
	all_dev_status.IMU = pending;
	all_dev_status.hall_effect_sensor = pending;
	all_dev_status.SD_card = pending;

	// geting sd card read ready
	int last_sd_write = k_uptime_get();
	int test_counter = 15;
	/*
	measure loop begin
	*/
// return 0;
// sd_card_cleanup();
	while (test_counter){
		/*
		read data and update status for bme280
		*/
		
		if (all_dev_status.BME280 == pending) {
			all_dev_status.BME280 = pending_calculation;
			ret = custom_bme280_read_sensor_data(&bme280, &bme280_data);
			if (ret != BME280_OK) {
				#if DEBUG
				printk("BME280 read failed: %d\n", ret);
				#endif
				return ret;
			}
			else
			{
				#if DEBUG
				printk("BME280 read success!\n");
				printk("Temperature: %.d C\n", (int)bme280_data.temperature);
				printk("Humidity: %.d %%\n", (int)bme280_data.humidity);
				#endif
				all_dev_status.BME280 = complete;
			}
		}


		if (k_uptime_get() - last_sd_write >= 1000) {
			test_counter --;
			last_sd_write = k_uptime_get();
			
			//TODO add status check

			data.temp = (uint8_t) bme280_data.temperature;
			data.humidity = (uint16_t) bme280_data.humidity;
			data.pressure = 0xff;
			data.rpm = 0xff;
			data.x = 0xff;
			data.y = 0xff;
			data.z = 0xff;

			ret = add_sensor_data_to_file(&data);
			if (ret != 0) {
				all_dev_status.SD_card = failed;
				#if DEBUG
					printk("sd card write failed");
				#endif
			} else {
				all_dev_status.SD_card = complete;

				//reset all other sensor status
				all_dev_status.BME280 = pending;
				#if DEBUG
					printk("sd card write success");
				#endif
			}
		}
	}

	/*
	*close the SD card at very end of the firmware
	*/
	close_ret = sd_card_cleanup();
	if (close_ret != 0) {
		ret = close_ret;
	}

	#if DEBUG
	printk("program ended");
	#endif

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

	//SPI3 init for bme280 is within custom_bme280_init()

	return ret;
}
//external devices/sensors initialization gose here, for example the BME280 sensor and SD card
static int external_peripherals_init(void)
{
	int ret = 0;

	/*
	SD Card Init
	*/
	ret = sd_card_init();
	if (ret != 0) {
		#if DEBUG
		printk("SD card init failed: %d\n", ret);
		#endif
		return ret;
	}
	else{
		#if DEBUG
		printk("SD card init! \n");
		#endif
		all_dev_status.SD_card = init;
	}

	/*
	* BME280 init
	*/
	ret = custom_bme280_init(&bme280);
	if (ret != BME280_OK) {
		#if DEBUG
		printk("BME280 init failed: %d\n", ret);
		#endif
		return ret;
	}
	else
	{
		#if DEBUG
		printk("BME280 init!\n");
		#endif
		all_dev_status.BME280 = init;
	}

	#if DEBUG
	printk("External peripherals initialized successfully\n");
	#endif

	return ret;
}