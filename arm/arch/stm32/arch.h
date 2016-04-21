#ifndef drv_x86_arch_h
#define drv_x86_arch_h

#include "bios/bios.h"
#include "stm32f1xx_hal.h"

extern struct drv_arch drv_arch_stm32;

struct drv_arch_stm32_data {
};

rv hal_status_to_rv(HAL_StatusTypeDef status);

#endif
