
/*
 * This file holds most of the glue between the superb application framework
 * and the kvaser CANOpen stack by implementer the kvaser can HAL interface.
 * Some additional functions are included for dumping can traffic and
 * translating canopen error codes to readable messages.
 */

#include <stdint.h>
#include <errno.h>
#include <string.h>

#if 0
#define debug printd
#else
#define debug(...)
#endif

#include "plat.h"

#include "bios/arch.h"
#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/can.h"
#include "bios/evq.h"

#include "lib/canopen/inc/canopen.h"
#include "lib/canopen/analyzer.h"

#define KVASER_MAX_CHANNELS 2
#define KVASER_MAX_CALLBACKS 5
#define KVASER_RX_QUEUE_SIZE 4

struct can_state {
	struct dev_can *dev;
	int chan;
	CanCallBackFun fn_cb[KVASER_MAX_CALLBACKS];
	CANMessage rx_queue[KVASER_RX_QUEUE_SIZE];
	uint8_t rx_head;
	uint8_t rx_tail;
	uint8_t bridge_id;
};

static struct can_state can_state[KVASER_MAX_CHANNELS];


/*
 * Handles both canopen stack result codes and SDO abort codes, since these
 * do not overlap.
 */

const char *co_strerror(uint32_t r)
{
	switch(r) {
#ifdef LIB_CANOPEN_STRERROR
	case coRes_OK                                       : return "ok";
	case coRes_CAN_ERR                                  : return "can err";
	case coRes_ERR_CAN_PARAM                            : return "can param";
	case coRes_ERR_CAN_NOMSG                            : return "can nomsg";
	case coRes_ERR_CAN_NOTFOUND                         : return "can notfound";
	case coRes_ERR_CAN_TIMEOUT                          : return "can timeout";
	case coRes_ERR_CAN_NOTINITIALIZED                   : return "can notinitialized";
	case coRes_ERR_CAN_OVERFLOW                         : return "can overflow";
	case coRes_ERR_CAN_HARDWARE                         : return "can hardware";
	case coRes_ERR_CAN_OTHER                            : return "can other";
	case coRes_ERR_CAN_SHUT_DOWN                        : return "can shut down";
	case coRes_ERR_CAN_GO_ON_BUS                        : return "can go on bus";
	case coRes_ERR_CAN_CALLBACK_SETUP                   : return "can callback setup";
	case coRes_ERR_RESET_COMM                           : return "reset comm";
	case coRes_ERR_RESET_NODE_ID                        : return "reset node id";
	case coRes_ERR_DEFAULT_PDO_MAP_SIZE                 : return "default pdo map size";
	case coRes_ERR_DEFAULT_PDO_MAP_ARGUMENTS            : return "default pdo map arguments";
	case coRes_ERR_DEFAULT_PDO_MAP_PDO_SIZE             : return "default pdo map pdo size";
	case coRes_ERR_GEN_HEARTBEAT                        : return "gen heartbeat";
	case coRes_ERR_GEN_SYNC                             : return "gen sync";
	case coRes_ERR_TPDO_NOT_OPERATIONAL                 : return "tpdo not operational";
	case coRes_ERR_TPDO_NO_MAP_FOUND                    : return "tpdo no map found";
	case coRes_ERR_TPDO_APP_ABORT                       : return "tpdo app abort";
	case coRes_ERR_TPDO_OBJECT_NOT_FOUND                : return "tpdo object not found";
	case coRes_ERR_TPDO_INHIBIT_TIME_NOT_ELAPSED        : return "tpdo inhibit time not elapsed";
	case coRes_ERR_TPDO_NOT_FOUND                       : return "tpdo not found";
	case coRes_ERR_UNDEFINED_SDO_NO                     : return "undefined sdo no";
	case coRes_ERR_SDO_BUSY                             : return "sdo busy";
	case coRes_ERR_EMCY_CNTRL_NODE_NOT_FOUND            : return "emcy cntrl node not found";
	case coRes_ERR_EMCY_CNTRL_ID                        : return "emcy cntrl id";
	case coRes_ERR_EMCY_CNTRL_LIST_FULL                 : return "emcy cntrl list full";
	case coRes_ERR_CFG_NODEGUARD                        : return "cfg nodeguard";
	case coRes_ERR_CFG_NODEGUARD_ID_NOT_FOUND           : return "cfg nodeguard id not found";
	case coRes_ERR_CFG_NODEGUARD_LIST_FULL              : return "cfg nodeguard list full";
	case coRes_ERR_DLC_REQUEST_PDO                      : return "dlc request pdo";
	case coRes_ERR_TX_PDO_RECEIVED_IS_RX                : return "tx pdo received is rx";
	case coRes_OK_UNPROCESSED                           : return "ok unprocessed";
	case coRes_ERR_PROTOCOL                             : return "protocol";
	case coRes_ERR_PROTOCOL_TIMEOUT                     : return "protocol timeout";
	case coRes_ERR_DLC_OUT_OF_RANGE                     : return "dlc out of range";
	case coRes_UNKNOW_FRAME                             : return "unknow frame";
	case coRes_ERR_NODE_ID_OUT_OF_RANGE                 : return "node id out of range";
	case coRes_ERR_INVALID_STATE                        : return "invalid state";
	case coRes_ERR_DYNAMIC_MAP_RESET                    : return "dynamic map reset";
	case coRes_ERR_OBJ_OUT_OF_RANGE                     : return "obj out of range";
	case coRes_ERR_NO_MAP_AVAILABLE                     : return "no map available";
	case coRes_OUT_OF_MEMORY                            : return "out of memory";
	case coRes_ERR_CREATE_PDO                           : return "create pdo";
	case coRes_ERR_OBJ_NOT_MAPABLE                      : return "obj not mapable";
	case coRes_ERR_SUB_IDX_OUT_OF_RANGE                 : return "sub idx out of range";
	case coRes_ERR_CANOPEN                              : return "canopen";
	case coRes_ERR_PDO_DISABLED                         : return "pdo disabled";
	case coRes_ERR_CONFIGURE_CLIENT_SDO                 : return "configure client sdo";
	case coRes_ERR_NODE_BY_NODE_RST_GENERAL             : return "node by node rst general";
	case coRes_ERR_CFG_CLIENT_SDO                       : return "cfg client sdo";
	case coRes_ERR_NODEID_NOT_FOUND_IN_BOOTLIST         : return "nodeid not found in bootlist";
	case coRes_ERR_BOOT_NEXT_NODE                       : return "boot next node";
	case coRes_ERR_NO_NODES_FOUND                       : return "no nodes found";
	case coRes_ERR_NO_NODES_TO_BOOT                     : return "no nodes to boot";
	case coRes_REBOOT_FAILING_NODES                     : return "reboot failing nodes";
	case coRes_ERR_CONFIGURE_VENDOR_ID                  : return "configure vendor id";
	case coRes_ERR_CONFIGURE_PRODUCT_CODE               : return "configure product code";
	case coRes_ERR_CONFIGURE_REVISION_NUMBER            : return "configure revision number";
	case coRes_ERR_CONFIGURE_SERIAL_NUMBER              : return "configure serial number";
	case coRes_ERR_BUFFER_LEN_INVALID                   : return "buffer len invalid";
	case coRes_ERR_NO_VALID_BUF_DATA                    : return "no valid buf data";
	case coRes_ERR_MESSAGE_DLC                          : return "message dlc";
	case coRes_ERR_INVALID_NODE_ID_SAM_PDO              : return "invalid node id sam pdo";
	case coRes_ERR_HANDLE_MPDO                          : return "handle mpdo";
	case coRes_ERR_TPDO_IS_NOT_DAM_PDO                  : return "tpdo is not dam pdo";
	case coRes_ERR_NO_MAPPED_OBJ_DAM_PDO                : return "no mapped obj dam pdo";
	case coRes_ERR_PACK_MPDO_BUF_LEN                    : return "pack mpdo buf len";
	case coRes_ERR_MPDO_TYPE_UNKNOWN                    : return "mpdo type unknown";
	case coRes_ERR_NO_SAM_TPDO_FOUND                    : return "no sam tpdo found";
	case coRes_ERR_PACK_DISPLIST_TO_BUF_LEN             : return "pack displist to buf len";

	case CO_ERR_TOGGLE_BIT_NOT_ALTERED                  : return "toggle bit not altered";
	case CO_ERR_SDO_PROTOCOL_TIMED_OUT                  : return "sdo protocol timed out";
	case CO_ERR_UNKNOWN_CMD                             : return "unknown cmd";
	case CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT            : return "unsupported access to object";
	case CO_ERR_OBJECT_READ_ONLY                        : return "object read only";
	case CO_ERR_OBJECT_WRITE_ONLY                       : return "object write only";
	case CO_ERR_OBJECT_DOES_NOT_EXISTS                  : return "object does not exists";
	case CO_ERR_OBJECT_LENGTH                           : return "object length";
	case CO_ERR_SERVICE_PARAM_TOO_HIGH                  : return "service param too high";
	case CO_ERR_SERVICE_PARAM_TOO_LOW                   : return "service param too low";
	case CO_ERR_INVALID_SUBINDEX                        : return "invalid subindex";
	case CO_ERR_OBJDICT_DYN_GEN_FAILED                  : return "objdict dyn gen failed";
	case CO_ERR_GENERAL                                 : return "general";
	case CO_ERR_GENERAL_PARAMETER_INCOMPATIBILITY       : return "general parameter incompatibility";
	case CO_ERR_GENERAL_INTERNAL_INCOMPABILITY          : return "general internal incompability";
	case CO_ERR_NOT_MAPABLE                             : return "not mapable";
	case CO_ERR_DATA_CAN_NOT_BE_STORED                  : return "data can not be stored";
	case CO_ERR_OUT_OF_MEMORY                           : return "out of memory";
	case CO_ERR_PDO_LENGHT                              : return "pdo lenght";
	case CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED            : return "val range for param exceeded";
	case CO_ERR_VALUE_TOO_HIGH                          : return "value too high";
	case CO_ERR_VALUE_TOO_LOW                           : return "value too low";
	case CO_ERR_ABORT_SDO_TRANSFER                      : return "abort sdo transfer";
#endif
	default: return "unknown";
	}
}


