#ifndef _RC522_CUSTOM_H_
#define _RC522_CUSTOM_H_

#include <esp_log.h>
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"

void rc522_custom_init(void);

void rc522_custom_start(void);

void rc522_custom_pause(void);


#endif