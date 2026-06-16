#include "bar30.h"

#include <errno.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define BAR30_I2C_NODE DT_NODELABEL(i2c0)

#define BAR30_ADDR 0x76
#define BAR30_CMD_RESET 0x1E
#define BAR30_CMD_ADC_READ 0x00
#define BAR30_CMD_PROM_READ 0xA0
#define BAR30_CMD_CONVERT_D1_OSR4096 0x48
#define BAR30_CMD_CONVERT_D2_OSR4096 0x58
#define BAR30_RESET_DELAY_MS 10
#define BAR30_CONVERSION_DELAY_MS 10
#define BAR30_PROBE_RETRIES 5
#define BAR30_PROBE_RETRY_DELAY_MS 10
#define BAR30_DEBUG 0

static const struct device *const bar30_i2c = DEVICE_DT_GET(BAR30_I2C_NODE);
static uint16_t bar30_prom[8];
static bool bar30_ready;

static void bar30_log_calibration(void)
{
#if BAR30_DEBUG
	printk("BAR30 C1 SENS_T1: %u (0x%04x)\n",
	       (unsigned int)bar30_prom[1], (unsigned int)bar30_prom[1]);
	printk("BAR30 C2 OFF_T1: %u (0x%04x)\n",
	       (unsigned int)bar30_prom[2], (unsigned int)bar30_prom[2]);
	printk("BAR30 C3 TCS: %u (0x%04x)\n",
	       (unsigned int)bar30_prom[3], (unsigned int)bar30_prom[3]);
	printk("BAR30 C4 TCO: %u (0x%04x)\n",
	       (unsigned int)bar30_prom[4], (unsigned int)bar30_prom[4]);
	printk("BAR30 C5 T_REF: %u (0x%04x)\n",
	       (unsigned int)bar30_prom[5], (unsigned int)bar30_prom[5]);
	printk("BAR30 C6 TEMPSENS: %u (0x%04x)\n",
	       (unsigned int)bar30_prom[6], (unsigned int)bar30_prom[6]);
#endif
}

static int bar30_cmd(uint8_t cmd)
{
	return i2c_write(bar30_i2c, &cmd, 1, BAR30_ADDR);
}

static uint8_t bar30_crc4(uint16_t prom[8])
{
	uint16_t rem = 0;

	prom[0] &= 0x0FFF;
	prom[7] = 0;

	for (uint8_t cnt = 0; cnt < 16; cnt++) {
		if ((cnt & 1) != 0) {
			rem ^= prom[cnt >> 1] & 0x00FF;
		} else {
			rem ^= prom[cnt >> 1] >> 8;
		}

		for (uint8_t bit = 8; bit > 0; bit--) {
			if ((rem & 0x8000) != 0) {
				rem = (rem << 1) ^ 0x3000;
			} else {
				rem <<= 1;
			}
		}
	}

	return (rem >> 12) & 0x0F;
}

static int bar30_read_prom(void)
{
	uint16_t crc_prom[8];
	uint8_t data[2];
	int ret;

	for (uint8_t i = 0; i < 7; i++) {
		uint8_t cmd = BAR30_CMD_PROM_READ + (i * 2);

		ret = i2c_write_read(bar30_i2c, BAR30_ADDR, &cmd, 1, data, sizeof(data));
		if (ret != 0) {
			return ret;
		}

		bar30_prom[i] = ((uint16_t)data[0] << 8) | data[1];
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(crc_prom); i++) {
		crc_prom[i] = bar30_prom[i];
	}

	if (bar30_crc4(crc_prom) != (bar30_prom[0] >> 12)) {
		return -EBADMSG;
	}

	return 0;
}

