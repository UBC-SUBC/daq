#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

typedef enum{
    un_init = 0,
    init,
    pending,
    pending_calculation,
    failed,
    complete,
}operation_status;

typedef struct 
{
    operation_status BME280;
    operation_status Barometer;
    operation_status IMU;
    operation_status hall_effect_sensor;
    operation_status SD_card;
}dev_status;

//TODO FIND OUT MORE ABOUT OTHER SENSORs
#endif /* MAIN_H */