void co_bind_dev(CanChannel chan, struct dev_can *dev)
{
	if(chan < KVASER_MAX_CHANNELS) {
		struct can_state *cs = &can_state[chan];
		cs->chan = chan;
		cs->dev = dev;
	}
}


void co_bridge_to(CanChannel chan, uint8_t node_id)
{
	if(chan < KVASER_MAX_CHANNELS) {
		struct can_state *cs = &can_state[chan];
		cs->bridge_id = node_id;
	}
}


static uint8_t call_callback(struct can_state *cs, CanCbType mask, CANMessage *msg)
{
	uint8_t r = 0;
	uint8_t i;

	for(i=0; i<8; i++) {
		if(mask & (1u<<i)) {
			CanCallBackFun fn = cs->fn_cb[i];
			//printd("Callback %d 0x%x\n", i, fn);
			if(fn) {
				r = fn(cs->chan, mask, msg);
			}
		}
	}

	return r;
}



KfHalTimer kfhalReadTimer(void)
{
	return arch_get_ticks();
}


CanHalCanStatus canSetUsedCanIds( CanChannel chan, CanId *idList, uint8 listSize)
{
	debug("%s\n", __FUNCTION__);
	return CANHAL_OK;
}


CanHalCanStatus canUpdateFilters( CanChannel chan)
{
	debug("%s\n", __FUNCTION__);
	return CANHAL_OK;
}


