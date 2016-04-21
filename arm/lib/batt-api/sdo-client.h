#ifndef sdo_client_h
#define sdo_client_h

typedef void (*sdo_read_callback)(uint8_t node_id, uint16_t objidx, uint8_t subidx, void *data, size_t len, uint32_t error, void *fndata);

rv sdo_read(CoHandle co, uint8_t node_id, uint16_t objidx, uint8_t subidx, sdo_read_callback fn, void *fndata);

#endif

