#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

typedef enum{
    pending = 0,
    running,
    complete,
}operation_status;

typedef struct 
{
    operation_status BME280;
    operation_status xxx_sensor;
    operation_status six_seven_sensor;
    operation_status hall_effect_sensor;
    operation_status SD_card;
}dev_status;

//TODO FIND OUT MORE ABOUT OTHER SENSORs
typedef struct {
    uint8_t temp;
    uint8_t pressure;
    uint8_t rpm;
    uint8_t xxx;
    uint8_t yyy;
    uint8_t ccc;
}sd_data_struct;

const char header[] = "temp,pressure,rmp,xxx,yyy,zzz";

#endif /* MAIN_H */
