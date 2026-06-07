#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

typedef enum{
    pending = 0,
    running, // likly not use?
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
#endif /* MAIN_H */
