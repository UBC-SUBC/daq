#ifndef CUSTOM_BME280_DRIVER_H
#define CUSTOM_BME280_DRIVER_H

#include <zephyr/drivers/spi.h>

#include "Bosch_driver/bme280.h"

#ifdef __cplusplus
extern "C" {
#endif

int8_t custom_bme280_init(struct bme280_dev *dev);
int8_t custom_bme280_read_sensor_data(struct bme280_dev *dev, struct bme280_data *data);

#ifdef __cplusplus
}
#endif

#endif /* CUSTOM_BME280_DRIVER_H */
