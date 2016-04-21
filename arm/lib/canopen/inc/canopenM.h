/*
**                         Copyright 2001 by KVASER AB
**                   P.O Box 4076 SE-51104 KINNAHULT, SWEDEN
**             E-mail: staff@kvaser.se   WWW: http://www.kvaser.se
**
** This software is furnished under a license and may be used and copied
** only in accordance with the terms of such license.
**
**  $Revision: 1.1.1.1 $
**  $Header: /home/cvs/canopen/canopen/canopen_stack/inc/canopenM.h,v 1.1.1.1 2005/06/23 15:52:24 db Exp $
**
**
** CANOpen Master Functions.
**
*/

#ifndef __CANOPENM_H
#define __CANOPENM_H

#include "kfCommon.h"
#include "canopen.h"

#ifdef __cplusplus
extern "C" {
#endif

	CoStackResult comNMT(CH uint8 nodeId, coNMTfunction fun);
	void comNodeGuardRequest(CH uint8 nodeId);
	CoStackResult comReadObjectSDO(CH uint8 nodeId, uint16 objIdx, uint8 subIdx, uint32 *val);
	CoStackResult comWriteObjectSDO(CH uint8 nodeId, uint16 objIdx, uint8 subIdx, uint32 val, uint8 len, coErrorCode *(canOpenResultCode));

#ifdef __cplusplus
}
#endif


#endif /* __CANOPENM_H */