static int bar30_read_adc(uint8_t convert_cmd, uint32_t *adc)
{
	uint8_t cmd = BAR30_CMD_ADC_READ;
	uint8_t data[3];
	int ret;

	ret = bar30_cmd(convert_cmd);
	if (ret != 0) {
		return ret;
	}

	k_msleep(BAR30_CONVERSION_DELAY_MS);

	ret = i2c_write_read(bar30_i2c, BAR30_ADDR, &cmd, 1, data, sizeof(data));
	if (ret != 0) {
		return ret;
	}

	*adc = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
	if (*adc == 0) {
		return -EIO;
	}

	return 0;
}

static int32_t bar30_compensate_pressure_mbar(uint32_t d1, uint32_t d2)
{
	int32_t dt = (int32_t)d2 - ((int32_t)bar30_prom[5] * 256);
	int64_t sens = ((int64_t)bar30_prom[1] * 65536) + (((int64_t)bar30_prom[3] * dt) / 128);
	int64_t off = ((int64_t)bar30_prom[2] * 131072) + (((int64_t)bar30_prom[4] * dt) / 64);
	int32_t temp = 2000 + (((int64_t)dt * bar30_prom[6]) / 8388608);
	int32_t pressure_mbar;

	pressure_mbar = (int32_t)(((((int64_t)d1 * sens) / 2097152) - off) / 3276800);

#if BAR30_DEBUG
	printk("BAR30 D1: %u\n", (unsigned int)d1);
	printk("BAR30 D2: %u\n", (unsigned int)d2);
	printk("BAR30 dT: %d\n", dt);
	printk("BAR30 TEMP: %d.%02d C\n", temp / 100, temp < 0 ? -(temp % 100) : temp % 100);
	printk("BAR30 SENS: %lld\n", (long long)sens);
	printk("BAR30 OFF: %lld\n", (long long)off);
	printk("BAR30 pressure: %d mbar\n", pressure_mbar);
#endif

	return pressure_mbar;
}

int bar30_init(void)
{
	int ret;

	bar30_ready = false;

	if (!device_is_ready(bar30_i2c)) {
		return -ENODEV;
	}

	ret = bar30_cmd(BAR30_CMD_RESET);
	if (ret != 0) {
		return ret;
	}

	k_msleep(BAR30_RESET_DELAY_MS);

	ret = bar30_read_prom();
	if (ret != 0) {
		return ret;
	}

	bar30_log_calibration();

	bar30_ready = true;
	return 0;
}

int bar30_i2c_scan(void)
{
	uint8_t cmd = BAR30_CMD_ADC_READ;
	uint8_t data;
	int ret = -EIO;

	if (!device_is_ready(bar30_i2c)) {
		printk("I2C0 not ready\n");
		return -ENODEV;
	}

	printk("Checking Bar30 at I2C address 0x%02x...\n", BAR30_ADDR);

	for (uint8_t attempt = 0; attempt < BAR30_PROBE_RETRIES; attempt++) {
		ret = i2c_write_read(bar30_i2c, BAR30_ADDR, &cmd, 1, &data, 1);
		if (ret == 0) {
			printk("Bar30 ACK at 0x%02x\n", BAR30_ADDR);
			return 0;
		}

		k_msleep(BAR30_PROBE_RETRY_DELAY_MS);
	}

	printk("No Bar30 ACK at 0x%02x\n", BAR30_ADDR);
	return ret;
}

int bar30_read_pressure_mbar(int32_t *pressure_mbar)
{
	uint32_t d1;
	uint32_t d2;
	int ret;

	if (pressure_mbar == NULL) {
		return -EINVAL;
	}

	if (!bar30_ready) {
		return -EACCES;
	}

	ret = bar30_read_adc(BAR30_CMD_CONVERT_D1_OSR4096, &d1);
	if (ret != 0) {
		return ret;
	}

	ret = bar30_read_adc(BAR30_CMD_CONVERT_D2_OSR4096, &d2);
	if (ret != 0) {
		return ret;
	}

	*pressure_mbar = bar30_compensate_pressure_mbar(d1, d2);
	return 0;
}
