
/*
 * This file holds all the canopen application callbacks that we are not using
 * in the application. These used to be defined as weak symbols inside the
 * lib/canopen directory but because linking for win32 does not support weak
 * symbols we will need to add them hear and manually sort out which to include
 */

#include <stdint.h>
#include <string.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "lib/canopen/inc/canopen.h"
#include "lib/canopen/hal-can.h"

#define __attribute__ (( __weak__ ))

#if 1
#define debug printd
#else
#define debug(...)
#endif


void appError(CH uint16 errType, uint8 pos, uint16 param1, uint8 param2, uint32 param3)
{
	debug("%s type %d pos %d (%d,%d,%d)\n", __FUNCTION__, errType, pos, param1, param2, param3);
}


coErrorCode appWriteObjectResponseSDO(CH uint8 sdoNo,uint16 objIdx, uint8 subIdx, uint8 *bufPtr, CoObjLength pos, uint8 *(valid), uint8 sizeOfBuffer)
{
	debug("%s\n", __FUNCTION__);
	return 0;
}


void appIndicateNodeGuardStateChange(CH uint8 nodeId, coOpState state)
{
	debug("%s: %d->%d\n", __FUNCTION__, nodeId, state);
}


void appIndicateSyncWindowLengthExpire(CH uint16 pdoNo)
{
	debug("%s\n", __FUNCTION__);
}


void appIndicateNodeGuardFailure(CH uint8 nodeId)
{
	debug("%s: %d\n", __FUNCTION__, nodeId);
}


CoPDOaction appIndicatePDOReception(CH uint16 pdoNo, uint8 *buf, uint8 dlc)
{
	debug("%s\n", __FUNCTION__);
	return coPA_Done;
}


CoPDOaction appIndicatePDORequest(CH uint16 pdoNo)
{
	debug("%s\n", __FUNCTION__);
	return coPA_Continue;
}


void appNotifyPDOReception(CH uint16 pdoNo)
{
	debug("%s\n", __FUNCTION__);
}


coErrorCode appWriteObject(CH uint16 objIdx, uint8 subIdx, uint8 *bufPtr, CoObjLength pos, uint8 valid, CoSegTranAction flag )
{
	debug("%s\n", __FUNCTION__);
	return 0;
}


coErrorCode appReadObject(CH uint16 objIdx, uint8 subIdx, uint8 *bufPtr, CoObjLength pos, uint8 *(valid), uint8 sizeOfBuffer )
{
	debug("%s\n", __FUNCTION__);
	return 0;
}


uint8 appGetObjectAttr(CH uint16 objIdx, uint8 subIdx)
{
	debug("%s %04x.%02x\n", __FUNCTION__, objIdx, subIdx);
	return 0;
}


coErrorCode appODWriteVerify(CH uint16 objIdx, uint8 subIdx, uint32 val)
{
	debug("%s %04x.%02x\n", __FUNCTION__, objIdx, subIdx);
	return 0;
}


void appEmcyError(CH uint8 nodeId, uint16 emcyErrCode, uint8 errReg, uint8* manfSpecErrField)
{
	debug("%s\n", __FUNCTION__);
}


uint8 appGetMfgDeviceNameLen(CH_S)
{
	debug("%s\n", __FUNCTION__);
	return 6;
}

bool appGetMfgDeviceName(CH uint8 *mfgDeviceName, uint8 len)
{
	debug("%s\n", __FUNCTION__);
	memcpy(mfgDeviceName, "noname", len);
	return true;
}

uint8 appGetMfgHwVersionLen(CH_S)
{
	debug("%s\n", __FUNCTION__);
	return 0;
}

bool appGetMfgHwVersion(CH uint8 *mfgHwVersion, uint8 len)
{
	debug("%s\n", __FUNCTION__);
	return true;
}


uint8 appGetMfgSwVersionLen(CH_S)
{
	debug("%s\n", __FUNCTION__);
	return 0;
}


bool appGetMfgSwVersion(CH uint8 *mfgSwVersion, uint8 len)
{
	debug("%s\n", __FUNCTION__);
	return true;
}

void appNotifyBootError(CH uint8 nodeId, bootResultStates bootErrCode)
{
	debug("%s\n", __FUNCTION__);
}


bool appBootFinishedIsOkEnterOperational(CH_S)
{
	debug("%s\n", __FUNCTION__);
	return 1;
}

void appBootFinishedSuccessfully(CH_S)
{
	debug("%s\n", __FUNCTION__);
}

bool appIsNonMandatoryBootErrOk(CH uint8 nodeId, bootResultStates bootErr, uint32 nodeResponse)
{
	debug("%s\n", __FUNCTION__);
	return 1;
}

/*
 * End
 */

