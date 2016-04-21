#ifndef lib_canopen_hal_can_h
#define lib_canopen_hal_can_h

const char *co_strerror(uint32_t err);
void co_bind_dev(CanChannel chan, struct dev_can *dev);
void co_bridge_to(CanChannel chan, uint8_t node_id);

#endif

