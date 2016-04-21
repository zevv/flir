#ifndef lib_crc16_h
#define lib_crc16_h

#define CRC16_INIT 0xFFFF

uint16_t crc_update(uint16_t crc, int8_t uc);
uint16_t crc_update_block(uint16_t crc, void *data, size_t len);

#endif

