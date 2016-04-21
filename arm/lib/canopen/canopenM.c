/*
**                         Copyright 2001 by KVASER AB
**                   P.O Box 4076 SE-51104 KINNAHULT, SWEDEN
**             E-mail: staff@kvaser.se   WWW: http://www.kvaser.se
**
** This software is furnished under a license and may be used and copied
** only in accordance with the terms of such license.
**
**  $Revision: 1.2 $
**  $Header: /home/cvs/canopen/canopen/canopen_stack/hlp/copen/canopenM.c,v 1.2 2005/06/23 16:16:24 db Exp $
**
**
** CANOpen Master Functions.
**
*/

#include <string.h>
#include "canopenM.h"
#include "canopen.h"
#include "coConfig.h"
#include "kfHal.h"

CanChannel coGetCanChannel(void); /* Defined in canopen.c. */

/* Transmits an NMT message to the CAN bus; the message is also passed on to
* the CANopen stack.
*/
CoStackResult comNMT(CH uint8 nodeId, coNMTfunction fun) {
  CanHalCanStatus res;
  uint8 buf[2];
  buf[0] = fun;
  buf[1] = nodeId;
  res = canWriteMessageEx(coGetChannel(CHP_S), 0, buf, 2, 0);
  if( res != CANHAL_OK ){
   return(getCanHalError(res));
  }
  return (coRes_OK);
}

/* Reads the object (objIdx,subIdx) from node nodeId using the predefined SDO,
* stores the value in val (at most 32 bits can be read).
* While waiting for the response, all messages that isn't the expected
* response is passed to coProcessMessage().
* Returns 0 if success, or an error code.
*/
CoStackResult comReadObjectSDO(CH uint8 nodeId, uint16 objIdx, uint8 subIdx, uint32 *val) {
  uchar buf[8];
  CanId canidTx = 0x600+nodeId,
        canidRx = 0x580+nodeId;
  KfHalTimer t0;
  CanId id;
  uint8 dlc;
  CanHalCanStatus stat;

  buf[0] = 0x40; /* ccs = 2 */
  buf[1] = LSB(objIdx);
  buf[2] = MSB(objIdx);
  buf[3] = subIdx;
  memset(buf+4, 0, 4); /* Clear the reserved bytes */
  stat = canWriteMessageEx(coGetChannel(CHP_S), canidTx, buf, 8, 0);
  if (stat != CANHAL_OK)
    return (getCanHalError(stat));   /* Can-bus error */

  t0 = kfhalReadTimer();
  for (;;) {
    stat = canReadMessage(coGetChannel(CHP_S), &id, buf, &dlc);
    if (stat == CANHAL_OK) {
      if (id == canidRx && dlc == 8)
        break;
      else
        coProcessMessage(CHP id, buf, dlc);
    } else if (kfhalReadTimer()-t0 > TIMERFREQ)
      break;
  }
  if (stat != CANHAL_OK) {
    return (coRes_ERR_PROTOCOL);   /* The protocol timed out */
  }
  if ((buf[0] & 0xe0) == 0x040) { /* scs=2 */
    /* A valid response */
    if (buf[1] != LSB(objIdx) || buf[2] != MSB(objIdx) || buf[3] != subIdx)
      return (coRes_UNKNOW_FRAME); /* Response to something else? */
    if (buf[0] & 0x02) { /* The e-bit */
      /* Expedited transfer */
      uint8 dLen; /* Nmbr of data bytes, or 0 if unspecified */
      uint8 *p;
      if (buf[0] & 0x01) /* The s-bit */
        dLen = 4-((buf[0] >> 2) & 0x03);  /* es = 11 */
      else
        dLen = 0; /* es = 10 */
      if (dLen == 0)
        return (coRes_UNKNOW_FRAME); /* We must have a length */
      *val = 0;
      p = buf+4;
      while(dLen--)
        *val = (*val << 8)+*p++;
      return (coRes_OK); /* OK! */
    } else
      return (coRes_UNKNOW_FRAME); /* We do not support any other transfer protocol */
  } else if ((buf[0] & 0xe0) == 0x80) { /* scs = 4 */
    /* Abort SDO Transfer */
    *val = mkULONG(buf[4], buf[5], buf[6], buf[7]);
    return (coRes_ERR_PROTOCOL); /* So we never return 0 in case of an error */
  } else
    return (coRes_UNKNOW_FRAME);  /* Contents of frame is corrupt. */
}

