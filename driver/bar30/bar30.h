#ifndef BAR30_H
#define BAR30_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int bar30_init(void);
int bar30_i2c_scan(void);
int bar30_read_pressure_mbar(int32_t *pressure_mbar);

#ifdef __cplusplus
}
#endif

#endif /* BAR30_H */