void canInitialize(void)
{
	debug("%s\n", __FUNCTION__);
}


CanHalCanStatus canSetup (CanChannel chan, CanBitRate btr)
{
	debug("%s %d\n", __FUNCTION__, chan);
	return CANHAL_OK;
}


CanHalCanStatus canShutdown(CanChannel chan)
{
	debug("%s %d\n", __FUNCTION__, chan);
	return CANHAL_OK;
}


CanHalCanStatus canTransceiverMode(CanChannel chan, int driverMode)
{
	debug("%s %d\n", __FUNCTION__, chan);
	return CANHAL_OK;
}


CanHalCanStatus canSetBaudrate(CanChannel chan, CanBitRate btr)
{
	debug("%s %d\n", __FUNCTION__, chan);
	return CANHAL_OK;
}


CanHalCanStatus canGoBusOn(CanChannel chan)
{
	debug("%s %d\n", __FUNCTION__, chan);
	return CANHAL_OK;
}


CanHalCanStatus canGoBusOff(CanChannel chan)
{
	debug("%s %d\n", __FUNCTION__, chan);
	return CANHAL_OK;
}


CanHalCanStatus canReadMessage(CanChannel chan, CanId *id, uchar *msg, unsigned char *dlc)
{
	//debug("%s %d\n", __FUNCTION__, chan);
	
	if(chan < KVASER_MAX_CHANNELS) {

		struct can_state *cs = &can_state[chan];

		if(cs->rx_head != cs->rx_tail ) {
			CANMessage *m = &cs->rx_queue[cs->rx_tail];
			*id = m->id;
			*dlc = m->dlc;
			memcpy(msg, m->data, m->dlc);
			cs->rx_tail = (cs->rx_tail + 1) % KVASER_RX_QUEUE_SIZE;
			return CANHAL_OK;
		} else {
			return CANHAL_ERR_NOMSG;
		}
	} else {
		return CANHAL_ERR_NOTFOUND;
	}
}


