/*
**                         Copyright 2001 by KVASER AB
**                   P.O Box 4076 SE-51104 KINNAHULT, SWEDEN
**                          WWW: http://www.kvaser.se
**
** This software is furnished under a license and may be used and copied
** only in accordance with the terms of such license.
**
** $Header: /home/cvs/canopen/canopen/canopen_stack/inc/kfCanHal.h,v 1.2 2005/07/05 07:12:54 omicron Exp $
** $Revision: 1.2 $
**
** Description:
**   CAN Hardware Abstraction Layer for Kingdom Founder.
**   Platform specific functions.
**
*/

#ifndef _KF_CANHAL_H_
#define _KF_CANHAL_H_

#include "kfCommon.h"

#define CAN_MESSAGE_LEN 8

#ifndef USE_EXTENDED_CAN
  #define USE_EXTENDED_CAN 1
#endif

typedef unsigned short CanHalStatusFlags;

#define CANHAL_STAT_ERROR_PASSIVE 0x0001
#define CANHAL_STAT_BUS_OFF 0x0002
#define CANHAL_STAT_ERROR_WARNING 0x0004
#define CANHAL_STAT_ERROR_ACTIVE 0x0008
#define CANHAL_STAT_TX_PENDING 0x0010
#define CANHAL_STAT_RX_PENDING 0x0020
#define CANHAL_STAT_TXERR 0x0040
#define CANHAL_STAT_RXERR 0x0080
#define CANHAL_STAT_BUFFER_OVERFLOW 0x0100

#define CANOPEN_CAN_BIT_RATE_1000000 0
#define CANOPEN_CAN_BIT_RATE_800000 1
#define CANOPEN_CAN_BIT_RATE_500000 2
#define CANOPEN_CAN_BIT_RATE_250000 3
#define CANOPEN_CAN_BIT_RATE_125000 4
#define CANOPEN_CAN_BIT_RATE_100000 5
#define CANOPEN_CAN_BIT_RATE_50000 6
#define CANOPEN_CAN_BIT_RATE_25000 7



typedef enum {
  CANHAL_OK                         = 0,
  CANHAL_ERR_PARAM                  = -1, /* Error in parameter */
  CANHAL_ERR_NOMSG                  = -2, /* No messages available */
  CANHAL_ERR_NOTFOUND               = -3, /* Specified hw not found */
  CANHAL_ERR_TIMEOUT                = -4, /* Timeout ocurred */
  CANHAL_ERR_NOTINITIALIZED         = -5, /* Lib not initialized */
  CANHAL_ERR_OVERFLOW               = -6, /* Transmit buffer overflow */
  CANHAL_ERR_HARDWARE               = -7, /* Some hardware error has occurred */
  CANHAL_ERR_OTHER                  = -256  /* Some other error occurred */
} CanHalCanStatus;

#define CANHAL_DRIVERMODE_NORMAL    1
#define CANHAL_DRIVERMODE_SILENT    0

#if USE_EXTENDED_CAN
 typedef unsigned long CanId;
 #define CANHAL_ID_INVALID 0xffffffff /* Marks that the envelope isn't defined */
 #define CANHAL_ID_MASK 0x1fffffff /* Masks the id bits only */
 #define CANHAL_ID_EXTENDED 0x80000000L /* Bit set in extended CAN id's */
 #define CANHAL_ID_RTR 0x40000000L /* Bit set in the ID of an RTR message id. */
#else
 typedef unsigned short CanId;
 #define CANHAL_ID_INVALID 0xffff /* Marks that the envelope isn't defined */
 #define CANHAL_ID_MASK 0x07ff /* Masks the id bits only */
 #define CANHAL_ID_EXTENDED 0x0000 /* No CAN id's are extended */
 #define CANHAL_ID_RTR 0x4000 /* Bit set in the ID of an RTR message id. */
#endif
#define CANHAL_DLC_INVALID      255