#if 1

/* Writes the len LSB of val to the object (objIdx,subIdx) in node nodeId using
* the predefined SDO.
* Returns 0 if success, or an error code.
*/
CoStackResult comWriteObjectSDO(CH uint8 nodeId, uint16 objIdx, uint8 subIdx, uint32 val, uint8 len, coErrorCode *(canOpenResultCode)) {
  uchar buf[8];
  CanId canidTx = 0x600+nodeId,
        canidRx = 0x580+nodeId;
  KfHalTimer t0;
  CanId id;
  uint8 dlc;
  CanHalCanStatus stat;

  if (len == 0 || len > 4)
    return (coRes_ERR_DLC_OUT_OF_RANGE);  /* Contents of responseframe is corrupt */

  buf[0] = 0x23 + (uint8)((uint8)(4-len) << 2); /* ccs = 1, es = 11 */
  buf[1] = LSB(objIdx);
  buf[2] = MSB(objIdx);
  buf[3] = subIdx;
  buf[4] = (uint8)val;
  buf[5] = (uint8)(val >> 8);
  buf[6] = (uint8)(val >> 16);
  buf[7] = (uint8)(val >> 24);
  stat = canWriteMessageEx(coGetChannel(CHP_S), canidTx, buf, 8, 0);
  if (stat != CANHAL_OK)
    return (getCanHalError(stat));

  t0 = kfhalReadTimer();
  for (;;) {
    stat = canReadMessage(coGetChannel(CHP_S), &id, buf, &dlc);
    if (stat == CANHAL_OK) {
      if (id == canidRx && dlc == 8)
        break;
      else
        coProcessMessage(CHP id, buf, dlc);
    } else if (kfhalReadTimer()-t0 > TIMERFREQ)
      break;
  }
  if (stat != CANHAL_OK)
    return (coRes_ERR_PROTOCOL_TIMEOUT);
  if ((buf[0] & 0xe0) == 0x060) { /* scs=3 */
    /* A valid response */
    if (buf[1] != LSB(objIdx) || buf[2] != MSB(objIdx) || buf[3] != subIdx)
      return (coRes_UNKNOW_FRAME); /* Contents of responseframe is corrupt */
    return 0; /* OK! */
  } else if ((buf[0] & 0xe0) == 0x80) { /* scs = 4 */
    /* Abort SDO Transfer */
    *(canOpenResultCode) = mkULONG(buf[4], buf[5], buf[6], buf[7]);
    return(coRes_ERR_PROTOCOL); /* CANopen error */
  } else
    return (coRes_UNKNOW_FRAME); /* Contents of responseframe is corrupt */
}

#endif

/*
* Node Guarding Protocol
* ----------------------
* The master transmits a Node Guard Request message (an RTR) for each guarded
* node. The node should respond with a Heartbeat message containing the
* current state and a toggle bit. If two messages have the same value of the
* toggle bit, the second message is dropped.
* When a node changes state or there is no response, the application
* is notified.
*
*
* Heartbeat Protocol
* ------------------
* The guarded nodes are set up to transmit a heartbeat message periodically,
* the toggle bit is always zero.
* Whenever a message is received, the state is recorded. If there is no message
* for a certain amount of time, an error is reported.
*/

/* Transmit a node guard request message to node nodeId. */
void comNodeGuardRequest(CH uint8 nodeId) {
  (void)canWriteMessageEx(coGetChannel(CHP_S), CO_HEARTBEAT_COBID(nodeId) | CANHAL_ID_RTR, NULL, 0, 0);
}