CanHalCanStatus canWriteMessageEx(CanChannel chan, CanId id, const uchar *msg, unsigned char dlc, unsigned long tag)
{
	debug("%s %d\n", __FUNCTION__, chan);

	if(chan < KVASER_MAX_CHANNELS) {
		struct can_state *cs = &can_state[chan];

		if(cs->dev == NULL) {
			return CANHAL_ERR_NOTINITIALIZED;
		}

		enum can_address_mode_t mode = (id & 0x80000000) ? CAN_ADDRESS_MODE_EXT : CAN_ADDRESS_MODE_STD;
		id &= 0x7fffffff;

		canalyzer_dump("tx", cs->dev, mode, id, msg, dlc);

		rv r = can_tx(cs->dev, mode, id, msg, dlc);

		if(r == RV_OK) {
			CANMessage msg2;
			msg2.id = id;
			msg2.dlc = dlc;
			msg2.tag = tag;
			call_callback(cs, CANHAL_CB_TX_DONE, &msg2);
			return CANHAL_OK;
		} else {
			return CANHAL_ERR_OVERFLOW;
		}
	} else {
		return CANHAL_ERR_NOTFOUND;
	}
}


CanHalCanStatus canSetTransceiverMode (CanChannel chan, CanDriverMode driverMode)
{
	debug("%s %d\n", __FUNCTION__, chan);
	return CANHAL_OK;
}


CanHalCanStatus canGetStatus(CanChannel chan, CanHalStatusFlags *flags)
{
	debug("%s %d\n", __FUNCTION__, chan);
	return CANHAL_OK;
}


CanHalCanStatus canGetErrorCounters (CanChannel chan, uint16 *txErr, uint16 *rxErr, uint16 *ovErr)
{
	debug("%s %d\n", __FUNCTION__, chan);

	CanHalCanStatus r;

	if(chan < KVASER_MAX_CHANNELS) {
		struct can_state *cs = &can_state[chan];

		if(cs->dev) {
			struct can_stats stats;
			can_get_stats(cs->dev, &stats);
	
			*txErr = stats.tx_err;
			*rxErr = stats.rx_err;
			*ovErr = stats.tx_xrun;

			r = CANHAL_OK;
		} else {
			r = CANHAL_ERR_NOTINITIALIZED;
		}
	} else {
		r = CANHAL_ERR_NOTFOUND;
	}

	return r;
}