typedef int CanChannel;
typedef unsigned short CanBitRate;
typedef int CanDriverMode;

typedef unsigned char CanCbType;
#define CANHAL_CB_FIRST_RECEIVE 1 /* A message was placed in an empty recevie queue, msg = NULL */
#define CANHAL_CB_TX_DONE 2 /* The message *msg is transmitted */
#define CANHAL_CB_TXQ_EMPTY 4 /* All messages has been transmitted, msg = NULL */
#define CANHAL_CB_MSG_REC 8 /* Called before message *msg is placed in receive queue. Returns true if message should be skipped */
#define CANHAL_CB_ERROR 16  /* Error interrupt */
#define CANHAL_CB_BUS_OFF 32 /* Bus off occurred */
#define CANHAL_CB_RX_OVERRUN 64 /* MikkoR 2004-07-15: Added callback for rx overrun */
#define CANHAL_CB_ERR_PASSIVE 128 /* MikkoR 2004-08-09: Added callback for error passive state */

typedef struct {
  CanId id;
  uchar dlc;
  uchar spare; /* make struct word-aligned */
  uchar data[CAN_MESSAGE_LEN];
  unsigned long tag;
/*
  KfHalTimer time;
*/
} CANMessage;
CompilerAssert((sizeof(CANMessage) & 1) == 0);

#ifdef __cplusplus
extern "C" {
#endif
typedef unsigned char (*CanCallBackFun)(CanChannel chan, CanCbType cbType, const CANMessage *msg);

/* initialize the CAN software */
void canInitialize(void);

/* initialize the CAN hardware */
CanHalCanStatus canSetup(CanChannel chan, CanBitRate btr);

/* set the callback function */
CanHalCanStatus canSetCallback(CanChannel chan, CanCallBackFun cbFunI, CanCbType cbMaskI);

/* set the CAN baudrate */
CanHalCanStatus canSetBaudrate(CanChannel chan, CanBitRate btr);

/* Go on bus */
CanHalCanStatus canGoBusOn (CanChannel chan);

/* Go off bus */
CanHalCanStatus canGoBusOff (CanChannel chan);

/* Reads a CAN message from the bus. If it is an extended CAN id, id:31 is set. */
CanHalCanStatus canReadMessage (CanChannel chan, CanId *id, uchar *msg, unsigned char *dlc);

/* variant useful if a callback function is registered for transmitted messags; tag is returned in the message struct. */
CanHalCanStatus canWriteMessageEx (CanChannel chan, CanId id, const uchar *msg, unsigned char dlc, unsigned long tag);



/* Set communication mode. Depending on hardware, some modes may not be supported, in
* which case a higher mode is selected. */
CanHalCanStatus canSetTransceiverMode (CanChannel chan, CanDriverMode driverMode);

CanHalCanStatus canGetStatus (CanChannel chan, CanHalStatusFlags *flags);

CanHalCanStatus canGetErrorCounters (CanChannel chan,
                                     uint16 *txErr,
                                     uint16 *rxErr,
                                     uint16 *ovErr);

/* Gets the number of bytes (rx and tx) that have been transferred to/from a CAN channel */
//CanHalCanStatus canGetChannelBitCount(CanChannel chan, unsigned long* bits);

/* Shut down the CAN communication */
CanHalCanStatus canShutdown (CanChannel chan);
/* Reset CAN controller */
CanHalCanStatus canReset (CanChannel chan);

/* Status changed Interrupt handler */
void CanBusStatusChange ( unsigned  char busId);

/* Gives a list of used CAN IDs (COB IDs) to the HAL layer */
CanHalCanStatus canSetUsedCanIds( CanChannel chan, CanId *idList, uint8 listSize);

/* Updates the CAN ID filters */
/* This function shall only be implemented if the filters can be updated
 * without resetting the controller and losing messages */
CanHalCanStatus canUpdateFilters( CanChannel chan);

#ifdef __cplusplus
}
#endif

#endif /* _KF_CANHAL_H_ */
