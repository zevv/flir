
#include <stdint.h>
#include <stddef.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "lib/crc16.h"

#define CRC16_POLY 0x1021  /**< CRC 16 polynom */

uint16_t crc_update(uint16_t crc, int8_t uc)
{
        uint16_t uwCmpt;
        crc = crc ^ (int32_t) uc << 8;
        for (uwCmpt= 0; uwCmpt < 8; uwCmpt++)
        {
                if (crc & 0x8000)
                        crc = crc << 1 ^ CRC16_POLY;
                else
                        crc = crc << 1;
        }
        return (crc & 0xFFFF);
}


uint16_t crc_update_block(uint16_t crc, void *data, size_t len)
{
        uint8_t *p = data;

        while(len) {
                crc = crc_update(crc, *p);
                p ++;
                len --;
        }

        return crc;
}


/*
 * End
 */