CanHalCanStatus canReset(CanChannel chan)
{
	debug("%s %d\n", __FUNCTION__, chan);
	CanHalCanStatus r;

	if(chan < KVASER_MAX_CHANNELS) {
		r = CANHAL_OK;
	} else {
		r = CANHAL_ERR_NOTFOUND;
	}

	return r;
}


CanHalCanStatus ckhalDisableCANInterrupts(CanChannel chan)
{
	debug("%s %d\n", __FUNCTION__, chan);
	return CANHAL_OK;
}


CanHalCanStatus ckhalEnableCANInterrupts(CanChannel chan)
{
	debug("%s %d\n", __FUNCTION__, chan);
	return CANHAL_OK;
}


CanHalCanStatus canSetCallback(CanChannel chan, CanCallBackFun cbFunI, CanCbType cbMaskI)
{
	debug("%s %d 0x%02x\n", __FUNCTION__, chan, cbMaskI);

	if(chan < KVASER_MAX_CHANNELS) {
		struct can_state *cs = &can_state[chan];

		uint8_t i;
		for(i=0; i<8; i++) {
			if(cbMaskI & (1u<<i)) {
				cs->fn_cb[i] = cbFunI;
			}
		}
		return CANHAL_OK;
	} else {
		return CANHAL_ERR_NOTFOUND;
	}
}



/*
 * Event handler for received CAN frames. Each message is passed to the
 * CANHAL_CB_MSG_REC callback function, and depending on its return value
 * pushed on the receive queue for that channel. The message will later need to
 * be removed from the queue by the canReadMessage() function.
 */

static void on_ev_can(event_t *ev, void *data)
{
	int i;

	struct ev_can *ec = &ev->can;
	
	canalyzer_dump("rx",
		ec->dev,
		(ec->extended == 1u) ? CAN_ADDRESS_MODE_EXT : CAN_ADDRESS_MODE_STD, 
		ec->id, ec->data, ec->len);

	for(i=0; i<KVASER_MAX_CHANNELS; i++) {

		struct can_state *cs = &can_state[i];

		if(cs->dev == ec->dev) {

			if(cs->bridge_id) {

				/* Bridge all messages, execpt write to SDO 0x2FFF.1 */
				
				if((ec->len != 8) ||
				   (ec->data[0] != 0x2f) || (ec->data[1] != 0xff) ||
				   (ec->data[2] != 0x2f) || (ec->data[3] != 0x01)) {
				
					uint8_t dest_id = ec->id & 0x7f;
					uint8_t my_node_id = 1; //coGetNodeId(scanner->ch);
					
					ec->id &= ~0x7f;

					if(dest_id == my_node_id) {
						ec->id |= cs->bridge_id;
					} else {
						ec->id |= 1;
					}

					canWriteMessageEx(i, ec->id, ec->data, ec->len, 0);
					return;
				}
			}

			uint8_t head_next = (cs->rx_head + 1) % KVASER_RX_QUEUE_SIZE;

			if(head_next != cs->rx_tail) {

				CANMessage msg;
				msg.dlc = ec->len;
				memcpy(msg.data, ec->data, msg.dlc);

				uint8_t r = call_callback(cs, CANHAL_CB_MSG_REC, &msg);

				if(r == 0) {
					uint8_t first = (cs->rx_head == cs->rx_tail);

					/* Push to RX queue */

					CANMessage *m = &cs->rx_queue[cs->rx_head];
					m->id = ec->id;
					m->dlc = ec->len;
					memcpy(m->data, ec->data, ec->len);
					cs->rx_head = head_next;

					/* Signal reception of first msg in rxqueue */

					if(first) {
						(void)call_callback(cs, CANHAL_CB_FIRST_RECEIVE, NULL);
					}

					int i;
					for(i=0; i<10; i++) {
						coPoll();
					}
				}
			}
		}
	}
}

EVQ_REGISTER(EV_CAN, on_ev_can);


/*
 * End
 */
