/*
**                        Copyright 2001-2003 by KVASER AB
**                    P.O Box 4076 SE-51104 KINNAHULT, SWEDEN
**
**                     Copyright 2004-2014 by TK Engineering Oy
**                       P.O. Box 810, FI-65101 Vaasa, FINLAND
**                     E-mail: info@tke.fi, Web: http://www.tke.fi
**
** This software is furnished under a license and may be used and copied
** only in accordance with the terms of such license.
**
**
**  CANopen protocol handling rutines
*/

#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wparentheses-equality"
#endif

#include "bios/bios.h"
#include "bios/printd.h"

#include <stdlib.h>
#include <string.h> //for memset
#include "canopen.h"
#include "canopenM.h" /* For initating the CANopen master (nodeInfo) array... */
#include "coConfig.h"

#if TIME_H_IS_AVAILABLE
  #include <time.h>
#endif

/* This struct stores all communication objects 0x1000 to 0x1FFF */
typedef struct  {
  uint8 magicNumber; /* Used to verify integrity of data when storing to nvram - Is always set to 0x0D on store and 0xEE on restore */
  uint32 deviceType; /* 0x1000 - Device Type. Defines the Device Profile or Application Profile */
#if EMERGENCY_PRODUCER_HISTORY
  /* Implementation of objIdx 0x1003, p.9-65, DS301. */
  coEmcyProdHistEntr  emcyProdHistList[MAX_EMCY_PRODUCER_HISTORY_RECORDED];
#endif
  uint8 errorRegister; /* 0x1001 - Error Register. */
  uint32 syncCOBID; /* 0x1005 - COB-ID SYNC message. Defines the COB ID of SYNC messages and if this device generates SYNC messages */
  uint32 syncPeriod; /* 0x1006 - Communication Cycle Period. The period in microsecond of SYNC messages */
#if SYNC_WINDOW_LENGTH
  uint32 syncWindowLen; /* 0x1007 - Synchronous Window Length (in microseconds) */
#endif
  /* 0x1008-0x100A is read from the application */
  uint16 guardTime; /* 0x100C - Guard time (in milliseconds). Multiplied with life time factor this gives us the lifetime of the lifeguarding protocol. */
  uint8 lifeTimeFactor; /* 0x100D - Life time factor. Multiplied with guard time this gives us the lifetime of the lifeguarding protocol. */
  /*  uint32 timeStampCOBID;*/ /* 0x1012 - COB-ID time stamp object. Defines COB-ID for timestamps and if device generates or consumes time */
  uint32 emcyCOBID; /* 0x1014 - COB-ID EMCY. The COB-ID used when seding EMCY messages */ 
//  uint16 emcyInhibitTime; /* 0x1015 - Inhibit time EMCY (in units of 100microseconds) */
#if HEARTBEAT_CONSUMER
  HeartbeatNodeInfo heartbeatNodes[MAX_HEARTBEAT_NODES]; /* 0x1016 - Consumer Heartbeat Time. Heartbeat Consumer monitored nodes list */
#endif
  uint16 producerHeartBeatTime; /* 0x1017 - Producer Heartbeat Time (in milliseconds) */
#if IDENTITY_OBJECT
  uint32 vendorId;       /* Object 0x1018 : 1 Vendor Id. */
  uint32 productCode;    /* Object 0x1018 : 2 Product code. */
  uint32 revisionNumber; /* Object 0x1018 : 3 Revison number. */
  uint32 serialNumber;   /* Object 0x1018 : 4 serial number. */
#endif
  uint32 verCfgDate; /* Object 0x1020 : 1 Configuration date, DS301 p.11-3. */
  uint32 verCfgTime; /* Object 0x1020 : 2 Configuration date, DS301 p.11-3. */
#if EMERGENCY_MASTER || EMERGENCY_CONSUMER_OBJECT
  EmcyNode emcyNodes[MAX_EMCY_GUARDED_NODES]; /* 0x1028 - Emergency Consumer Object. List of guarded EMCY COB-IDs */
#endif
#if ERROR_BEHAVIOR 
  uint8 CommunicationError; /* Object 0x1029 : 1 Communication error, DS301, p125 */
#endif
  coSDO SDO[SDO_MAX_COUNT]; /* 0x1200 - 0x1300 - SDO Server and Client Parameters */
  coPDO PDO[PDO_MAX_COUNT]; /* 0x1400 - 0x15FF and 0x1800 - 0x19FF - RPDO/TPDO Communication Parameters */
#if DYNAMIC_PDO_MAPPING
  coPdoMapItem dynPdoMap[DYNAMIC_PDO_MAPPING_MAP_ITEMS]; /* 0x1600 - 0x17FF and 0x1A00 - 0x1BFF - RPDO/TPDO mapping parameters. PDO_MAP points here when using DYNAMIC_PDO_MAPPING */
#endif
  uint32              appSwDate;  /* Object 0x1f52 : 1 Appl software date */
  uint32              appSwTime;  /* Object 0x1f52 : 2 Appl software time */
  uint32              NMTStartup;  /* Object 0x1f80: on p.15, DS302. (CO_OBJDICT_NMT_STARTUP)*/
#if BOOTUP_DS302
  coNMTNetworkList    netWorkList[BOOTUP_MAX_COUNT]; /* Object 0x1f81-0x1f88  p.16 -> */
#endif
  uint32              NMTBootTime;  /* Object 0x1f89: on p.16 Boot time, default is 0. */
#if MULTIPLEXED_PDO
  /*   Implementation of multiplexed PDO, appendix A, DS301. */
  coDispListObjT dispList[MAX_DISP_LIST_ENTRIES];
  coScannerListObjT scanList[MAX_NUMBER_OF_SCAN_LISTS];
#endif
} CoComObjects;

/* This structure stores LSS data */
typedef struct {
	uint8 nodeId;
	CanBitRate bitRate;
} CoLssData;

/* This structure stores run-time data */
typedef struct {
  CoComObjects *comObjects; 
#if LSS_SLAVE
  CoLssData *lssData;
#endif
  CanChannel canChan;
  uint8 nodeId; /* Non-static so it can be seen from the demo GUI. */
  CanBitRate defaultBitrate;
  uint8 instanceIdx;  /* Tells the application which instance this is */
  coOpState opState;		/* The NMT State of the node */
#if LSS_SLAVE
  uint8 lssState;  /* State of LSS state machine */
#endif
  uint8 nodeGuardToggle;	/* The state of the toggle bit in sent life guarding / heartbeat messages */
  KfHalTimer lastNodeGuardRequest; /* Records when the last node guarding request (heartbeat RTR) was received */
#if LIFE_GUARDING
  bool lifeGuardingIsAlive;
#endif
  const coPdoMapItem *pdoMap; /* Static PDO map. PDO_MAP points here when not using DYNAMIC_PDO_MAPPING */
  const coObjDictEntry *OD;
  const coObjDictEntry *ODapp;
  KfHalTimer intProducerHeartbeatTime; /* Copy of producerHeartBeatTime in ticks */
  KfHalTimer lastHeartBeatTime; /* Time of last heart beat message we transmitted. */
  uint8 heartBeatQueued;	/* If non-zero, a heartbeat message has been queued. */
  KfHalTimer lastSyncTime; /* Time of last SYNC message we transmitted. */
#if SYNC_WINDOW_LENGTH
  KfHalTimer syncRecvTime;  /* Time of last SYNC message received. Used to calculate when sync window ends */
#endif
  uint8 syncQueued; /* If non-zero, a sync message has been queued. */
  cobIdT intEmcyCOBID; /* Should mirror value of emcyCOBID */
  cobIdT intSyncCOBID; /* Should mirror value of syncCOBID. */
  KfHalTimer intSyncPeriod;
#if NODEGUARD_MASTER
  NodeguardingNodeInfo guardedNodes[MAX_GUARDED_NODES]; /* Nodeguarding protocol monitored nodes list */
#endif
  KfHalTimer          intNMTBootTime;  /* Mirroring NMTBootTime - but in the "ticks" unit */
  KfHalTimer          bootStartTimeStamp;
  int                 networkListIdxBootNode;
  bool                isInstanceBooting;
  uint16 overrunCnt;   /* Overrun error counter */
} CoRuntimeData;

static CoRuntimeData coRuntime[CO_INST_MAX];
static CoComObjects coComObjects[CO_INST_MAX];
#if LSS_SLAVE
static CoLssData coLssData[CO_INST_MAX];
#endif
#if CO_MULTIPLE_INSTANCES
  static uint8 coInstancesInUse[CO_INST_MAX];
#endif


CanChannel coGetCanChannel(CH_S) {return RV(canChan);} /* Needed by e.g. canopenM.c. Not declared in canopen.h. */

/* Make sure there is room for the Predefined Master/Slave Connection Set */
/*CompilerAssert(PDO_MAX_COUNT >= 8);*/
CompilerAssert(SDO_MAX_COUNT >= 1);


/* The tabulated Object Dictionary */
/* The different variable descriptions */
static coObjDictVarDesc
   odeDeviceType      = {CANOPEN_DEVICE_TYPE, NULL, 4, CO_OD_ATTR_READ, 0}, /*lint !e785   Too few initializers for aggregate */
   odeErrorRegister   = {0, NULL, 1, CO_OD_ATTR_READ, 0},     /*lint !e785   Too few initializers for aggregate */
   odeSyncCOBID       = {0x00000080, NULL, 4, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE|CO_OD_WRITE_NOTIFY|CO_OD_WRITE_VERIFY, 0},  /*lint !e785   Too few initializers for aggregate */
   odeSyncPeriod      = {0, NULL, 4, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE|CO_OD_WRITE_NOTIFY, 0}, /*lint !e785   Too few initializers for aggregate */
   odeSyncWindowLen   = {0, NULL, 4, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE|CO_OD_WRITE_NOTIFY, 0}, /*lint !e785   Too few initializers for aggregate */ 
#if LIFE_GUARDING
   odeGuardTime       = {0, NULL, 2, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE, 0}, /*lint !e785   Too few initializers for aggregate */
   odeLifteTimeFactor = {0, NULL, 1, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE, 0}, /*lint !e785   Too few initializers for aggregate */
#else
   odeGuardTime       = {0, NULL, 2, CO_OD_ATTR_READ, 0}, /*lint !e785   Too few initializers for aggregate */
   odeLifteTimeFactor = {0, NULL, 1, CO_OD_ATTR_READ, 0}, /*lint !e785   Too few initializers for aggregate */
#endif
/*   odeTimeStampCOBID  = {0x100, NULL, 4, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE|CO_OD_READ_NOTIFY|CO_OD_WRITE_NOTIFY, 0},*/ /*lint !e785   Too few initializers for aggregate */
   odeEmcyCOBID       = {cobPDOinvalidP, NULL, 4, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE|CO_OD_WRITE_NOTIFY|CO_OD_WRITE_VERIFY, 0}, /*lint !e785   Too few initializers for aggregate */
                                                                                                                     /* Default value should be assigned at comm. reset */
/*   odeEmcyInhibitTime = {0, NULL, 2, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE, 0},*/ /*lint !e785   Too few initializers for aggregate */
/*odeConsumerHeartBeatTime = {0, 4, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE|CO_OD_ATTR_VECTOR, 0},*/
   odeNMTStartup = {0, NULL, 4, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE|CO_OD_READ_NOTIFY, 0},
   odeProducerHeartBeatTime = {0, NULL, 2, CO_OD_ATTR_READ|CO_OD_ATTR_WRITE|CO_OD_WRITE_NOTIFY, 0}; /*lint !e785   Too few initializers for aggregate */

/* --- To be able to support multiple instances without wasting RAM memory we have to do 4 (similar) arrays except
       for the coComObjects-index. */


#define INIT_OD_ENTRY(n) \
    {CO_OBJDICT_DEVICE_TYPE, 0, &(coComObjects[n].deviceType), &odeDeviceType, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT32)},                                                   \
    {CO_OBJDICT_ERROR_REGISTER, 0, &(coComObjects[n].errorRegister), &odeErrorRegister, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT8)},                                              \
    {CO_OBJDICT_SYNC_COBID, 0, &(coComObjects[n].syncCOBID), &odeSyncCOBID, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT32)},                                      \
    {CO_OBJDICT_SYNC_PERIOD, 0, &(coComObjects[n].syncPeriod), &odeSyncPeriod, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT32)},                                   \
    {CO_OBJDICT_SYNC_WINDOW_LEN, 0, &(coComObjects[n].syncWindowLen), &odeSyncWindowLen, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT32)},                                   \
    {CO_OBJDICT_GUARD_TIME, 0, &(coComObjects[n].guardTime), &odeGuardTime, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT16)},                                                     \
	{CO_OBJDICT_LIFETIME_FACTOR, 0, &(coComObjects[n].lifeTimeFactor), &odeLifteTimeFactor, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT8)},                                           \
/*	{CO_OBJDICT_TIMESTAMP_COBID, 0, &(coComObjects[n].timeStampCOBID), &odeTimeStampCOBID, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT32)},*/                                     \
    {CO_OBJDICT_EMCY_COBID, 0, &(coComObjects[n].emcyCOBID), &odeEmcyCOBID, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT32)},                                      \
/*    {CO_OBJDICT_EMCY_INHIBIT_COBID, 0, &(coComObjects[n].emcyInhibitTime), &odeEmcyInhibitTime, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT16)},              */    \
    {CO_OBJDICT_PRODUCER_HEARTBEAT_TIME, 0, &(coComObjects[n].producerHeartBeatTime), &odeProducerHeartBeatTime, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT16)}, \
    {CO_OBJDICT_NMT_STARTUP, 0, &(coComObjects[n].NMTStartup), &odeNMTStartup, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_UINT32)}, \
    {0, 0, NULL, NULL, 0} /* Marks the end */


static const coObjDictEntry OD[CO_INST_MAX][11] = {
#if CO_INST_MAX > 0
   {INIT_OD_ENTRY(0)}  /*lint !e785   Too few initializers for aggregate */
#endif
#if CO_INST_MAX > 1
  , {INIT_OD_ENTRY(1)} /*lint !e785   Too few initializers for aggregate */
#endif
#if CO_INST_MAX > 2
  , {INIT_OD_ENTRY(2)} /*lint !e785   Too few initializers for aggregate */
#endif
#if CO_INST_MAX > 3
  , {INIT_OD_ENTRY(3)} /*lint !e785   Too few initializers for aggregate */
#endif
};


/* --- Tag values used when transmitting CAN messages. See comment at coWriteMessage. */
#define TAG_HEARTBEAT 0x80000001
#define TAG_SYNC      0x80000002

/*STATIC EmcyNode emcyNodes[MAX_EMCY_GUARDED_NODES];*/

/* -- Internal additional functionality -- */
#define PDO_MAP_COBID_CHECK 1

/* --- Function prototypes */
static const coObjDictEntry *odFind(CH uint16 objIdx);
static CoObjLength locGetObjectLen(CH uint16 objIdx, uint8 subIdx, coErrorCode *err);
static uint32 locReadObject(CH uint16 objIdx, uint8 subIdx, coErrorCode *err);
static uint32 locReadObjectEx(CH uint16 objIdx, uint8 subIdx, coErrorCode *err);
static coErrorCode locReadObjectP(CH uint16 objIdx, uint8 subIdx, uint8 *buf, CoObjLength pos, uint8 *(valid), uint8 len);
static uint32 readDataType(uint16 objIdx, uint8 subIdx, coErrorCode *err);
static coErrorCode locWriteObject(CH uint16 objIdx, uint8 subIdx, uint32 val);
static coErrorCode locWriteObjectP(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len);
//static coErrorCode locWriteObjectPEx(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len);
static coErrorCode locWriteObjectEx(CH uint16 objIdx, uint8 subIdx, uint32 val);
static coErrorCode coODWriteNotify(CH uint16 objIdx, uint8 subIdx, uint16 notificationGroup);
static coErrorCode coODReadNotify(CH uint16 objIdx, uint8 subIdx, uint16 notificationGroup);
static coErrorCode coODWriteVerify(CH uint16 objIdx, uint8 subIdx, uint32 val);
static void handleNMT(CH coNMTfunction cmd);
static void handleNMTNodeGuarding(CH_S);
static void handleSyncEvent(CH_S);
static CoStackResult transmitHeartbeatMessage(CH bool isBootMsg);
static void handlePDO(CH int i, uint8 *buf, uint8 dlc);
static void handlePDORequest(CH int i);
static void handleSDO(CH int i, uint8 *buf, uint8 dlc);
static CoStackResult transmitPDO(CH int i);   /* qqq */
static void coError(CH uint16 errType, uint8 pos, uint16 param1, uint8 param2, uint32 param3);
/*static void coEmergencyError(CH uint16 errCode); BIGFIX 041104: This one moved to canopen.h.*/
#if BOOTUP_DS302
static int findNodeIdIdxInNodeInfo(CH uint8 nodeId);  // May produce a warning depending on stack configuration
static uint16 readHeartbeatTime(CH uint8 nodeId); // May produce a warning depending on stack configuration
static bool isHeartbeatReceived(CH uint8 nodeId); // May produce a warning depending on stack configuration
static bool isHeartbeatTimeouted(CH uint8 nodeId);  // May produce a warning depending on stack configuration
#endif
static coErrorCode odFindObjP(CH uint16 objIdx, uint8 subIdx, uint8 **objP, coObjDictVarDesc *varP);
static coErrorCode handleReceivedDataInSegmentedSDO(CH int i, uint8 *bufP);

coErrorCode constructTransmitDataInSegmentedSDO(CH int i, uint8 *canDataP, uint8 *seg_n, CoSegTranAction *transferStat);

#if DYNAMIC_PDO_MAPPING
static CoStackResult resetDynMapList(CH_S);
static coErrorCode writeDynamicMapPDO(CH uint16 objIdx, uint8 subIdx, uint32 val);
/*coErrorCode coRemapDirectPDO(CH int pdoMapIdx, int pdoIdx);*/
static coErrorCode coReCalcAndRemapDirectPDO(CH int pdoMapIdx, int pdoIdx);
#endif

#if IDENTITY_OBJECT
  static uint32 readIdentityObject(CH uint16 objIdx, uint8 subIdx, coErrorCode *err);
  static coErrorCode writeIdentityObject(CH uint16 objIdx, uint8 subIdx, uint32 val, bool overrideReadOnly);
#endif

#if EMERGENCY_PRODUCER_HISTORY
  static uint32 readNodeEmcyHist(CH uint8 subIdx, coErrorCode *err);
  static coErrorCode writeNodeEmcyHist(CH uint8 subIdx, uint32 val);
#endif

#if EMERGENCY_CONSUMER_OBJECT
  static int findEmcyListIdx (CH uint8 nodeId);
  static uint32 readEmcyConsObj(CH uint8 subIdx, coErrorCode *err);
  static coErrorCode writeEmcyConsObj(CH uint8 subIdx, uint32 val);
#endif


#if HEARTBEAT_CONSUMER
static coErrorCode writeConsumerHeartbeatTime(CH uint8 subIdx, uint32 val);
static uint32 readConsumerHeartbeatTime(CH uint8 subIdx, coErrorCode *err);
static void handleHeartbeat(CH uint8 mNodeId, coOpState mState);
#endif

#if NODEGUARD_MASTER
static void handleNodeguardMessage(CH uint8 mNodeId, uint8 mToggle, coOpState mState);
#endif


#if BOOTUP_DS302
/*
*   Functions to setup and read the nodeguardning settings via the object
*   dictionaty.
*/
static coErrorCode writeNMTStartupSettings(CH uint16 objIdx, uint8 subIdx, uint32 val);
static uint32 readNMTStartupSettings(CH uint16 objIdx, uint8 subIdx, coErrorCode *err);
static int findNetworkListNodeIdC(CH uint8 nodeId);
static int findNetworkListNodeId(CH uint8 nodeId);
static CoStackResult resetNodeByNode(CH_S);  /* qqq: To do. Not required by Sandvik! */
static CoStackResult resetAllNodes(CH_S);
static bool handleReceivedBootData(CH int nodeId, uint16 objIdx, uint8 subIdx, uint32 val, bool isErrorData);
static void superviseBootDS302(CH_S);
static CoStackResult initBackgroundBootParamDS302(CH_S);
static coErrorCode connectClientSdoToNodeId(CH int clientSdoNo, int nodeId);
#endif

#if MULTIPLEXED_PDO
static bool isSrcObjFoundInDispatcherList(CH int dispLstIdx, uint16 remObjIdx, uint8 remSubIdx, uint8 remNodeId, uint16 *locObjIdx, uint8 *locSubIdx);
static coErrorCode writeScannerList(CH uint16 objIdx, uint8 subIdx, uint32 val);
static bool isObjInScannerList(CH uint16 objIdx, uint8 subIdx);
static uint32 readScannerList(CH uint16 objIdx, uint8 subIdx, coErrorCode *coErr);
static bool deleteScanListSubMemIdx(CH int scListIdx, uint8 subIdx);
static int findScanListSubMemIdx(CH int scListIdx, uint8 subIdx, bool creaIfNotFnd);
static int findScanListNumMemIdx(CH uint8 scanListNo, bool creaIfNotFnd);
static int findScanListObjMemIdx(CH uint16 objIdx, bool creaIfNotFnd);
static CoStackResult extractDataFromSAMPDO(coSamPdoFrameDataT *resP, uint8 *buf, uint8 dlc);
static CoStackResult extractDataFromMPDO (coMPDOFrameDataT *mpdoP, uint8 *buf, uint8 dlc);
static CoStackResult handleMPDO (CH uint8 *buf, uint8 dlc);
static bool isSamPdoInDispatcherList(CH coMPDOFrameDataT samPDO, uint16 *locObjIdx, uint8 *locSubIdx);
static CoStackResult transmitSAMPDO (CH uint16 locObjIdx, uint8 locSubIdx, uint32 val);
static coErrorCode writeDispatcherList(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len);
static coErrorCode readDispatcherList(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len);
static int findSAMPDO(CH uint8 dir);
static CoStackResult extrDispListVarsFrBuf(dispListDataT *resP, uint8 *buf, uint8 len);
static CoStackResult packDispListVarsToBuf(dispListDataT disLstData, uint8 *buf, uint8 len);
#endif
static coErrorCode createPdoMapSpaceInList(CH int startIdx, int space);
static int findMapListEndIdx(CH int startSearchIdx);
static int findPdoIdxForMapParms(CH uint16 objIdx, bool createPdoIfNotFound, bool createMapIfNotExist, coErrorCode *err);

#if STORE_PARAMS 
static coErrorCode writeStoreParamObj(CH uint8 subIdx, uint32 val);
static uint32 readStoreParamObj(CH uint8 subIdx, coErrorCode *err);
static bool saveCommParams(CH_S);
static bool loadCommParams(CH_S);
static bool saveODParams(CH uint8 area, uint16 lowIndex, uint16 highIndex);
static bool loadODParams(CH uint8 area, uint16 lowIndex, uint16 highIndex);
#endif

#if RESTORE_PARAMS
static coErrorCode writeRestoreParamObj(CH uint8 subIdx, uint32 val);
static uint32 readRestoreParamObj(CH uint8 subIdx, coErrorCode *err);
#endif

#if ERROR_BEHAVIOR
static coErrorCode writeErrorBehaviorObj(CH uint8 subIdx, uint8 val);
static uint8 readErrorBehaviorObj(CH uint8 subIdx, coErrorCode *err);
#endif


#if TIME_H_IS_AVAILABLE
static bool updateConfigDate(CH_S);
static bool updateConfigTime(CH_S);
#endif

#if LSS_SLAVE
	static void lssSetState(CH uint8 state);
	static int lssProcessMessage(CH uint32 cobId, uint8 *buf, uint8 dlc);
	static int lssSetNodeId(CH uint8 nodeId);
	static void lssSend(CH uint8 errorCode, uint8 specError, uint8 lssError);
	static bool lssStoreParams(CH_S);
	static bool lssLoadParams(CH_S);
#endif

#if USE_EXTENDED_CAN
  #define long2cob(p) (p)
  #define cob2long(c) (c)
#else
  static cobIdT long2cob(uint32 p);
  static uint32 cob2long(cobIdT cobid);
#endif

static uint32 buf2val(uint8 *buf, uint8 len);
static void val2buf(uint32 val, uint8 *buf, uint8 len);
#if BIG_ENDIAN
  static uint32 getVal(uint8 *objPtr, uint8 len);
  static void storeVal(uint32 val, uint8 *objPtr, uint8 len);
#else
  #define getVal(objPtr, len) buf2val(objPtr, len)
  #define storeVal(val, objPtr, len) val2buf(val, objPtr, len)
#endif

static void abortTransferSDO(CH int i, uint16 objIdx, uint8 subIdx, coErrorCode err);
static unsigned char coCanCallback(CanChannel chan, CanCbType cbType, const CANMessage *msg);
static int isRestrictedCanId( cobIdT cobId);
static void updateUsedCobIds( CH_S);

coErrorCode coGetErrorCount(CH uint16 *txErr, uint16 *rxErr, uint16 *ovErr){
  CanHalCanStatus status = canGetErrorCounters(RV(canChan), txErr, rxErr, ovErr);
  if(status == CANHAL_OK)
    return CO_OK;
  else
    return CO_ERR_GENERAL;
}

/* Checks if a COBID is restricted according to CiA 301, v4.2.0.2, section 7.3.5. According
* to the CiA 310 "Such a restricted CAN-ID shall not be used as a CAN-ID by any
* configurable communication object, neither for SYNC, TIME, EMCY, PDO, and SDO."
*
* Returns 1 if the COB id is reserved or 0 otherwise
*/
static int isRestrictedCanId( cobIdT cobId){
	cobId = cobId & (~(cobPDOinvalid + cobRTRdisallowed + cobExtended));
	if( cobId == 0  // NMT
		|| (cobId >= 0x1 && cobId <= 0x7F) 		// reserved
		|| (cobId >= 0x101 && cobId <= 0x180)	// reserved
		|| (cobId >= 0x581 && cobId <= 0x5FF)	// default SDO (tx)
		|| (cobId >= 0x601 && cobId <= 0x67F)	// default SDO (rx)
		|| (cobId >= 0x6E0 && cobId <= 0x6FF)	// reserved
		|| (cobId >= 0x701 && cobId <= 0x77F)	// NMT Error Control
		|| (cobId >= 0x780 && cobId <= 0x7FF)){	// reserved
		return 1;
	}
	return 0;
}


void coInitLibrary(void){
#if CO_MULTIPLE_INSTANCES  
  int i = 0;
#endif
  canInitialize();
#if CO_MULTIPLE_INSTANCES
  while(i < CO_INST_MAX){
    coInstancesInUse[i]=0;
    i++;
  }
#endif
} /*lint !e529   Symbol 'i' (line 241) not subsequently referenced */

/* Searches for the PDO numbered pdoNo (1..512) (and of correct rx/tx type) in the PDO-table;
* if it is found, all entries are set to default and COBID is assigned;
* otherwise a new record is allocated, if available.
* cobId might have the cobRTRdisallowed and the cobExtended bits set.
* Returns the index into PDO upon success, -1 otherwise.
*/
static int initPDO(CH uint16 pdoNo, uint8 dir, cobIdT cobId) {
  int i, ii = -1;
  /* We scan from the end so we, in case of a new record, we get the lowest numbered free slot. */
  for (i = PDO_MAX_COUNT-1; i >= 0; i--)
    if (CV(PDO)[i].dir == PDO_UNUSED)
      ii = i;
    else if (CV(PDO)[i].dir == dir && CV(PDO)[i].pdoNo == pdoNo) {
      ii = i;
      break;
    }
  if (ii >= 0) {
    CV(PDO)[ii].status = 0;  /*clear all statusflags */
    CV(PDO)[ii].pdoNo = pdoNo;
    CV(PDO)[ii].dir = dir;
    CV(PDO)[ii].COBID = cobId;
    CV(PDO)[ii].transmissionType = CO_PDO_TT_DEFAULT; /* Device Profile dependent */
	CV(PDO)[ii].pdoSyncCntr = 0;
    CV(PDO)[ii].inhibitTime = 0;
    CV(PDO)[ii].eventTimer = 0;
    CV(PDO)[ii].mapIdx = PDO_MAP_NONE;
#if CO_DIRECT_PDO_MAPPING
    CV(PDO)[ii].dmFlag = 0;
#endif
    return ii;
  } else
    return -1;
}

/* Searches for a record in PDO[] with the asked-for pdoNo and direction.
* Returns the index, or -1 of not found.
*/
static int findPDO(CH uint16 pdoNo, uint8 dir) {
  int i;
  for (i = 0; i < PDO_MAX_COUNT; i++)
    if (CV(PDO)[i].dir == dir && CV(PDO)[i].pdoNo == pdoNo)
      return i;
  return -1;
}

/* Searches for a record in PDO[] with the asked-for pdoNo and direction.
* Returns the index, or creates a new entry if needed.
* Returns -1 if there is no more room.
* If a new item is created, two serches of PDOs are made.
*/
static int findPDOc(CH uint16 pdoNo, uint8 dir) {
  int i = findPDO(CHP pdoNo, dir);
  if (i < 0)
    i = initPDO(CHP pdoNo, dir, cobPDOinvalid);
  return i;
}

/* If the PDO at index i is unused, mark it as such.
* We remove entries with an invalid COBID and no mapping.
* When writing to a PDO, the COBID has to be assigned before any other fields.
* Possibly, we should look also on other fields.
*/
static void packPDO(CH int i) {
  if (CV(PDO)[i].dir != PDO_UNUSED) {
    if ((CV(PDO)[i].COBID & cobPDOinvalid) && (CV(PDO)[i].mapIdx == PDO_MAP_NONE))
      CV(PDO)[i].dir = PDO_UNUSED;
  }
}

/* Searches for the SDO numbered sdoNo in the SDO-table;
* if it is found, all entries are set to default and the COBID's are assigned;
* otherwise a new record is allocated, if available.
* cobId might have the cobExtended bit set.
* Returns the index into SDO[] upon success, -1 otherwise.
*/
static int initSDO(CH uint8 sdoNo, uint8 type, cobIdT cobIdRx, cobIdT cobIdTx) {
  int i, ii = -1;
  /* We scan from the end so we, in case of a new record, we get the lowest numbered free slot. */
  for (i = SDO_MAX_COUNT-1; i >= 0; i--)
    if (CV(SDO)[i].type == SDO_UNUSED)
      ii = i;
    else if (CV(SDO)[i].type == type && CV(SDO)[i].sdoNo == sdoNo) {
      ii = i;
      break;
    }
  if (ii >= 0) {
    CV(SDO)[ii].sdoNo = sdoNo;
    CV(SDO)[ii].type = type;
    CV(SDO)[ii].COBID_Rx = cobIdRx;
    CV(SDO)[ii].COBID_Tx = cobIdTx;
    CV(SDO)[ii].otherNodeId = 0;
    CV(SDO)[ii].status = 0;
    return ii;
  } else
    return -1;
}

/* This function returns the requested SDO-number (1-128 SDO) */
/* If 0 is return the requested SDO was not found. */

uint8 coGetSdoNo(CH uint8 otherNodeId, uint8 type) {
  int i;
  for (i = 0; i < SDO_MAX_COUNT; i++) {
    if (CV(SDO)[i].type == type && CV(SDO)[i].otherNodeId == otherNodeId)
      return(CV(SDO)[i].sdoNo);
  }
  return 0; /* SDO not found. */
}

/* Searches for a record in SDO[] with the asked-for sdoNo.
* Bothe the COBID's must be marked as valid for a entry to be considered valid.
* Returns the index, or -1 of not found.
*/
static int findSDO(CH uint8 sdoNo, uint8 type) {
  int i;
  for (i = 0; i < SDO_MAX_COUNT; i++) {
    if (CV(SDO)[i].type == type && CV(SDO)[i].sdoNo == sdoNo)
      return i;
  }
  return -1;
}

/* Searches for a record in SDO[] with the asked-for sdoNo and direction.
* Returns the index, or creates a new entry if needed.
* Returns -1 if there is no more room.
* If a new item is created, two serches of SDOs are made.
*/
static int findSDOc(CH uint8 sdoNo, uint8 type) {
	int i = findSDO(CHP sdoNo, type);
	if (i < 0){
		i = initSDO(CHP sdoNo, type, cobPDOinvalid, cobPDOinvalid);
	}
	return i;
}

/* If the SDO at index i is unused, mark it as such.
* We remove entries with invalid COBIDs.
* When writing to a SDO, the COBIDs have to be assigned before otherNodeId.
*/
static void packSDO(CH int i) {
  if (CV(SDO)[i].type != SDO_UNUSED) {
    if ((CV(SDO)[i].COBID_Rx & cobPDOinvalid) && (CV(SDO)[i].COBID_Tx & cobPDOinvalid))
      CV(SDO)[i].type = SDO_UNUSED;
  }
}

/*
*  This function converts between CAN HAL errors and CanOpen-stack error codes
*/

CoStackResult getCanHalError(CanHalCanStatus stat){
  switch (stat) {
    case CANHAL_OK:
      return(coRes_OK);
    case CANHAL_ERR_PARAM:
      return(coRes_ERR_CAN_PARAM);
    case CANHAL_ERR_NOMSG:
      return(coRes_ERR_CAN_NOMSG);
    case CANHAL_ERR_NOTFOUND:
      return(coRes_ERR_CAN_NOTFOUND);
    case CANHAL_ERR_TIMEOUT:
      return(coRes_ERR_CAN_TIMEOUT);
    case CANHAL_ERR_NOTINITIALIZED:
      return(coRes_ERR_CAN_NOTINITIALIZED);
    case CANHAL_ERR_OVERFLOW:
      return(coRes_ERR_CAN_OVERFLOW);
    case CANHAL_ERR_HARDWARE:
      return(coRes_ERR_CAN_HARDWARE);
    default:
      return(coRes_ERR_CAN_OTHER);
  } /*lint !e788   enum constant 'Symbol' not used within defaulted switch */
}



/* -----------------------------------------------------------------------------
* CANopen boot-up.
* We support only the minimum boot-up process.
* Returns 0 in case of success.
* In case of error, the CAN channel is closed (if it could be opened).
*/

#if CO_MULTIPLE_INSTANCES
  CoHandle
#else
  CoStackResult
#endif
        coInit(CanChannel chan, CanBitRate btr, uint8 nodeId, const coObjDictEntry *ODappI, const coPdoMapItem *pdoMapI){

		CoStackResult result;
#if CO_MULTIPLE_INSTANCES
  /* Locate a free slot in coInternals[] */
  int chIdx;
  CoRuntimeData *ch;
  for (chIdx = 0; chIdx < CO_INST_MAX; chIdx++){
	  if( coInstancesInUse[chIdx] && coRuntime[chIdx].canChan == chan){
		  return (NULL);  /* channel can only be used once! */
	  }
  }
  for (chIdx = 0; chIdx < CO_INST_MAX; chIdx++){
	  if (!coInstancesInUse[chIdx]){
		  break;
	  }
  }
  if (chIdx >= CO_INST_MAX){
    return NULL;
  }
  coInstancesInUse[chIdx] = 1;
  ch = &coRuntime[chIdx];
  memset( &coComObjects[chIdx], 0, sizeof(CoComObjects));
  memset( ch, 0, sizeof(CoRuntimeData));
  coRuntime[chIdx].comObjects = &coComObjects[chIdx];

#if LSS_SLAVE
  memset( &coLssData[chIdx], 0, sizeof(CoLssData));
  coRuntime[chIdx].lssData = &coLssData[chIdx];
#endif

#else
  memset(&coComObjects[0], 0, sizeof(CoComObjects));
  memset(&coRuntime[0], 0, sizeof(CoRuntimeData));
  coRuntime[0].comObjects = &coComObjects[0];

#if LSS_SLAVE
  memset(&coLssData[0], 0, sizeof(CoLssData));
  coRuntime[0].lssData = &coLssData[0];
#endif

#endif

  // Enter state Initialisation at Power on and Hardware Reset
  // Entering sub-state Initialising
  RV(opState) = coOS_Initialising;
  RV(canChan) = chan;
  RV(nodeId) = nodeId;
  RV(defaultBitrate) = btr; /* Store default bitrate in case no LSS or LSS memory bad */
#if CO_MULTIPLE_INSTANCES
  RV(instanceIdx) = (uint8)chIdx;
#endif

  /* Save the initial PDO map. If we want to support dynamic PDO mapping,
   * we should make a local copy (and return to it in case of a
   * ResetCommunicaion event).
   */
  RV(pdoMap) = pdoMapI;
  RV(ODapp) = ODappI;
#if CO_MULTIPLE_INSTANCES
  RV(OD) = OD[chIdx];
#else
  RV(OD) = OD[0];
#endif

  // We set the default values
  // Enter sub-state Reset_Application
  result = coResetNode(CHP_S);
  if (result != coRes_OK) {
    if(canShutdown(RV(canChan))!=CANHAL_OK)
#if CO_MULTIPLE_INSTANCES
      return(NULL);
#else
      return(coRes_ERR_CAN_SHUT_DOWN);
#endif
#if CO_MULTIPLE_INSTANCES
    return(NULL);
#else
    return (coRes_ERR_RESET_COMM);
#endif
  }
#if CO_MULTIPLE_INSTANCES
//  printPDOmap(CHP_S);
  return (ch);
#else
  return (coRes_OK);
#endif

}

CoStackResult coExit(CH_S) {
#if CO_MULTIPLE_INSTANCES
  coInstancesInUse[RV(instanceIdx)] = 0;
#endif

  if(canShutdown(RV(canChan))!=CANHAL_OK)
    return(coRes_ERR_CAN_SHUT_DOWN);
  return(coRes_OK);
}

/* Assigns all OD entries the default value as seen in the table. */
static void resetOD(CH const coObjDictEntry *ode) {
  // Object directory not available
  if (!ode)
    return;

  while (ode->objIdx) {
    const coObjDictVarDesc *objDesc = ode->objDesc;
    if (ode->subIdxCount == 0) {
      // Target variable to write to is a simple variable.
	  if( objDesc->attr & CO_OD_ATTR_STRING) {
        if (objDesc->defValStrOnly != NULL) {
          uint8 len = (uint8)min(objDesc->size, strlen((const char*)objDesc->defValStrOnly));
          // Clear the old value
          memset((uint8*)ode->objPtr, 0, objDesc->size);

          memcpy((uint8*)ode->objPtr, objDesc->defValStrOnly, len);
        }
	  } else if( objDesc->size > 4){
        if (objDesc->defValStrOnly != NULL) {
          memcpy((uint8*)ode->objPtr, objDesc->defValStrOnly, objDesc->size);
        }
	  } else {
        // The object is not a string and <= 32 bit.
        storeVal(objDesc->defVal, (uint8*)ode->objPtr, (uint8)objDesc->size);
      }
    } else {
      // Target variable to write to is a array/vector variable.
      uint8 i;
      uint8 *objPtr = (uint8*)ode->objPtr;
      for (i=0; i < ode->subIdxCount; i++) {
		  if( objDesc->attr & CO_OD_ATTR_STRING) {
			  if (objDesc->defValStrOnly != NULL) {
				  uint8 len = (uint8)min(objDesc->size, strlen((const char*)objDesc->defValStrOnly));
				  // Clear the old value
				  memset((uint8*)objPtr, 0, objDesc->size);

				  memcpy((uint8*)objPtr, objDesc->defValStrOnly, len);
			  }
		  } else if( objDesc->size > 4){
			  if (objDesc->defValStrOnly != NULL) {
				  memcpy((uint8*)ode->objPtr, objDesc->defValStrOnly, objDesc->size);
			  }
		  } else {
			  // The object is not a string and <= 32 bit.
			  storeVal(objDesc->defVal, (uint8*)objPtr, (uint8)objDesc->size);
		  }
        objPtr += objDesc->size;
		if( objDesc->attr & CO_OD_ATTR_VECTOR){
			continue;
		} else {
		  objDesc++;
		}
//		if( CO_ODOBJCODE( ode->structure) == CO_ODOBJ_RECORD){
//        }
      }
    } 
    ode++;
  }
}

/* Called when the NMT command ResetNode is received.
*/
CoStackResult coResetNode(CH_S) {
  RV(opState) = coOS_Initialising;

#if LSS_SLAVE
  lssSetState(CHP LSSfinal);
  if( lssLoadParams(CHP_S) == false){
	LSSV(nodeId) = 0xFF;
	LSSV(bitRate) = RV(defaultBitrate);
  }
#endif
  
  resetOD(CHP RV(ODapp));

#if STORE_PARAMS
  /* Load application and manufacturer specific parts of OD from NVRAM */
  loadODParams(CHP CO_STORE_MANUF_PARAMS_MIN, 0x2000, 0x5FFF);
  loadODParams(CHP CO_STORE_APPL_PARAMS, 0x6000, 0x9FFF);
#endif

  // Enter sub-state Reset_Communication
  return coResetCommunication(CHP_S);
}

/* Reset the communication objects
* Returns 0 in case of success.
*/
CoStackResult coResetCommunication(CH_S) {
  int i;
  bool parametersLoaded = false;
  CanHalCanStatus stat;

  RV(opState) = coOS_Initialising;

#if LSS_SLAVE
  lssSetState(CHP LSSinit);
  if( LSSV(nodeId) > 0 && LSSV(nodeId) < 128){
	  RV(nodeId) = LSSV(nodeId);
	  }
#endif

  /* printf("coResetCommunication()\n"); */
  /* The parameters of the communication profile area are set to
  * their power-on values.
  */
  if (RV(nodeId) == 0 || RV(nodeId) > 127)
    return(coRes_ERR_RESET_NODE_ID);
  
  RV(nodeGuardToggle) = 0;
  RV(heartBeatQueued) = 0;
  
#if STORE_PARAMS
  /* Load com settings 0x1000-0x1FFF from non-volatile memory, if available */
  parametersLoaded = loadCommParams(CHP_S);
#endif

  /* Load default com settings, if loading from non-volatile memory failed */
  if( parametersLoaded == false) {
	memset( RV(comObjects), 0, sizeof(CoComObjects));

    /* Initiate the PDO vectors. We do not initiate pdoNo or mapNo as the whole
    * record is considered undefined if the cobPDOinvalid bit is set in COBID. */
    for (i = 0; i < PDO_MAX_COUNT; i++)
      CV(PDO)[i].dir = PDO_UNUSED;
    for (i = 0; i < SDO_MAX_COUNT; i++)
      CV(SDO)[i].type = SDO_UNUSED;
    
    /* Fill in data for the Predefined Master/Slave Connection Set */
    for (i = 0; i < 4; i++) {
      if (i < PDO_MAX_COUNT/2) {
        (void)initPDO(CHP (uint16)(1+i), PDO_RX, (uint16)(0x200+i*0x100+RV(nodeId)));
        (void)initPDO(CHP (uint16)(1+i), PDO_TX, (uint16)(0x180+i*0x100+RV(nodeId)));
      }
    }
  
    // Reset the parameters of the communication profile area
    resetOD(CHP RV(OD));

#if EMERGENCY_MASTER || EMERGENCY_CONSUMER_OBJECT
    resetEmcyCntrlList(CHP_S);  /* clears the COBIDs from where we should control COBIDs */
#endif
    
    CV(emcyCOBID) = cob2long(0x80+RV(nodeId));
    
#if DYNAMIC_PDO_MAPPING
    if( resetDynMapList(CHP_S) != coRes_OK)
      return(coRes_ERR_DYNAMIC_MAP_RESET);
#endif
    
    /* Scan the PDO map, update or create PDOs */
    if (PDO_MAP) {  /*lint !e774   Boolean within 'if' always evaluates to True */
      const coPdoMapItem *m = PDO_MAP;
      while (m->len != CO_PDOMAP_END) {
        uint8 dir;
        uint16 pdoNo = m->objIdx;
        int pdoLen;
        if (m->len == CO_PDOMAP_RX)
          dir = PDO_RX;
        else
          dir = PDO_TX;
        i = findPDOc(CHP pdoNo, dir);
        if (i < 0) {
          coError(CHP LOC_CO_ERR_PDO, 1, (uint16)i, 0, 0);
          return(coRes_ERR_DEFAULT_PDO_MAP_SIZE);
        } else {
#if CO_DIRECT_PDO_MAPPING
          uint8 pdoPos = 0; /* Byte counter in PDO */
          CV(PDO)[i].dmFlag = 1;
          CV(PDO)[i].dmNotifyCount = 0;
#endif
          CV(PDO)[i].mapIdx = (uint16)(m-PDO_MAP);
          pdoLen = 0;
          m++;
          while (CO_PDOMAP(m)) {
            uint8 attr = coGetObjectAttr(CHP m->objIdx, m->subIdx);
            if (!(attr & CO_OD_ATTR_PDO_MAPABLE)) {
              coError(CHP LOC_CO_ERR_PDO_ATTR, 1, (uint16)i, 0, 0);
              return(coRes_ERR_DEFAULT_PDO_MAP_ARGUMENTS);
            }
            if (dir == PDO_TX) {
              if (!(attr & CO_OD_ATTR_READ)) {
                coError(CHP LOC_CO_ERR_PDO_ATTR, 2, (uint16)i, 0, 0);
                return(coRes_ERR_DEFAULT_PDO_MAP_ARGUMENTS);
              }
            } else if (dir == PDO_RX)  {
              if (!(attr & CO_OD_ATTR_WRITE)) {
                coError(CHP LOC_CO_ERR_PDO_ATTR, 3, (uint16)i, 0, 0);
                return(coRes_ERR_DEFAULT_PDO_MAP_ARGUMENTS);
              }
            } else {
              coError(CHP LOC_CO_ERR_PDO_ATTR, 4, (uint16)i, 0, 0);
              return(coRes_ERR_DEFAULT_PDO_MAP_ARGUMENTS);
            }
            pdoLen += m->len;
#if CO_DIRECT_PDO_MAPPING
            if (CV(PDO)[i].dmFlag) {
              const coObjDictEntry *ode = odFind(CHP m->objIdx);
              if (ode && (m->len & 7) == 0) {
                /* We know from the previous calls that (objIdx, subIdx) is valid */
                uint8 subIdx = m->subIdx;
                coObjDictVarDesc *vd = ode->objDesc;
                uint8 *objPtr = (uint8*)ode->objPtr;
                uint8 itemLen = m->len/8; /* Length of each mapped object in bytes */
                                          /* Locate the address of the data and the variable descriptor.
                * Compare to coReadObject()/coWriteObject(). */
                if (subIdx)
                  subIdx--;
                while (subIdx) {
                  if (vd->attr & CO_OD_ATTR_VECTOR) {
                    objPtr += vd->size*subIdx;
                    break;
                  } else {
                    objPtr += vd->size;
                    subIdx--;
                    vd++;
                  }
                }
                if (vd->attr & CO_OD_WRITE_VERIFY)
                  CV(PDO)[i].dmFlag = 0;
                if ((CV(PDO)[i].dir == PDO_RX && (vd->attr & CO_OD_WRITE_NOTIFY)) ||
                  (CV(PDO)[i].dir == PDO_TX && (vd->attr & CO_OD_READ_NOTIFY))) {
                  /* Is the same group (or object) already notified? */
                  uint8 j;
                  for (j = 0; j < CV(PDO)[i].dmNotifyCount; j++) {
                    if ((CV(PDO)[i].dmNotifyObjIdx[j] == m->objIdx && CV(PDO)[i].dmNotifySubIdx[j] == m->subIdx) ||
                      (vd->notificationGroup && CV(PDO)[i].dmNotifyGroup[j] == vd->notificationGroup))
                      break;
                  }
                  if (j >= CV(PDO)[i].dmNotifyCount) {
                    CV(PDO)[i].dmNotifyObjIdx[CV(PDO)[i].dmNotifyCount] = m->objIdx;
                    CV(PDO)[i].dmNotifySubIdx[CV(PDO)[i].dmNotifyCount] = m->subIdx;
                    CV(PDO)[i].dmNotifyGroup[CV(PDO)[i].dmNotifyCount] = vd->notificationGroup;
                    CV(PDO)[i].dmNotifyCount++;
                  }
                }
#if BIG_ENDIAN
                /* MikkoR 2005-01-21: Changed to use byte lenght instead of bit length */
                objPtr += (uint8)(coGetObjectLen(CHP m->objIdx, m->subIdx)/8)-1; /* If itemLen shold differ from coGetObjectLen(). */
                while(itemLen) {
                  CV(PDO)[i].dmDataPtr[pdoPos++] = objPtr--;
                  itemLen--;
                }
#else
                while(itemLen) {
                  CV(PDO)[i].dmDataPtr[pdoPos++] = objPtr++;
                  itemLen--;
                }
#endif
              } else
                CV(PDO)[i].dmFlag = 0;
            }
#endif /* CO_DIRECT_PDO_MAPPING */
            m++;
          }
          if (pdoLen > 64) {
            coError(CHP LOC_CO_ERR_PDO, 2, RV(nodeId), 0, 0);
            return(coRes_ERR_DEFAULT_PDO_MAP_PDO_SIZE);
          }
          CV(PDO)[i].dataLen = (uint8)pdoLen;
        }
      }
    }
  /* Reset the hearbeat consumer node list. */
#if HEARTBEAT_CONSUMER
  for( i=0; i<MAX_HEARTBEAT_NODES; i++ ){
    CV(heartbeatNodes)[i].nodeId = 0; /* 0 or >127 means not in use */
    CV(heartbeatNodes)[i].heartBeatPeriod = 0; /* 0 means not in use */
  }
#endif
  } /* if( parametersLoaded == false) */


// The following variables should not be loaded from nvram so they are always initialized 
#if EMERGENCY_PRODUCER_HISTORY
  for( i=0; i<MAX_EMCY_PRODUCER_HISTORY_RECORDED; i++){
    CV(emcyProdHistList)[i].additionalInfo = 0;
    CV(emcyProdHistList)[i].errorCode = 0;
	memset( CV(emcyProdHistList)[i].manufSpecField, 0, sizeof( CV(emcyProdHistList)[i].manufSpecField));
  }
#endif

/* Reset the hearbeat consumer runtime variables. */
#if HEARTBEAT_CONSUMER
  for( i=0; i<MAX_HEARTBEAT_NODES; i++ ){
    CV(heartbeatNodes)[i].alive = false;
    CV(heartbeatNodes)[i].state = coOS_Unknown;
	CV(heartbeatNodes)[i].isHeartBeatControlRunning = false;
  }
#endif

/* Reset any PDO runtime data */
  for (i = 0; i < PDO_MAX_COUNT; i++){
	CV(PDO)[i].status &= ~PDO_STATUS_RPDO_MONITORED; // Need to clear the RPDO monitored bit at reset comm
	CV(PDO)[i].status &= ~PDO_STATUS_INHIBIT_ACTIVE; // Need to clear the RPDO inhibit active at reset comm
	CV(PDO)[i].status &= ~PDO_STATUS_QUEUED; // Need to clear the RPDO queued at reset comm
	CV(PDO)[i].lastMsgTime = 0;
	CV(PDO)[i].pdoSyncCntr = 0;
  }

  (void)initSDO(CHP 1, SDO_SERVER, 0x600+RV(nodeId), 0x580+RV(nodeId));
#ifdef SDO_MANAGER_DEFAULT_CHANNELS
  for(i = 1; i <= SDO_MANAGER_DEFAULT_CHANNELS; i++){
	(void)initSDO(CHP i, SDO_CLIENT, 0x580 + i, 0x600 + i);
  }
#endif

#if IDENTITY_OBJECT
  /* The identity object is managed by the application as the numbers are
   * application and device specific
   */
  appGetIdentityObject(CHP &CV(vendorId), &CV(productCode), &CV(revisionNumber), &CV(serialNumber));
#endif

  /* Initialize runtime variables that depend on the communication objects */
  RV(intSyncCOBID) = long2cob(CV(syncCOBID));
  RV(intSyncPeriod) = kfhalUsToTicks(CV(syncPeriod));
  RV(intProducerHeartbeatTime) = kfhalMsToTicks(CV(producerHeartBeatTime));
  RV(intEmcyCOBID) = long2cob(CV(emcyCOBID));
  RV(intNMTBootTime) = kfhalMsToTicks( CV(NMTBootTime));

  /* The nodeguarding toggle bit is set to zero on reset communication */
  RV(nodeGuardToggle) = 0;

#if LIFE_GUARDING
  RV(lifeGuardingIsAlive) = false;
#endif

  /* Reset the Node-guarding node list */
#if NODEGUARD_MASTER
  for( i=0; i<MAX_GUARDED_NODES; i++ ){
    RV(guardedNodes)[i].nodeId = 0; /* 0 or >127 means not in use */
    RV(guardedNodes)[i].alive = false;
    RV(guardedNodes)[i].state = coOS_Unknown;
    RV(guardedNodes)[i].guardTimeMs = 0;
	RV(guardedNodes)[i].guardTimeTicks = 0;
	RV(guardedNodes)[i].isNodeguardingActive = false;
  }
#endif

  /* Initialize CAN Driver */
  updateUsedCobIds(CHP_S); /* Tell HAL what COB ids are in use */

#if LSS_SLAVE
  stat = canSetup(RV(canChan), LSSV(bitRate));
#else
  stat = canSetup(RV(canChan), RV(defaultBitrate));
#endif
  if (stat != CANHAL_OK){
    return(getCanHalError(stat));
  }
  stat = canSetCallback(RV(canChan), coCanCallback, CANHAL_CB_TX_DONE);
  if (stat != CANHAL_OK){
    canShutdown(RV(canChan));
    return(getCanHalError(stat));
  }

  stat = canGoBusOn(RV(canChan));
  if (stat != CANHAL_OK){
    canShutdown(RV(canChan));
    return(getCanHalError(stat));
  }
  stat = canSetTransceiverMode(RV(canChan), CANHAL_DRIVERMODE_NORMAL);
  if (stat != CANHAL_OK){
    canShutdown(RV(canChan));
    return(getCanHalError(stat));
  }

  /*
  *	 Make sure that the node will not boot.
  */
#if BOOTUP_DS302
  RV(isInstanceBooting) = false;
#endif

  /* initiate Time Stamp objects (see p 9-56)
   * ... */
  
  /* Transmit the boot-up message */
  i = transmitHeartbeatMessage(CHP true);
  RV(opState) = coOS_PreOperational;
  if( CV(NMTStartup) == CO_SLAVE_AUTOSTART){
	  RV(opState) = coOS_Operational;
  }
  if(i!=0)
    return(coRes_ERR_GEN_HEARTBEAT);
  else
    return(coRes_OK);
}

/* resetEmcyCntrlList is called in the initalizion phase of the stack, it is */
/* used to reset the list where all the EMERGENCY objects */

#if EMERGENCY_MASTER || EMERGENCY_CONSUMER_OBJECT
void resetEmcyCntrlList(CH_S){
  int k=0;
  while ( k < MAX_EMCY_GUARDED_NODES ){
    CV(emcyNodes)[k].nodeId = 0;   /* reset node list */
    k++;
  }
}
#endif


/* -------------------------------------------------------------------------- */

void coMain(int seconds) {
  KfHalTimer t0;
  CanId id;
  uint8 msg[8];
  uint8 dlc;
#if CO_MULTIPLE_INSTANCES
  int instanceCntr=0;
#endif
  t0 = kfhalReadTimer();

  for(;;){
#if CO_MULTIPLE_INSTANCES
    while( instanceCntr < CO_INST_MAX ){
      if (coInstancesInUse[instanceCntr]) {   /* Check if instance is running */
        coPeriodicService(&coRuntime[instanceCntr]);
        if(!canReadMessage(coRuntime[instanceCntr].canChan, &id, msg, &dlc)) {
          //printf("mess read");
          (void)coProcessMessage(&(coRuntime[instanceCntr]), id, msg, dlc);
        }
      }
      instanceCntr++;
    }
    instanceCntr = 0;
#else
    if(!canReadMessage(coRuntime[0].canChan, &id, msg, &dlc))
      (void)coProcessMessage(id, msg, dlc);
    coPeriodicService();
#endif
    if((kfhalReadTimer()-t0) > ((KfHalTimer)seconds*TIMERFREQ)) /*lint !e574 Warning 574: Signed-unsigned mix with relational */ /*lint !e737 Info 737: Loss of sign in promotion from int to unsigned long */
      break;
  }
}

/*
*   Implemented by PLENWARE.
*/
void coPoll(void)
{
  CanId id;
  uint8 msg[8];
  uint8 dlc;
#if CO_MULTIPLE_INSTANCES
  int instanceCntr=0;
#endif

#if CO_MULTIPLE_INSTANCES
  while( instanceCntr < CO_INST_MAX ){
    if(coInstancesInUse[instanceCntr]){
      while(canReadMessage(coRuntime[instanceCntr].canChan, &id, msg, &dlc) == CANHAL_OK)
        (void)coProcessMessage(&(coRuntime[instanceCntr]), id, msg, dlc);
      coPeriodicService(&coRuntime[instanceCntr]);        
    }
    instanceCntr++;
  }
#else
  while(canReadMessage(coRuntime[0].canChan, &id, msg, &dlc) == CANHAL_OK)
    (void)coProcessMessage(id, msg, dlc);
  coPeriodicService();
#endif
}



/*
* Access of the Object Dictionary.
* There are three supported opertions: get the length of an object (including
* test if a object exists), read an object or write an object.
* The OD can either be accessed via the OD table (the canopen objects or the
* applicaton objects), the code stack local object access fucntions or the
* application access functions.
*
* The main entry points are prefixed by 'co', local canopen access functions
* are prefixed by 'lco' and the application functiosn are prefixed by 'app'.
* The 'co'-functions starts by scanning the table, then if necessary calls
* the apropriate access function depending on the index.
*
* The functions returns 0 if success, otherwise a 32-bit error code.
*
* Areas of the object dictionary:
*
* Index      Object                                   Handled by
* ---------  ---------------------------------------  ----------
* 0000       -
* 0001-001f  Static data types                        RO
* 0020-003f  Complex data types                       RO
* 0040-005f  Manuf specific data types                RO
* 0060-007f  Dev profile specific data types          RO
* 0080-009f  Dev profile specific complex data types  RO
* 00a0-0fff  -
* 1000-1fff  Communcation profile area                locWriteObject()
* 2000-5fff  Manuf specific profile area              appWriteObject()
* 6000-9fff  Standardised device profile area         appWriteObject()
* a000-ffff  -
*
*
* All elements have a síze of an integral number of bytes, and all sizes/lengths
* are given in bytes.
*/


static coErrorCode coGlobError = 0; // May produce a warning depending on stack configuration
/* Helper function for various ReadObject functions. When an error condition
* occurs, do a
*   return coSettErr(CO_ERR_..., err)
* and the error code is stored in *err or in coGlobError if err==NULL.
* Always returns 0.
*/
uint32 coODReadSetError(coErrorCode err, coErrorCode *perr) {
  if (perr)
    *perr = err;
  else
    coGlobError = err;
  return 0;
}

/* Locate the OD entry for the object numbered objIdx or NULL if it doesn't
* exists in the table.
* We makes a linear search, but should probably make it binary in the future.
*/
static const coObjDictEntry *odFind(CH uint16 objIdx) {
  const coObjDictEntry *od = RV(OD);   /* qqq: This must be solved */
  while (od->objIdx)
    if (od->objIdx == objIdx)
      return od;
    else
      od++;
  if (RV(ODapp)) {
    od = RV(ODapp);
    while (od->objIdx)
      if (od->objIdx == objIdx)
        return od;
      else
        od++;
  }
  return NULL;
}

/* Extracts a numerical value from a buffer. The value is extracted in
* little-endian order. len is the length in bytes. */
static uint32 buf2val(uint8 *buf, uint8 len) {
  uint32 val = 0;
  buf += len; /* Points to MSB+1 (we use predecrement or CodeGuard will complain on buffer underrun) */
  while(len--)
    val = (val << 8)+*--buf;
  return val;
}

/* Writes to the buffer */
uint32 coBuf2Val(uint8 *buf, uint8 len) {
	return buf2val( buf, len);
}

/* Stores a numerical value in buf[0..len-1]. The value is stored in
* little-endian order. */
static void val2buf(uint32 val, uint8 *buf, uint8 len) {
  while(len) {
    *buf = (uint8)(val & 0xff);
    val >>= 8;
    buf++;
    len--;
  }
}


// Copies value of bit length to buffer in bit position.
// The values are stored in little endian order
// Note that the sender needs to provide a sufficiently large buffer
// val: Value to copy to the buffer
// len: The length (in bits) of the value

static void val2buf_bit(uint32 val, uint8 len, uint8* buf, uint8 pos) {
  uint8 shift;
  // Go to the correct byte
  buf += pos / 8;
  pos %= 8;
  val &= 0xffffffff >> (32 - len);

  if (pos != 0) {
    // Needs some bit shifting
    shift = 8 - pos;
    *buf &= (0xff >> shift);
    *buf |= val << pos;
    val >>= shift;
    len -= shift < len ? shift : len;
    buf++;
  }

  while (len / 8 > 0) {
    // Write whole bytes
    *buf = (uint8) (val & 0xff);
    val >>= 8;
    buf++;
    len -= 8;
  }

  if (len > 0) {
    // Write remaining bits
    *buf &= (0xff << len);
    *buf |= val;
  }
}

// Copies to value of bit length from buffer in bit position.
// The values are stored in little endian order
// Note that the sender needs to provide a sufficiently large buffer
static uint32 buf2val_bit(uint8 *buf, uint8 pos, uint8 len) {
  uint32 val = 0;

  // Go to the byte with the most significant bit
  buf += (pos + len - 1) / 8;
  pos = (pos + len) % 8;

  if (pos != 0) {
    if (pos > len) {
      // The whole value is within this byte
      // Needs some shifting
      val = (*buf >> (pos - len)) & (0xff >> (8 - len));
      len = 0;
    }
    else {
      // Needs some bit shifting
      val = *buf & (0xff >> (8 - pos));
      len -= pos;
    }

    buf--;
  }

  while (len / 8 > 0) {
    val = (val << 8) + *buf;
    buf--;
    len -= 8;
  }

  if (len > 0) {
    // Needs some bit shifting
    val = (val << len) + (*buf >> (8 - len));
  }
  
  return val;
}

void coVal2Buf(uint32 val, uint8 *buf, uint8 len) {
	val2buf( val, buf, len);
}

/* Store val interpreted as a len byytes value at objPtr.
* The value is written using the system's endiness.
*/
#if BIG_ENDIAN
static void storeVal(uint32 val, uint8 *objPtr, uint8 len) {
  objPtr += len; /* Points to LSB+1 */
  while(len) {
    *--objPtr = (uint8)(val & 0xff);
    val >>= 8;
    len--;
  }
}
#endif

/* Converts the value at objPtr[0..len-1] to a uint32.
*/
#if BIG_ENDIAN
static uint32 getVal(uint8 *objPtr, uint8 len) {
  uint32 val = 0;
  while(len--)
    val = (val << 8)+*objPtr++;
  return val;
}
#endif


/* Returns the length in bits of the specified object.subIdx or
* 0 if it doesn't exist.
*/
CoObjLength coGetObjectLen(CH uint16 objIdx, uint8 subIdx, coErrorCode *err) {
  CoObjLength locObjLength;
  const coObjDictEntry *ode = odFind(CHP objIdx);
  if (ode) {
    /* The object is described in OD. Calculate the size. */
	if (subIdx == 0){
	  if (ode->subIdxCount > 0){
        return CONV_FROM_BYTES(1); /* Size of subIndex */
	  } else {
        return CONV_FROM_BYTES(ode->objDesc->size); /* Just one value */
	  }
	} else if (subIdx <= ode->subIdxCount) {
      /* A vector or record type */
      const coObjDictVarDesc *vd = ode->objDesc;
      /* A vector or record type. The attribute of sub-index 1 can be found
       * in vd[0] etc. If an item in vd has the CO_OD_ATTR_VECTOR bit set, the
       * remaining subindexes are defined by that one. */
      while (--subIdx) {
		if (vd->attr & CO_OD_ATTR_VECTOR){
          return CONV_FROM_BYTES(vd->size);
		} else {
          vd++;
		}
	  }
      return CONV_FROM_BYTES(vd->size);
	} else if (subIdx == 0xff){
      return CONV_FROM_BYTES(4); /* Size of the structure sub-index */
	} else {
      return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
	}
  }

  /* No match in OD. We handle it it with switches. */
  if (objIdx == 0){
    return coODReadSetError( CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
  } else if (objIdx < 0x0040){
    locObjLength = locGetObjectLen(CHP objIdx, subIdx, err);
    if( *err){
		*err = 0;
        return appGetObjectLen(CHP objIdx, subIdx, err);
	} else {
      return locObjLength;
	}
  } else if (objIdx < 0x1000) {
    return coODReadSetError( CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
  } else if (objIdx < 0x2000){
    return locGetObjectLen(CHP objIdx, subIdx, err);
  } else if (objIdx < 0xe000){
  	return appGetObjectLen(CHP objIdx, subIdx, err);
  }
  return coODReadSetError( CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
}

/* Returns the attribute of the specified object.subIdx or
* 0 if it doesn't exist.
*/
uint8 coGetObjectAttr(CH uint16 objIdx, uint8 subIdx) {
  const coObjDictEntry *ode = odFind(CHP objIdx);
  if (ode) {
    /* The object is described in OD. Get the attribute. */
    if (subIdx == 0)
      if (ode->subIdxCount > 0)
        return CO_OD_ATTR_READ; /* Subindex count, (usually) read only */
      else
        return ode->objDesc->attr; /* Just one value */
    else if (subIdx <= ode->subIdxCount) {
      /* A vector or record type */
      const coObjDictVarDesc *vd = ode->objDesc;
      while (--subIdx) {
        if (vd->attr & CO_OD_ATTR_VECTOR)
          return vd->attr;
        else
          vd++;
      }
      return vd->attr;
#if SUBINDEX_FF_STRUCTURE
    } else if (subIdx == 0xff){
      return CO_OD_ATTR_READ; /* The structure sub-index */
#endif
    } else
      return 0;
  }

  /* No match in OD. We handle it it with switches. */
  if (objIdx == 0)
    return 0;
  else if (objIdx < 8) /* Dummy entries. Possibly breaking the CANopen spec, we make them PDO mapable also for reads. */
    return CO_OD_ATTR_READ|CO_OD_ATTR_WRITE|CO_OD_ATTR_PDO_MAPABLE;
  else if (objIdx < 0x0260) /* Various data types */
    return CO_OD_ATTR_READ;
  else if (objIdx < 0x00a0)
    return appGetObjectAttr(CHP objIdx, subIdx);
  else if (objIdx < 0x1000)
    return 0;
  else if (objIdx < 0x2000) /* Comm profile area */
    return CO_OD_ATTR_READ|CO_OD_ATTR_WRITE; /* Not exact */
  else if (objIdx < 0x6000) /* Manufacturer Specific Profile Area */
    return appGetObjectAttr(CHP objIdx, subIdx);
  else if (objIdx < 0xa000) /* Standardised Device Profile Area */
    return appGetObjectAttr(CHP objIdx, subIdx);
  else if (objIdx < 0xb000) /*  Standardized Network Variable Area */
    return appGetObjectAttr(CHP objIdx, subIdx);
  else
    return 0;
}

/*
*   coWriteObjectP makes it possible to address the local object dictionatry
*   with objects of a size bigger than 32 bits.
*/
coErrorCode coWriteObjectP(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len) {
  coErrorCode res = CO_ERR_OBJECT_DOES_NOT_EXISTS;
  uint8 *objP = NULL;
  coObjDictVarDesc varDescP;

  if (len > 0 && len <= 4) { 
    uint32 val = buf2val(buf, len);
    res = coWriteObject(CHP objIdx, subIdx, val);
  }
  else {
    res = odFindObjP(CHP objIdx, subIdx, &objP, &varDescP);
    if (res == CO_OK) {
	  CoObjLength copyCntr = 0;
      // Clear the old value
      memset(objP, 0, varDescP.size);

      if (varDescP.size > len) {
        copyCntr = len;
      } else {
        copyCntr = varDescP.size;
      }
      memcpy(objP, buf, copyCntr);
      /* Simon B: Bugfix, write notify needed also for objects larger than 32bit */
	  if (varDescP.attr & CO_OD_WRITE_NOTIFY) {
          (void)coODWriteNotify(CHP objIdx, subIdx, varDescP.notificationGroup);
      }
    }
  }
  if (res == CO_ERR_OBJECT_DOES_NOT_EXISTS) {
    res = locWriteObjectP(CHP objIdx, subIdx, buf, len);
  }

  if ((res == CO_OK) && 
      (objIdx != CO_OBJDICT_VER_CONFIG) && 
      (objIdx != CO_OBJDICT_STORE_PARAMS)) {
    // Reset the configuration date and time to 0.
    CV(verCfgDate) = 0;
    CV(verCfgTime) = 0;
  }

  return res;
}


/*
*   This function looks for the given objIdx and subIdx for the given objIdx and
*   subIdx and returns a object pointer and variable descriptor
*/
static coErrorCode odFindObjP(CH uint16 objIdx, uint8 subIdx, uint8 **objP, coObjDictVarDesc *varP) {
  coErrorCode res = CO_ERR_OBJECT_DOES_NOT_EXISTS;
  const coObjDictEntry *odeP = NULL;
  uint8 *objPtr = NULL;
  const coObjDictVarDesc *varDescPtr = NULL;

  odeP = odFind(CHP objIdx);
  if (odeP) {
    varDescPtr = odeP->objDesc;
    objPtr = (uint8*)odeP->objPtr; 
    if (subIdx == 0) {
      if (odeP->subIdxCount > 0) {
        res = CO_ERR_OBJECT_READ_ONLY;
      }
    } else if (subIdx > odeP->subIdxCount) {
      res = CO_ERR_INVALID_SUBINDEX;
    } else {
      subIdx--;
      while (subIdx) {
        if (varDescPtr->attr & CO_OD_ATTR_VECTOR) {
          objPtr += varDescPtr->size*subIdx;
          break;
        } else {
          objPtr += varDescPtr->size;
          subIdx--;
          varDescPtr++;
        }
      }
    }
  }
  if (odeP && objPtr && varDescPtr) {
    *objP = objPtr;
    *varP = *varDescPtr;
    res = CO_OK;
  }
  return res;
}




/*
*   
*   This function writes to the local object dictionary. If
*   we write, sucessfully, to a loc obj entry and this
*   object is in the object scanner list we use that as
*
*/
coErrorCode coWriteObject(CH uint16 objIdx, uint8 subIdx, uint32 val) {
  uint8 tbuf[4];
  coErrorCode res = CO_ERR_OBJECT_DOES_NOT_EXISTS;
#if MULTIPLEXED_PDO
  bool isObjInScanLst = false;
#endif
  const coObjDictEntry *ode = odFind(CHP objIdx);
  if (ode) {
    const coObjDictVarDesc *vd = ode->objDesc;
    uint8 *objPtr = (uint8*)ode->objPtr; 
    if (subIdx == 0) {
      if (ode->subIdxCount > 0) {
        res = CO_ERR_OBJECT_READ_ONLY;
      }
    } else if (subIdx > ode->subIdxCount) {
      res = CO_ERR_INVALID_SUBINDEX;
    } else {
      // Bugfix 050208. Variable subIdx is used later on.
      int i = subIdx - 1; // 2005-03-01 MikkoR: Bufix to a bugfix
      while (i) {
        if (vd->attr & CO_OD_ATTR_VECTOR) {
          objPtr += vd->size*i;
          break;
        } else {
          objPtr += vd->size;
          i--;
          vd++;
        }
      }
    }
    if (!(vd->attr & CO_OD_ATTR_WRITE)) {
      return CO_ERR_OBJECT_READ_ONLY;
    }
    if (vd->attr & CO_OD_WRITE_VERIFY) {
      res = coODWriteVerify(CHP objIdx, subIdx, val);
	  if( res != CO_OK){
	      return res;
	  }	
    }
    storeVal(val, objPtr, (uint8)vd->size);
    if (vd->attr & CO_OD_WRITE_NOTIFY) {
      (void)coODWriteNotify(CHP objIdx, subIdx, vd->notificationGroup);
    }
    res = CO_OK;
  }
  if (res == CO_ERR_OBJECT_DOES_NOT_EXISTS) {
    if (objIdx == 0) {
      res = CO_ERR_OBJECT_DOES_NOT_EXISTS;
    } else if (objIdx < 0x08) {
      res = CO_OK; /* A dummy write object */
    } else if (objIdx < 0x1000) {
      res = CO_ERR_OBJECT_READ_ONLY;
    } else if (objIdx < 0x2000) {
      res = locWriteObjectEx(CHP objIdx, subIdx, val);
    }
    if (res == CO_ERR_OBJECT_DOES_NOT_EXISTS) {
      if (objIdx < 0xb000) {
        uint8 appAttr;
        /* 
        *   Going for the application... 
        */
        appAttr = appGetObjectAttr(CHP objIdx, subIdx);
        if (appAttr) {
          if (!(appAttr & CO_OD_ATTR_WRITE)) {
            res = CO_ERR_OBJECT_READ_ONLY;
			return res;
          } else {
			  CoObjLength len = appGetObjectLen(CHP objIdx, subIdx, &res);
			  if( res != CO_OK){
				return res;
			  }
			  if( BITSBYTES(len) > 4){
				  return CO_ERR_GENERAL_INTERNAL_INCOMPABILITY;
			  }
            val2buf(val, tbuf, (uint8)BITSBYTES(len));
            res = appWriteObject(CHP objIdx, subIdx, tbuf, 0, (uint8)BITSBYTES(len), CO_SEG_DONE );
          }
        } 
      } else {
        res = CO_ERR_OBJECT_DOES_NOT_EXISTS;
      }
    }
  }
#if MULTIPLEXED_PDO
  /*
  *   This is our "data change evenet"-handler. This means that
  *   we should transmit a SAMPDO to the bus (qqq: I think) if
  *   the given object is in the scanner list.
  */
  if (res == CO_OK) {
    isObjInScanLst = isObjInScannerList(CHP objIdx, subIdx);
    if (isObjInScanLst) {
      CoStackResult coStaRes;
      coStaRes = transmitSAMPDO(CHP objIdx, subIdx, val);
      if (coStaRes != coRes_OK) {
        /*
        *   qqq: Maybe we do have a scanner list configured
        *   but maybe we have no SAM MPDO producer configured,
        *   What should we do about this?
        */
        coError(CHP 1,2,3,4,5); /*  What should we do about this? */
      }
    }
  }
#endif
  if ((res == CO_OK) && 
      (objIdx != CO_OBJDICT_VER_CONFIG) && 
      (objIdx != CO_OBJDICT_STORE_PARAMS)) {
    // Reset the configuration date and time to 0.
    CV(verCfgDate) = 0;
    CV(verCfgTime) = 0;
  }

  return res;
}

/* Called after update of an object with attribute CO_OD_WRITE_NOTIFY.
* If the value is updated "automatically" via the OD, it can be checked and
* modified here. If the new value is invalid, an error code can be returned,
* but in this case the old value should be restored somehow.
*/
static coErrorCode coODWriteNotify(CH uint16 objIdx, uint8 subIdx, uint16 notificationGroup) {
  if (objIdx >= 0x2000 && objIdx < 0xb000) /* Manufacturer Specific Profile Area or Standardised Device Profile Area or Standardized network variable area  */
    return appODWriteNotify(CHP objIdx, subIdx, notificationGroup);
  switch(objIdx) {
    case CO_OBJDICT_PRODUCER_HEARTBEAT_TIME:
      if (subIdx != 0)
        return CO_ERR_INVALID_SUBINDEX;
      RV(intProducerHeartbeatTime) = kfhalMsToTicks(CV(producerHeartBeatTime));
      if (CV(producerHeartBeatTime) && RV(intProducerHeartbeatTime) == 0)
        RV(intProducerHeartbeatTime) = 1;	/* If kfhalMsToTicks converted to zero, smallest possible is one. */
	  /* Reset node guearing toggle bit when node guarding is enabled */
	  if( CV(producerHeartBeatTime) == 0){
	  	RV(nodeGuardToggle) = 0;	
	  }
      return CO_OK;
    case CO_OBJDICT_SYNC_COBID:
      RV(intSyncCOBID) = long2cob(CV(syncCOBID));
      if((CV(syncCOBID) & SYNC_COBID_GENERATE)){ /* Maybe the sync should stop? */
        RV(intSyncPeriod) = kfhalUsToTicks(CV(syncPeriod));
	  } else {
        RV(intSyncPeriod) = 0;
	  }
	  updateUsedCobIds(CHP_S);
      return CO_OK;
    case CO_OBJDICT_SYNC_PERIOD:
      if(CV(syncCOBID) & SYNC_COBID_GENERATE){
        RV(intSyncPeriod) = kfhalUsToTicks(CV(syncPeriod));
      } else {
        RV(intSyncPeriod) = 0;
      }
      return CO_OK;
    case CO_OBJDICT_EMCY_COBID:
      RV(intEmcyCOBID) = long2cob(CV(emcyCOBID));  /* Bugfixed: 040921 */
      return CO_OK;
    default:
      return CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT;
  }
}

static coErrorCode coODWriteVerify(CH uint16 objIdx, uint8 subIdx, uint32 val){
	if (objIdx >= 0x2000 && objIdx < 0xb000){ /* Manufacturer Specific Profile Area or Standardised Device Profile Area or Standardized network variable area */
      return appODWriteVerify(CHP objIdx, subIdx, val);
	}
	if( objIdx == CO_OBJDICT_SYNC_COBID){
	  	cobIdT cobId = long2cob(val);
		if( isRestrictedCanId( cobId)){
			return CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED;
		}
	}
	if( objIdx == CO_OBJDICT_EMCY_COBID){
	  	cobIdT cobId = long2cob(val);
		if( isRestrictedCanId( cobId)){
			return CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED;
		}
	}
	return CO_OK;
}

/* Read from the object dictionary.
* Returns 0 if success, otherwise a 32-bit error code.
*/
coErrorCode coReadObjectP(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len) {
	
  uint8 tValid = 0; 
  uint8 tReadCntr; 
  CoObjLength tExpected;
  coErrorCode res = coRes_OK;  

	tExpected = coGetObjectLen(CHP objIdx, subIdx, &res); /* Get object length. */
	if(res != CO_OK){
		return res;
	}   
	tReadCntr = 0;
    while( tReadCntr < BITSBYTES(tExpected)){
		res = coReadObjectEx(CHP objIdx, subIdx, &buf[tReadCntr], tReadCntr, &tValid, len-tReadCntr );
		  if(res != CO_OK){
              return res; 
          }
          if( tValid > (len-tReadCntr) || tValid == 0 ){
              return CO_ERR_GENERAL; /* Amount of written data to the buffer can not be bigger than the buffer itself, used to crash!  */ 
          }
          tReadCntr = tReadCntr + tValid;
	}
	return res;
}

coErrorCode coReadObjectEx(CH uint16 objIdx, uint8 subIdx, uint8 *buf, CoObjLength pos, uint8 *(valid), uint8 len) {
  coErrorCode res; 

  res = locReadObjectP(CHP objIdx, subIdx, buf, pos, valid, len);

  if (res == CO_ERR_OBJECT_DOES_NOT_EXISTS) {
  	uint8 *objP = NULL;
  	coObjDictVarDesc vd;
    res = odFindObjP(CHP objIdx, subIdx, &objP, &vd);
    if (res == CO_OK) {
    	CoObjLength copyCntr;
		
		if (!(vd.attr & CO_OD_ATTR_READ)){
      		return CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT;
		}
    	if (vd.attr & CO_OD_READ_NOTIFY) {
      		res = coODReadNotify(CHP objIdx, subIdx, vd.notificationGroup);
      		if (res){
        		return res;
			}
		}

	  	copyCntr = vd.size - pos;

	   	if( copyCntr > len) {
        	copyCntr = len;
      	}
      	memcpy(buf, objP+pos, copyCntr);	
		*valid = (uint8)copyCntr;
	}

  }
  
  if (res == CO_ERR_OBJECT_DOES_NOT_EXISTS) {
    uint8 appAttr = appGetObjectAttr(CHP objIdx, subIdx);
    if( !appAttr) {
        return CO_ERR_OBJECT_DOES_NOT_EXISTS;
    }
    if(!(appAttr & CO_OD_ATTR_READ)) {
        /*  The object exist but only with write flags. */
        return CO_ERR_OBJECT_WRITE_ONLY;
    } 
	res = appReadObject(CHP objIdx, subIdx, buf, pos, valid, len);   
  } 
  return res;
}



/* Read from the object dictionary, returns the result.
* In case of error, *err is assigned an error code, otherwise 0.
* err can be NULL.
*/
uint32 coReadObject(CH uint16 objIdx, uint8 subIdx, coErrorCode *err) {
  uint8 tBuf[4]; 
  uint8 tValid = 0; 
  uint8 tReadCntr; 
  CoObjLength tExpected;

  const coObjDictEntry *ode;
  if (err)
    *err = CO_OK;
  ode = odFind(CHP objIdx);
  if (ode) {
    /*  
    *   The requested object was found in the list of objects
    *   that was introduced by the application. 
    */
    const coObjDictVarDesc *vd = ode->objDesc;
    uint8 *objPtr = (uint8*)ode->objPtr; /* So it can be added to */
    if (subIdx == 0) {
      if (ode->subIdxCount > 0)
        return ode->subIdxCount;
      /* vd points to the correct descriptor, objPtr is OK */
#if SUBINDEX_FF_STRUCTURE
    } else if (subIdx == 0xff){
      return ode->structure;
#endif
    } else if (subIdx > ode->subIdxCount)
      return coODReadSetError(CO_ERR_INVALID_SUBINDEX, err);
    else {
      /* A valid subIdx for a vector or record type */
      subIdx--;
      while (subIdx) {
        if (vd->attr & CO_OD_ATTR_VECTOR) {
          objPtr += vd->size*subIdx;
          break;
        } else {
          objPtr += vd->size;
          subIdx--;
          vd++;
        }
      }
    }
    if (!(vd->attr & CO_OD_ATTR_READ))
      return coODReadSetError(CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT, err);
    if (vd->attr & CO_OD_READ_NOTIFY) {
      coErrorCode res = coODReadNotify(CHP objIdx, subIdx, vd->notificationGroup);
      if (res)
        return coODReadSetError(res, err);
    }
    return getVal(objPtr, (uint8)vd->size);
  }
  if (objIdx == 0) {
    return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
  } else if (objIdx < 0x260) {
    return readDataType(objIdx, subIdx, err);
/*} else if (objIdx < 0x0040) {
    return locReadObject(objIdx, subIdx, err);
  } else if (objIdx < 0x00a0) {
    return appReadObject(objIdx, subIdx, err);*/
  } else if (objIdx < 0x1000) {
    return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
  } else if (objIdx < 0x2000) {
    return locReadObjectEx(CHP objIdx, subIdx, err);
  } else if (objIdx < 0xb000) {
    uint8 appAttr = appGetObjectAttr(CHP objIdx, subIdx);
    if (appAttr) {
      if(!(appAttr & CO_OD_ATTR_READ)) {
        /*
        *   The object exist but only with write flags.
        */
        return coODReadSetError(CO_ERR_OBJECT_WRITE_ONLY, err);
      } else {
        tExpected = appGetObjectLen(CHP objIdx, subIdx, err); /* Get object length. */
        /* Do we have a fair chance at all? */
        if(tExpected > 32){
          return coODReadSetError(CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED, err);
        } else {
          tReadCntr = 0;
          while( tReadCntr < BITSBYTES(tExpected)){
            coErrorCode e0 = appReadObject(CHP objIdx, subIdx, &tBuf[tReadCntr], tReadCntr, &tValid, (sizeof(tBuf)-tReadCntr));
			if( err){
              *err = e0;
			}
            if(e0 != CO_OK){
              return(0); /* The error code is given by application */
            }
            if( tValid > (sizeof(tBuf)-tReadCntr)){
              if(err) {
                *(err) = CO_ERR_GENERAL; /* Amount of written data to the buffer can not be bigger than the buffer itself, used to crash!  */
                return(0);
              }
            }
            tReadCntr = tReadCntr + tValid;
          }
          /* qqq: Now we've received the contents of an object, held by the */
          /*      application ín "string" format. It's a good idea to to */
          /*      convert this sting to a value. */
          return (buf2val(tBuf, tReadCntr));
        }
      }
    } else {
      return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
    }
  } else {
    return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
  }
}


/* Called before an object with attribute CO_OD_READ_NOTIFY is read.
* If the value is read "automatically" via the OD, it can be updated
* here. If the read operation should fail for some reason, an error
* code can be returned.
*/
static coErrorCode coODReadNotify(CH uint16 objIdx, uint8 subIdx, uint16 notificationGroup) {
  if (objIdx >= 0x2000 && objIdx < 0xb000){ /* Manufacturer Specific Profile Area or Standardised Device Profile Area or Standardized network variable area */
    return appODReadNotify(CHP objIdx, subIdx, notificationGroup);
  } else if (objIdx == CO_OBJDICT_NMT_STARTUP){
#if BOOTUP_DS302 || DS302_SLAVE_AUTOSTART
    return CO_OK;
#else
    return CO_ERR_OBJECT_DOES_NOT_EXISTS;
#endif
  } else {
    return CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT;
  }
}


/* Help function for a GetObjectLen.
* For a simple data type, set arrLen to 1 and elemSize to the size of the data type.
* For an array, set arrLen to the number of elements and elemSize to the element size.
* The function can not be used for a record data type.
*/
uint8 coObjLen(uint8 subIdx, uint8 arrLen, uint8 elemSize) {
  if (subIdx == 0)
    if (arrLen > 1)
      return 1; /* Size of subIndex */
    else
      return elemSize; /* Size of the one-and-only element */
  else if (subIdx <= arrLen)
    return elemSize;
#if SUBINDEX_FF_STRUCTURE
  else if (subIdx == 0xff)
    return 4; /* Size of the structure sub-index */
#endif
  else
    return 0;
}

/* Returns the length in bits of the specified object.subIdx or
* 0 if it doesn't exist.
*/
static CoObjLength locGetObjectLen(CH uint16 objIdx, uint8 subIdx, coErrorCode *err) {
/*if (objIdx == 0x1001) / * Error Register * /
    return coObjLen(subIdx, 1, 1);
  else if (objIdx == 0x1014) / * COB-ID EMCY * /
    return coObjLen(subIdx, 1, 4); / * qqq But the COB-ID's are said to behave as an array somehow * /
  else */
  if (objIdx >= 0x1200 && objIdx < 0x1300) { /* Server or Client SDO parameter */
    switch (subIdx) {
      case 0:
        return CONV_FROM_BYTES(1);     /* qqq: This is in bits! */
      case 1: /* COB-ID Client->Server */
      case 2: /* COB-ID Server->Client */
        return CONV_FROM_BYTES(4);
      case 3: /* Node id of the other end */
        return CONV_FROM_BYTES(1);
#if SUBINDEX_FF_STRUCTURE
      case 0xff:
        return CONV_FROM_BYTES(4); /* Size of the structure sub-index */
#endif
      default:
 	  	return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
     }
  } else if ((objIdx >= 0x1400 && objIdx < 0x1600) || (objIdx >= 0x1800 && objIdx < 0x1a00)) {
    /* Receive or TransmitPDO Communication Parameters */
    switch (subIdx) {
      case 0: /* subIdx */
        return CONV_FROM_BYTES(1); //Bugfix UH-050131.
      case 1: /* COB-ID */
        return CONV_FROM_BYTES(4); //Bugfix UH-050131, used to return 1 byte sized objLen.
      case 2: /* Transmission Type */
      case 4: /* Compability entry */
        return CONV_FROM_BYTES(1);
      case 3: /* Inhibit Time */
      case 5: /* eventTimer */
        return CONV_FROM_BYTES(2);
#if SUBINDEX_FF_STRUCTURE
      case 0xff:
        return CONV_FROM_BYTES(4);
#endif
      default:
 	  	return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
    }
  } else if ((objIdx >= 0x1600 && objIdx < 0x1800) || (objIdx >= 0x1a00 && objIdx < 0x1c00)) {
    /* Reveive or Transmit PDO Mapping Parameters
     * We do not check if there is a map entry for the given subIndex. */
    if (subIdx == 0)
      return CONV_FROM_BYTES(1);
#if SUBINDEX_FF_STRUCTURE
    else if (subIdx == 0xff)
      return CONV_FROM_BYTES(4);
#endif
    else
      return CONV_FROM_BYTES(4);
#if HEARTBEAT_CONSUMER
  } else if (objIdx == CO_OBJDICT_CONSUMER_HEARTBEAT_TIME) {
    if (subIdx == 0) {
      return CONV_FROM_BYTES(1);
    } else if (subIdx <= MAX_HEARTBEAT_NODES) {
      return CONV_FROM_BYTES(4);
    } else {
 	  	return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
    }
#endif
#if BOOTUP_DS302
  } else if (objIdx == CO_OBJDICT_VER_CONFIG || objIdx == CO_OBJDICT_VER_APPL_SW) {   /* ObjIdx : 0x1020, 0x1f52. */
    switch (subIdx) {
      case 0:
        return CONV_FROM_BYTES(1);
      case 1:
      case 2:
        return CONV_FROM_BYTES(4);
      default:
 	  	return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
    }
  } else if (objIdx == CO_OBJDICT_NMT_STARTUP || objIdx == CO_OBJDICT_NMT_BOOTTIME) {
    switch (subIdx) {
      case 0:
        return CONV_FROM_BYTES(4);
      default:
 	  	return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
    }
  } else if ((objIdx >= CO_OBJDICT_NMT_NETWORK_LIST) &&
             (objIdx <= CO_OBJDICT_NMT_STARTUP_EXPECT_SERIAL_NUMBER)) {
    if (subIdx == 0) {
      return CONV_FROM_BYTES(1);
    } else if (subIdx >= 1 && subIdx <= 127) {
      return CONV_FROM_BYTES(4);
    } else {
 	  	return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
    }
#endif
#if IDENTITY_OBJECT
  } else if (objIdx == CO_OBJDICT_IDENTITY) {
    if (subIdx == 0) {
      return CONV_FROM_BYTES(1);
    } else if (subIdx >= 1 && subIdx <= 4) {
      return CONV_FROM_BYTES(4);
    } else {
 	  	return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
    }
#endif
#if EMERGENCY_PRODUCER_HISTORY
  } else if( objIdx == CO_OBJDICT_PREDEFINED_ERR_FIELD){
	  if( subIdx == 0){
		return CONV_FROM_BYTES(1);	  
	  } else {
		uint16 errCode;
		uint16 additionalInfo;
		int i = subIdx - 1;
		if( i >= MAX_EMCY_PRODUCER_HISTORY_RECORDED){
	 	  	return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
		}
		errCode = CV(emcyProdHistList)[i].errorCode;
		additionalInfo = CV(emcyProdHistList)[i].additionalInfo;
		if( errCode == 0 && additionalInfo == 0){
	 	  	return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
		} else {
			return CONV_FROM_BYTES(4);
		}
	 }
#endif
#if EMERGENCY_CONSUMER_OBJECT
  } else if (objIdx == CO_OBJDICT_EMCY_CONSUMER) {
    if (subIdx == 0) {
      return CONV_FROM_BYTES(1);
    } else if (subIdx >= 1 && subIdx <= MAX_EMCY_GUARDED_NODES) {
      return CONV_FROM_BYTES(4);
    } else {
 	  	return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
    }
#endif
#if MULTIPLEXED_PDO
  } else if (objIdx >= CO_OBJDICT_SCANNER_LIST && objIdx < CO_OBJDICT_DISPATCH_LST) {
    if (subIdx == 0) {
      return CONV_FROM_BYTES(1);
    } else if (subIdx >= 1 && subIdx <= 0xfe) {
      return CONV_FROM_BYTES(4);
    } else {
 	  return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
    }
  } else if (objIdx >= CO_OBJDICT_DISPATCH_LST && objIdx < 0x1fff) {
    if (subIdx == 0) {
      return CONV_FROM_BYTES(1);
    } else if (subIdx >= 1 && subIdx <= 0xfe) {
      return CONV_FROM_BYTES(8);
    } else {
 	  return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
    }
#endif
#if STORE_PARAMS /* DS301, p.9-70, 0x1010. */
  } else if (objIdx == CO_OBJDICT_STORE_PARAMS) {
    if(subIdx == 0) {                 // Added by PN
      return CONV_FROM_BYTES(1);
    } else if( subIdx >= 1 && subIdx <= 4){
	  return CONV_FROM_BYTES(4);
	} else {
 	  return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
	}
#endif
#if RESTORE_PARAMS /* DS301, 0x1011. */
  } else if (objIdx == CO_OBJDICT_RESTORE_PARAMS) {
    if(subIdx == 0) {                 
      return CONV_FROM_BYTES(1);
    } else if( subIdx >= 1 && subIdx <= 4){
	  return CONV_FROM_BYTES(4);
	} else {
 	  return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
	}
#endif
#if ERROR_BEHAVIOR /* DS301, 0x1029. */
  } else if (objIdx == CO_OBJDICT_ERR_BEHAVIOR) {
    if( subIdx >= 0 && subIdx <= 0xff) {                 
      return CONV_FROM_BYTES(1);
	} else {
 	  return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
	}
#endif
#if OD_MFG_STRINGS
  } else if (objIdx == CO_OBJDICT_MANUF_DEVICE_NAME){
	if( subIdx == 0) {
		uint8 len = appGetMfgDeviceNameLen(CHP_S);
		if( len > 15){ /* maximum device name has 15 bytes */
			len = 15;
		}
		return CONV_FROM_BYTES( len);
	} else {
 		return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
	}
  } else if (objIdx == CO_OBJDICT_MANUF_HW_VERSION) {
	if( subIdx == 0) {
		uint8 len = appGetMfgHwVersionLen(CHP_S);
		if( len > 15){
			len = 15;
		}
		return CONV_FROM_BYTES( len);
	} else {
 		return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
	}
  } else if (objIdx == CO_OBJDICT_MANUF_SW_VERSION) {
	if( subIdx == 0) {
		uint8 len = appGetMfgSwVersionLen(CHP_S);
		if( len > 15){
			len = 15;
		}
		return CONV_FROM_BYTES( len);
	} else {
 		return coODReadSetError( CO_ERR_INVALID_SUBINDEX, err);
	}
#endif
  } else {
	return coODReadSetError( CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
  }
}

/* If we store COB-IDs in uint16 internally, we must translate them when
* they are read from or written to the CAN bus.
*/
#if !USE_EXTENDED_CAN
static cobIdT long2cob(uint32 p) {
  cobIdT cobid = p & CANHAL_ID_MASK;
  if (p & cobPDOinvalidP)
    cobid |= cobPDOinvalid;
  if (p & cobExtendedP)
    cobid |= cobExtended;
  return cobid;
}
static uint32 cob2long(cobIdT cobid) {
  uint32 p = cobid & CANHAL_ID_MASK;
  if (cobid & cobPDOinvalid)
    p |= cobPDOinvalidP;
  if (cobid & cobExtended)
    p |= cobExtendedP;
  return p;
}
#endif


/*
*   locWriteObjectPEx is the extended version of locWriteObjP. It accesses the 
*   standard "locWriteObjP" which is aimed to carry local objects bigger than
*   32 bits and if the object is not found there is tries the locWriteObject
*   which holds local objects in the sizerange 8-32bits.
*/
/*
// May produce a warning depending on stack configuration
static coErrorCode locWriteObjectPEx(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len) {
  coErrorCode res;
  res = locWriteObjectP(CHP objIdx, subIdx, buf, len);
  if (res == CO_ERR_OBJECT_DOES_NOT_EXISTS) {
    uint32 val = buf2val(buf, 4);
    res = locWriteObject(CHP objIdx, subIdx, val);
  }
  return res;
}
*/

/*
*   locWriteObjectEx is the extended version of locWriteObject, This function 
*   accesses the standard "locWriteObject" function and if the object is not
*   found in there it tries the objects held by locWriteObjectP.
*/
static coErrorCode locWriteObjectEx(CH uint16 objIdx, uint8 subIdx, uint32 val) {
  coErrorCode res;
  res = locWriteObject(CHP objIdx, subIdx, val);
  if (res == CO_ERR_OBJECT_DOES_NOT_EXISTS) {
    uint8 buf[4];
    val2buf(val, buf, sizeof(buf));
    res = locWriteObjectP(CHP objIdx, subIdx, buf, sizeof(buf));
  }
  return res;
}


/*
*   locWriteObjectP mainly holds the objects bigger than 32 bits. However, it is sometimes
*   neccary for this function to hold objects 32 bits or smaller, by confinece.
*   The reason for this is that sometimes a object consist of objects with a
*   size bigger that 32 bits and smaller. To aviod having to implement that specific
*   object in the "locWriteObject" it can be placed here. All the objects
*   found in locWriteObjectP must have at least one object (subindex) larger than
*   32 bits.
*/
static coErrorCode locWriteObjectP(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len) {
  coErrorCode res = CO_ERR_OBJECT_DOES_NOT_EXISTS;
#if MULTIPLEXED_PDO
  if (objIdx >= CO_OBJDICT_DISPATCH_LST && objIdx < CO_OBJDICT_MAN_SPEC_PROF_AREA) {
    res = writeDispatcherList(CHP objIdx, subIdx, buf, len);
  }
#endif
#if OD_MFG_STRINGS 
  if (objIdx == CO_OBJDICT_MANUF_DEVICE_NAME
  		  || objIdx == CO_OBJDICT_MANUF_HW_VERSION
		  || objIdx == CO_OBJDICT_MANUF_SW_VERSION) {
		return CO_ERR_OBJECT_READ_ONLY;
  }
#endif

  return res;
}


/* 
*   Write an object in the part of the OD that we manage.
*   Returns 0 if success, otherwise a 32-bit error code.
*/
static coErrorCode locWriteObject(CH uint16 objIdx, uint8 subIdx, uint32 val) {
  int i;
  uint16 j;

  if (objIdx >= 0x1200 && objIdx < 0x1300) {
    /* Server or Client SDO parameter */
    if (objIdx < 0x1280) {
      /* j = 1+objIdx-0x1200; ID: Is this an off by one error from kvaser ? */
      j = objIdx-0x1200;
      i = findSDOc(CHP (uint8)j, SDO_SERVER); /* Server SDO Parameters */
    } else {
#if CLIENT_SDO_SUPPORT
      /* j = 1+objIdx-0x1280; ID: Is this an off by one error from kvaser ? */
      j = objIdx-0x1280;
      i = findSDOc(CHP (uint8)j, SDO_CLIENT); /* Client SDO Parameters */
#else
      return(CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT); /* No client SDOs available. */
#endif
    }
	if( j > SDO_MAX_COUNT){
      return CO_ERR_OBJECT_DOES_NOT_EXISTS;
	}
    if (i < 0)
      return CO_ERR_OBJDICT_DYN_GEN_FAILED; /* No room for more SDOs */
    switch(subIdx) {
      case 0:
#if SUBINDEX_FF_STRUCTURE
      case 0xff:
#endif
        packSDO(CHP i); /* If a new entry was created above */
        return CO_ERR_OBJECT_READ_ONLY;
      case 1:
		if( objIdx == 0x1200 ){
		  return CO_ERR_OBJECT_READ_ONLY;
		}
        if (CV(SDO)[i].type == SDO_SERVER)
          CV(SDO)[i].COBID_Rx = long2cob(val);
        else
          CV(SDO)[i].COBID_Tx = long2cob(val);
        packSDO(CHP i);
        break;
      case 2:
		if( objIdx == 0x1200 ){
		  return CO_ERR_OBJECT_READ_ONLY;
		}
        if (CV(SDO)[i].type == SDO_SERVER)
          CV(SDO)[i].COBID_Tx = long2cob(val);
        else
          CV(SDO)[i].COBID_Rx = long2cob(val);
        packSDO(CHP i);
        break;
      case 3:
		if( objIdx == 0x1200){
          return CO_ERR_INVALID_SUBINDEX;
		}
        CV(SDO)[i].otherNodeId = (uint8)val;
        packSDO(CHP i);
        break;
      default:
        packSDO(CHP i);
        return CO_ERR_INVALID_SUBINDEX;
    }
  } else if ((objIdx >= 0x1400 && objIdx < 0x1600) || (objIdx >= 0x1800 && objIdx < 0x1a00)) {
    /* ReveivePDO or TransmitPDO Communication Parameters */
    if (objIdx < 0x1600)
      i = findPDOc(CHP 1+objIdx-0x1400, PDO_RX);
    else
      i = findPDOc(CHP 1+objIdx-0x1800, PDO_TX);
	
	if( i < 0){
    	return CO_ERR_OBJECT_DOES_NOT_EXISTS;
	}
 
    switch(subIdx) {
      case 0:
#if SUBINDEX_FF_STRUCTURE
      case 0xff:
#endif
        packPDO(CHP i); /* If a new entry was created above */
        return CO_ERR_OBJECT_READ_ONLY;
      case 1: {
	  	cobIdT cobId = long2cob(val);
		/* 
		*  It is not allowed to change a valid PDOs COB ID. Bit 31
		*  of the COB ID tells us if the PDO is valid or not. CiA 301 v4.2.0.70,
		*  section 7.5.2.35 (RPDO) and 7.5.2.37 (TPDO). Fixed SimonB 2012-03-15
		*/
		int cobChange = 0;
		if( (CV(PDO)[i].COBID & (~cobPDOinvalid)) != (cobId & (~cobPDOinvalid))){
			cobChange = 1;
		}
		if( cobChange && (CV(PDO)[i].COBID & cobPDOinvalid) == 0 && (cobId & cobPDOinvalid) == 0){
			return CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED;
		}
		
		/* Don't allow setting a restricted CAN ID */
		if( isRestrictedCanId( cobId)){
			return CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED;
		}

		/* Extended COB ID must be marked as extended */
		if( (cobId & cobExtended) == 0 && (cobId & CANHAL_ID_MASK) > 0x7FF){
			return CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED;			
		}

		CV(PDO)[i].COBID = cobId;
        updateUsedCobIds(CHP_S);
		packPDO(CHP i);
        break;
	  }
      case 2:
		  /* Reject reserved transmission types */
		if(objIdx < 0x1600 && ( val >= 0xF1 && val <= 0xFD)){ // RPDO
			return CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED;			
		}
		if( objIdx >= 0x1800 && (val >= 0xF1 && val <= 0xFB)){ // TPDO
			return CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED;			
		}
        CV(PDO)[i].transmissionType = (uint8)val;
		CV(PDO)[i].pdoSyncCntr = 0;
        packPDO(CHP i);
        break;
      case 3:
		// Fix SimonB 2012-03-15 It's not allowed to change inhibit time while PDO exists
		// CiA 301 4.2.0.70 section 7.5.2.35
		if( (CV(PDO)[i].COBID & cobPDOinvalid) == 0){
			return CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED;
		}
#if USE_INHIBIT_TIMES
        CV(PDO)[i].inhibitTime = kfhalMsToTicks(val / 10);
#else
        CV(PDO)[i].inhibitTime = 0;
#endif
        packPDO(CHP i);
        break;
      case 5:
        CV(PDO)[i].eventTimer = val;
		CV(PDO)[i].status &= ~PDO_STATUS_RPDO_MONITORED; /* CiA-301: The deadline monitoring is activated within the next reception of an RPDO after configuring the event-timer */
        packPDO(CHP i);
        break;
      default:
        packSDO(CHP i);
        return CO_ERR_INVALID_SUBINDEX;
    }
  } else if ((objIdx >= 0x1600 && objIdx < 0x1800) || (objIdx >= 0x1a00 && objIdx < 0x1c00)) {
    /* ReveivePDO or TransmitPDO Mapping Parameters (dynamic mapping of PDO) */
#if DYNAMIC_PDO_MAPPING
    return(writeDynamicMapPDO(CHP objIdx, subIdx, val));
#else
    return CO_ERR_OBJECT_READ_ONLY; /* if dynamic PDO mapping is not being used then these obejcts are read only. */
#endif

#if HEARTBEAT_CONSUMER
  } else if (objIdx == CO_OBJDICT_CONSUMER_HEARTBEAT_TIME) {
    return  writeConsumerHeartbeatTime(CHP subIdx, val);
#endif

#if BOOTUP_DS302
  } else if (objIdx == CO_OBJDICT_NMT_STARTUP || objIdx == CO_OBJDICT_VER_CONFIG) {  /*  qqq: Bugfixed 040905 */
    return writeNMTStartupSettings(CHP objIdx, subIdx, val);
  } else if (objIdx == CO_OBJDICT_VER_APPL_SW) {
    return writeNMTStartupSettings(CHP objIdx, subIdx, val);
  } else if (objIdx == CO_OBJDICT_NMT_EXPECTED_SW_DATE || objIdx == CO_OBJDICT_NMT_EXPECTED_SW_TIME) {
    return writeNMTStartupSettings(CHP objIdx, subIdx, val);
  } else if (objIdx == CO_OBJDICT_NMT_NETWORK_LIST) {
    return writeNMTStartupSettings(CHP objIdx, subIdx, val);
  } else if (objIdx >= CO_OBJDICT_NMT_STARTUP_EXPECT_DEVICETYPE && objIdx <= CO_OBJDICT_NMT_BOOTTIME) {
    return writeNMTStartupSettings(CHP objIdx, subIdx, val);
#endif
#if IDENTITY_OBJECT
  } else if (objIdx == CO_OBJDICT_IDENTITY) {
    /*
    *   We will not override the the read only attributes (the reason for "false" arg).
    */
    return (writeIdentityObject(CHP objIdx, subIdx, val, false));  /* DS301, p. 9-78. */
#endif
#if EMERGENCY_PRODUCER_HISTORY
  } else if (objIdx == CO_OBJDICT_PREDEFINED_ERR_FIELD) {
    return writeNodeEmcyHist(CHP subIdx, val);  /* DS301, p.9-65, 0x1003. */
#endif
#if EMERGENCY_CONSUMER_OBJECT
  } else if (objIdx == CO_OBJDICT_EMCY_CONSUMER) {
    return writeEmcyConsObj(CHP subIdx, val);  /* DS301, p.11-13, 0x1028. */
#endif
#if ERROR_BEHAVIOR
  } else if (objIdx == CO_OBJDICT_ERR_BEHAVIOR) {
	return writeErrorBehaviorObj(CHP subIdx, (uint8)val);  /* DS301, p125, 0x1029. */
#endif
#if MULTIPLEXED_PDO /* DS301, p.11-8, 0x1fa0-0x1fcf. */
  } else if (objIdx >= CO_OBJDICT_SCANNER_LIST && objIdx < CO_OBJDICT_DISPATCH_LST) {
    return writeScannerList(CHP objIdx, subIdx, val);
#endif
#if STORE_PARAMS /* DS301, p.9-70, 0x1010. */
  } else if (objIdx == CO_OBJDICT_STORE_PARAMS) {
    return writeStoreParamObj(CHP subIdx, val);
#endif
#if RESTORE_PARAMS /* DS301, p.9-72, 0x1011. */
  } else if (objIdx == CO_OBJDICT_RESTORE_PARAMS) {
    return writeRestoreParamObj(CHP subIdx, val);
#endif
#if OD_MFG_STRINGS 
  } else if (objIdx == CO_OBJDICT_MANUF_DEVICE_NAME
  		  || objIdx == CO_OBJDICT_MANUF_HW_VERSION
		  || objIdx == CO_OBJDICT_MANUF_SW_VERSION) {
    return CO_ERR_OBJECT_READ_ONLY;
#endif
  } else {
    return CO_ERR_OBJECT_DOES_NOT_EXISTS; 
  }
  return CO_OK;
}


/*
*   locReadObjectP mainly holds the objects bigger than 32 bits. However, it is sometimes
*   neccary for this function to hold objects 32 bits or smaller, by convinience.
*   The reason for this is that sometimes a object consist of objects with a 
*   size bigger that 32 bits and smaller. To aviod having to implement that specific
*   object in the "locReadObject" it can be placed here. All the objects 
*   found in locReadObjectP must have at least one object (subindex) larger than
*   32 bits (meaning it can not be implemented in the locReadObject).
*/
static coErrorCode locReadObjectP(CH uint16 objIdx, uint8 subIdx, uint8 *buf, CoObjLength pos, uint8 *(valid), uint8 len) {
  coErrorCode res = CO_ERR_OBJECT_DOES_NOT_EXISTS;
  if (objIdx >= CO_OBJDICT_DISPATCH_LST && objIdx < CO_OBJDICT_MAN_SPEC_PROF_AREA) {
#if MULTIPLEXED_PDO > 0																	// Added by PN
    res = readDispatcherList(CHP objIdx, subIdx, buf, len);
#else
	res = CO_ERR_GENERAL_INTERNAL_INCOMPABILITY;
#endif
#if OD_MFG_STRINGS
  } else if (objIdx == CO_OBJDICT_MANUF_DEVICE_NAME){
	uint8 mfgDeviceName[15];
	int finalLen = len;
	if( subIdx != 0) {
		return CO_ERR_INVALID_SUBINDEX;
	}
	if( appGetMfgDeviceName(CHP mfgDeviceName, sizeof( mfgDeviceName))){
		//if( finalLen > len){
		//	finalLen = sizeof( mfgDeviceName);
		//}
		//memcpy( buf, mfgDeviceName, finalLen);
		finalLen = sizeof( mfgDeviceName) - pos; 
		if(finalLen > len){
			finalLen = len;
		}
		memcpy( buf, mfgDeviceName + pos, finalLen);
		*valid = finalLen;
		return CO_OK;
	} else {
		return CO_ERR_OBJECT_DOES_NOT_EXISTS;
	}
  } else if (objIdx == CO_OBJDICT_MANUF_HW_VERSION) {
	uint8 mfgHwVersion[15];
	int finalLen = len;
	if( subIdx != 0) {
		return CO_ERR_INVALID_SUBINDEX;
	}
	if( appGetMfgHwVersion(CHP mfgHwVersion, sizeof( mfgHwVersion))){
		//if( finalLen > len){
		//	finalLen = sizeof( mfgHwVersion);
		//}
		//memcpy( buf, mfgHwVersion, finalLen);
		finalLen = sizeof( mfgHwVersion) - pos; 
		if(finalLen > len){
			finalLen = len;
		}
		memcpy( buf, mfgHwVersion + pos, finalLen);
		*valid = finalLen;
		return CO_OK;
	} else {
		return CO_ERR_OBJECT_DOES_NOT_EXISTS;
	}
  } else if (objIdx == CO_OBJDICT_MANUF_SW_VERSION) {
	uint8 mfgSwVersion[15];
	int finalLen = len;
	if( subIdx != 0) {
		return CO_ERR_INVALID_SUBINDEX;
	}
	if( appGetMfgSwVersion(CHP mfgSwVersion, sizeof( mfgSwVersion))){
		//if( finalLen > len){
		//	finalLen = sizeof( mfgSwVersion);
		//}
		//memcpy( buf, mfgSwVersion, finalLen);
		finalLen = sizeof( mfgSwVersion) - pos; 
		if(finalLen > len){
			finalLen = len;
		}
		memcpy( buf, mfgSwVersion + pos, finalLen);
		*valid = finalLen;
		return CO_OK;
	} else {
		return CO_ERR_OBJECT_DOES_NOT_EXISTS;
	}
#endif
  }

  if (res == CO_ERR_OBJECT_DOES_NOT_EXISTS) {
    uint32 val = locReadObject(CHP objIdx, subIdx, &res);
	if (res == CO_OK) {
		CoObjLength actualLenInBits = coGetObjectLen(CHP objIdx, subIdx, &res);
		if( res != CO_OK){
			*valid = 0;
			return res;
		}
		if( actualLenInBits == 0 || BITSBYTES(actualLenInBits) > 4){
			*valid = 0;
			return CO_ERR_GENERAL_INTERNAL_INCOMPABILITY;
		}
		*valid = (uint8)BITSBYTES(actualLenInBits);
		val2buf(val, buf, (uint8)BITSBYTES(actualLenInBits));
    }
  }
  return res;
}

/*
*   This function looks for the given object index in both locReadObjectP 
*   and locReadObject. The aim for locReadObjectP is to introduce objects
*   greater than 4 bytes, but sometimes, for convinence it can be more simple
*   to add some uint32 (or smaller) objects here (typically if it is a 
*   object that carries an object with uint64s and uint8s).
*/
static uint32 locReadObjectEx(CH uint16 objIdx, uint8 subIdx, coErrorCode *err) {
  uint32 res;
  coErrorCode locErrCode;
  res = locReadObject(CHP objIdx, subIdx, &locErrCode);
  if (locErrCode == CO_ERR_OBJECT_DOES_NOT_EXISTS) {
    uint8 buf[4];
	CoObjLength pos = 0;
	uint8 valid = 0;
    locErrCode = locReadObjectP(CHP objIdx, subIdx, buf, pos, &valid, sizeof(buf));
    if (locErrCode == CO_OK) {
      res = buf2val(buf, sizeof(buf));
    }
  }
  coODReadSetError(locErrCode, err);
  return res;
}




/* 
*   Read an object from the part of the OD that we manage.
*   Stores 0 in *err if success, otherwise a 32-bit error code.
*/
static uint32 locReadObject(CH uint16 objIdx, uint8 subIdx, coErrorCode *err) {
  int i;
  uint16 j;
  if (err)
    *err = 0;
  if (objIdx >= 0x1200 && objIdx < 0x1300) {
    /* 
    *   Server or Client SDO parameter 
    */
    if (objIdx < 0x1280) {
      j = 1+objIdx-0x1200;
      i = findSDO(CHP (uint8)j, SDO_SERVER); 
    } else {
#if CLIENT_SDO_SUPPORT
      j = 1+objIdx-0x1280;
      i = findSDO(CHP (uint8)j, SDO_CLIENT);
#else
      return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
//      return cob2long(cobPDOinvalid);
#endif
    }
	if( j > SDO_MAX_COUNT){
      return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
	}
    switch(subIdx) {
      case 0:
		if( objIdx == 0x1200){
		  return 2;
		} else {
		  return 3;
		}
      case 1:
        if (i < 0)
          return cob2long(cobPDOinvalid);  /* qqq: PDO invalid ? SDO invalid? */
        else
          return cob2long((CV(SDO)[i].type == SDO_SERVER) ? CV(SDO)[i].COBID_Rx : CV(SDO)[i].COBID_Tx);  /* COB-ID Client -> Server */
      case 2:
        if (i < 0)
          return cob2long(cobPDOinvalid);  /* qqq: PDO invalid ? SDO invalid? */
        else
          return cob2long((CV(SDO)[i].type == SDO_SERVER) ? CV(SDO)[i].COBID_Tx : CV(SDO)[i].COBID_Rx);  /* COB-ID Server -> Client */
      case 3:
		if( objIdx == 0x1200){
          return coODReadSetError(CO_ERR_INVALID_SUBINDEX, err);
		}
        else if (i < 0)
          return 0;
        else
          return CV(SDO)[i].otherNodeId;
#if SUBINDEX_FF_STRUCTURE
      case 0xff:
        return CO_ODDEF(CO_ODOBJ_DEFSTRUCT, CO_ODTYPE_SDO_PAR);
#endif
      default:
        return coODReadSetError(CO_ERR_INVALID_SUBINDEX, err);
    }
  } else if ((objIdx >= 0x1400 && objIdx < 0x1600) || (objIdx >= 0x1800 && objIdx < 0x1a00)) {
    /* 
    *   ReceivePDO or TransmitPDO Communication Parameters 
    */
    if (objIdx < 0x1600)
      i = findPDO(CHP 1+objIdx-0x1400, PDO_RX);
    else
      i = findPDO(CHP 1+objIdx-0x1800, PDO_TX);
	
	if( i < 0){
    	return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
	}
    
	switch(subIdx) {
      case 0:
        return 5;
      case 1:
          return cob2long(CV(PDO)[i].COBID);
      case 2:
          return CV(PDO)[i].transmissionType;
      case 3:
          return kfhalTicksToMs(CV(PDO)[i].inhibitTime);
      case 5:
          return CV(PDO)[i].eventTimer;
#if SUBINDEX_FF_STRUCTURE
      case 0xff:
        return CO_ODDEF(CO_ODOBJ_DEFSTRUCT, CO_ODTYPE_PDO_COMM_PAR);
#endif
      default:
        return coODReadSetError(CO_ERR_INVALID_SUBINDEX, err);
    }
  } else if ((objIdx >= 0x1600 && objIdx < 0x1800) || (objIdx >= 0x1a00 && objIdx < 0x1c00)) {
    /* 
    *   ReceivePDO or TransmitPDO Mapping Parameters 
    */
    uint16 n;
    const coPdoMapItem *m;
    if (objIdx < 0x1800)
      i = findPDO(CHP 1+objIdx-0x1600, PDO_RX);
    else
      i = findPDO(CHP 1+objIdx-0x1a00, PDO_TX);
#if MULTIPLEXED_PDO
    if (IS_POSITIVE(i)) {
      if (CV(PDO)[i].status & PDO_STATUS_DAM_PDO) {
        return SUB_0_DAMPDO;
      } else if (CV(PDO)[i].status & PDO_STATUS_SAM_PDO) {
        return SUB_0_SAMPDO;
      }
    }
#endif 
    /* 
    *   Count how many map entries there is 
    */
    if (i < 0 || CV(PDO)[i].mapIdx == PDO_MAP_NONE) {
      /*
      *   PDO not found or not mapping found for PDO.
      */
      return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
    } else {
      /*
      *   PDO mapping found. Parse all the mapped objects and calculate
      *   how many there is.
      */
      m = &PDO_MAP[CV(PDO)[i].mapIdx+1]; /* m points to the first map item */
      n = 0;
      while (CO_PDOMAP(m)) {
        n++;
        m++;
      }
    }
    if (subIdx == 0){
#if DYNAMIC_PDO_MAPPING
      if(CV(PDO)[i].status & PDO_STATUS_SUB_DISABLED)
        return 0; /* If disabled => no subIndexes available */
      else
#endif
        return n;
#if SUBINDEX_FF_STRUCTURE
    } else if (subIdx == 0xff){
      return CO_ODDEF(CO_ODOBJ_DEFSTRUCT, CO_ODTYPE_PDO_MAPPING);
#endif
    } else if (subIdx > n || i < 0){
      return coODReadSetError(CO_ERR_INVALID_SUBINDEX, err);
    } else {
      uint32 returnValue;
	  m = &PDO_MAP[CV(PDO)[i].mapIdx+subIdx];
	  /* 2012-03-15 Simon B Added type casts to support Renesas M16C compiler */
	  returnValue = m->len; 
	  returnValue += ((uint16)m->subIdx) << 8;
	  returnValue += (((uint32)LSB(m->objIdx)) << 16);
	  returnValue += (((uint32)MSB(m->objIdx)) << 24);
      return returnValue;
    }
#if HEARTBEAT_CONSUMER
  } else if (objIdx == CO_OBJDICT_CONSUMER_HEARTBEAT_TIME) {
    return readConsumerHeartbeatTime(CHP subIdx, err);
#endif
#if BOOTUP_DS302
  } else if (objIdx == CO_OBJDICT_VER_CONFIG) {
    return(readNMTStartupSettings(CHP objIdx, subIdx, err));
  } else if (objIdx == CO_OBJDICT_VER_APPL_SW) {
    return(readNMTStartupSettings(CHP objIdx, subIdx, err));
  } else if (objIdx == CO_OBJDICT_NMT_STARTUP &&  objIdx < 0x2000) {  /* qqq: Bugfix 041006. */
    return(readNMTStartupSettings(CHP objIdx, subIdx, err));
  } else if (objIdx == CO_OBJDICT_NMT_EXPECTED_SW_DATE || objIdx == CO_OBJDICT_NMT_EXPECTED_SW_TIME) {
    return(readNMTStartupSettings(CHP objIdx, subIdx, err));
  } else if (objIdx == CO_OBJDICT_NMT_NETWORK_LIST) {
    return(readNMTStartupSettings(CHP objIdx, subIdx, err));
  } else if (objIdx >= CO_OBJDICT_NMT_STARTUP_EXPECT_DEVICETYPE && objIdx <= CO_OBJDICT_NMT_BOOTTIME) {
    return readNMTStartupSettings(CHP objIdx, subIdx, err);
#endif
#if IDENTITY_OBJECT
  } else if (objIdx == CO_OBJDICT_IDENTITY) {
    return (readIdentityObject(CHP objIdx, subIdx, err));  /* DS301, p. 9-78. */
#endif
#if EMERGENCY_PRODUCER_HISTORY
  } else if (objIdx == CO_OBJDICT_PREDEFINED_ERR_FIELD) {
    return readNodeEmcyHist(CHP subIdx, err);  /* DS301, p.9-65, 0x1003. */
#endif
#if EMERGENCY_CONSUMER_OBJECT
  } else if (objIdx == CO_OBJDICT_EMCY_CONSUMER) {
    return readEmcyConsObj(CHP subIdx, err);  /* DS301, p.11-13, 0x1028. */
#endif
#if ERROR_BEHAVIOR
  } else if (objIdx == CO_OBJDICT_ERR_BEHAVIOR) {
	return readErrorBehaviorObj(CHP subIdx, err); /* DS301, p125, 0x1029. */
#endif
#if MULTIPLEXED_PDO /* DS301, p.11-8, 0x1fa0-0x1fcf. */
  } else if (objIdx >= CO_OBJDICT_SCANNER_LIST && objIdx < CO_OBJDICT_DISPATCH_LST) {
    return readScannerList(CHP objIdx, subIdx, err);
#endif
#if STORE_PARAMS /* DS301, p.9-70, 0x1010. */
  } else if (objIdx == CO_OBJDICT_STORE_PARAMS) {
    return readStoreParamObj(CHP subIdx, err);
#endif
#if RESTORE_PARAMS /* DS301, p.9-72, 0x1011. */
  } else if (objIdx == CO_OBJDICT_RESTORE_PARAMS) {
    return readRestoreParamObj(CHP subIdx, err);
#endif
  } else {
    return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
  }
}


/* Return the length/structure of the supported data types
*/
static uint32 readDataType(uint16 objIdx, uint8 subIdx, coErrorCode *err) {
  if (objIdx < 0x20) {
#if SUBINDEX_FF_STRUCTURE
    if (subIdx == 0xff)
      return CO_ODDEF(CO_ODOBJ_DEFTYPE, CO_ODTYPE_UINT32);
    else if (subIdx)
      return coODReadSetError(CO_ERR_INVALID_SUBINDEX, err);
#else
    if (subIdx)
      return coODReadSetError(CO_ERR_INVALID_SUBINDEX, err);
#endif
    switch(objIdx) {
      case CO_ODTYPE_BOOLEAN:
        return 1;
      case CO_ODTYPE_INT8:
      case CO_ODTYPE_UINT8:
        return 8;
      case CO_ODTYPE_INT16:
      case CO_ODTYPE_UINT16:
        return 16;
      case CO_ODTYPE_INT32:
      case CO_ODTYPE_UINT32:
        return 32;
      default:
        return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
    }
  } else {
    /* Structured data types */
    return coODReadSetError(CO_ERR_OBJECT_DOES_NOT_EXISTS, err);
  }
}

/* Handle a received CAN message.
*
* The COB-id's we recognize are:
*  0                 A NMT Module Control Message
*  0x700+nodeId RTR  A NMT Node Guarding message
*  *                 A PDO or SDO message
*  0x701..0x7ff      A Heartbeat message
*
* Returns zero if the CAN message was consumed.
*/
CoStackResult coProcessMessage(CH CanId cmId, uint8 *cmBuf, uint8 cmDlc) {
  int i;

/*  CoStackResult res; qqq: this is something for the next realise to use. */
  cobIdT COBID = cmId & CANHAL_ID_MASK;
  //printf("\nReceived COBID: %x", cmId);
#if USE_EXTENDED_CAN
  if (cmId & CANHAL_ID_EXTENDED)
    COBID |= cobExtended;
#endif

#if LSS_SLAVE
  /* Callout to optional LSS module */
  if( lssProcessMessage(CHP COBID, cmBuf, cmDlc)){
	return coRes_OK;
  }
#endif

  if (COBID == 0 && !(cmId & CANHAL_ID_RTR)) { /* Standard id, no RTR */
    /* A NMT Module Control Message. */
    if (cmDlc == 2 && (cmBuf[1] == 0 || cmBuf[1] == RV(nodeId)))
      if (RV(opState) == coOS_Operational || RV(opState) == coOS_PreOperational || RV(opState) == coOS_Stopped)
        if (!(CV(NMTStartup) & CO_NODE_IS_NMT_MASTER))
          handleNMT(CHP (coNMTfunction)cmBuf[0]);
#if NODE_GUARDING
  } else if ((cmId & CANHAL_ID_RTR) && COBID == 0x700+RV(nodeId)) {
    /* A NMT Node Guarding message (std can-id) */
	handleNMTNodeGuarding(CHP_S); 
	return coRes_OK;
#endif
  } else if ((COBID == (RV(intSyncCOBID) & CANHAL_ID_MASK))) { /* RTR? */
    handleSyncEvent(CHP_S);
	return coRes_OK;
  } else {
    /* Is it a PDO or a SDO message? */
    if (RV(opState) == coOS_Operational) {
      /* Scan the PDO table for the incoming CAN id. */
      if (cmId & CANHAL_ID_RTR) {
        /* 
         *  A RTR message. If RTR is disallowed, the cobRTRdisallowed bit is set
         *  in PDO[i].COBID, and the comparision below will fail
         *
         */
        for (i = 0; i < PDO_MAX_COUNT; i++){
          if (CV(PDO)[i].dir == PDO_TX && CV(PDO)[i].COBID == COBID) {
            if (cmDlc == BITSBYTES(CV(PDO)[i].dataLen)){ /* We demand the RTR and our message should have the same DLC. */
              handlePDORequest(CHP i);
              return(coRes_OK);
            }
            return (coRes_ERR_DLC_REQUEST_PDO);
          }
        }
      } else { 
        /*
        *   Parse all the receive PDO COBIDs (see if it is a RPDO).
        */
        for (i = 0; i < PDO_MAX_COUNT; i++){
          if (CV(PDO)[i].dir != PDO_UNUSED && (CV(PDO)[i].COBID & cobIdMask) == COBID) {
            if (CV(PDO)[i].dir == PDO_RX && (CV(PDO)[i].COBID & cobPDOinvalid) == 0){
              uint8 transType = CV(PDO)[i].transmissionType;
              uint8 copyCntr = 0;
              if( transType <= CO_PDO_TT_SYNC_CYCLIC_MAX){
                /* 
                *   A transmissiontype is used so that the application should *
                *   only be notified on the reception of the next SYNC. 
                */
#if SYNC_WINDOW_LENGTH
			    /* RPDOs outside of sync window may be discarded */
				if( CV(syncWindowLen) > 0 && kfhalReadTimer() - RV(syncRecvTime) > kfhalUsToTicks(CV(syncWindowLen))){
	                /* we indicate app user the sync window length expires */
					appIndicateSyncWindowLengthExpire(CHP CV(PDO)[i].pdoNo); 
					return coRes_OK;
				}
#endif
                while (copyCntr < cmDlc) {
                  CV(PDO)[i].cmBuf[copyCntr] = *(cmBuf + copyCntr);
                  copyCntr++;
                }
                CV(PDO)[i].cmDlc = cmDlc;   /* Copy data so that we know vaild no. databytes */
                CV(PDO)[i].status |= SYNC_RX_PDO_EVENT; /* This PDO should be "handled" with the following SYNC */
              } else {
                /* 
                *   The transmission type says that we should handle the 
                *   PDO now (not related to the sync).
                */
                handlePDO(CHP i, cmBuf, cmDlc);
                return coRes_OK;  /* No errorcodes received from handlePDO */
              }
            }
            else
            {
              // Someone is using our COB-ID
              coEmergencyError(CHP CO_EMCY_MONITORING_COM_COBID_COLLISION_ERROR);
#if ERROR_BEHAVIOR
			  switch(CV(CommunicationError)){
				case 0:
					if( RV(opState)== coOS_Operational){
						RV(opState) = coOS_PreOperational; 
					}
					break;
				case 1: 
					break;
				case 2: 
					RV(opState) = coOS_Stopped;
					break;
				default:
					break;
			  }
#else
			  RV(opState) = coOS_PreOperational; 
#endif

              // The error is handled so we return ok
              return coRes_OK;
            }
          }
        }
      }
    }
    if (RV(opState) == coOS_PreOperational || RV(opState) == coOS_Operational) {
      /* 
      *   Parse all the receive SDO COBIDs (see if it is a receive SDO). 
      */
      if (!(cmId & CANHAL_ID_RTR)) {
        for (i = 0; i < SDO_MAX_COUNT; i++) {
          if (CV(SDO)[i].type != SDO_UNUSED) {
            if (CV(SDO)[i].COBID_Rx == COBID) {
              handleSDO(CHP i, cmBuf, cmDlc);
              return (coRes_OK);
            }
            else if (CV(SDO)[i].COBID_Tx == COBID) {
              // Someone is using our COB-ID
              coEmergencyError(CHP CO_EMCY_MONITORING_COM_COBID_COLLISION_ERROR);

#if ERROR_BEHAVIOR
			  switch(CV(CommunicationError)){
				case 0:
					if( RV(opState)== coOS_Operational){
						RV(opState) = coOS_PreOperational; 
					}
					break;
				case 1: 
					break;
				case 2: 
					RV(opState) = coOS_Stopped;
					break;
				default:
					break;
			  }
#else
			  RV(opState) = coOS_PreOperational; 
#endif

              // The error is handled so we return ok
              return coRes_OK;
            }
          }
        }
      }
    }
    /*
    *   Look if it is a Heartbeat message.
    */
    if (cmDlc == 1 && COBID > 0x700 && COBID < 0x780) { /* A Heartbeat message */
#if HEARTBEAT_CONSUMER
      handleHeartbeat(CHP (uint8)(COBID-0x700), (coOpState)(cmBuf[0] & 0x7f));
#endif
#if NODEGUARD_MASTER
	  handleNodeguardMessage(CHP (uint8)(COBID-0x700), cmBuf[0] & 0x80, (coOpState)(cmBuf[0] & 0x7f));
#endif
	  return (coRes_OK);
    }
    // Check if there is a COB-ID conflict
    if (CV(emcyCOBID) == COBID) {
		// Someone is using our COB-ID

		// We have to filter out the EMCY we are sending ourselves or it will create an infinite loop
		// with two nodes both running the Kvaser stack sending EMCY at each other.
		if( *(uint16 *)cmBuf != CO_EMCY_MONITORING_COM_COBID_COLLISION_ERROR){
			coEmergencyError(CHP CO_EMCY_MONITORING_COM_COBID_COLLISION_ERROR);
		}

#if ERROR_BEHAVIOR
	  switch(CV(CommunicationError)){
			case 0:
				if( RV(opState)== coOS_Operational){
					RV(opState) = coOS_PreOperational; 
				}
				break;
			case 1: 
				break;
			case 2: 
				RV(opState) = coOS_Stopped;
				break;
			default:
				break;
	  }
#else
	  RV(opState) = coOS_PreOperational; 
#endif
      
      // The error is handled so we return ok
      return coRes_OK;
    }

#if EMERGENCY_MASTER || EMERGENCY_CONSUMER_OBJECT
    /* 
    *   Parse the EMERGENCY GURADING lookup table and see if this is a emergency message.
    */
    i=0;
    while( (i < MAX_EMCY_GUARDED_NODES) && (CV(emcyNodes)[i].COBID != 0) && (cmDlc == 8) ){
      if(CV(emcyNodes)[i].COBID == COBID ){
        handleReceivedEmcy(CHP CV(emcyNodes)[i].nodeId, cmBuf );
        return (coRes_OK); /* We've taken care of this CAN-message */
      } else {
        i++;
      }
    }
#endif
  }
  return (coRes_OK_UNPROCESSED);
}

/* handleReceivedEmcy is called if a emergency message has been received. */

void handleReceivedEmcy(CH uint8 nodeID, uint8* dataPtr) {
  uint16 emcyErrCode = (uint16) buf2val(&dataPtr[0], 2);
  uint8 errReg = dataPtr[2];
  /* nodeId : Current node´s node Id. */
  /* buf[0]->buf[1]:	Emergency Error Code. */
  /* buf[3]:			Error register. */
  /* buf[4]->buf[7]:	Manufacurer specific error field. */

  /* the only thing to do is to call the application, what should CANopen do? */
  appEmcyError(CHP nodeID, emcyErrCode, errReg, &dataPtr[3]);}

/* 
*   handleNMT is called if a network management message has been received.
*/
static void handleNMT(CH coNMTfunction cmd) {
  switch(cmd) {
    case coNMT_StartRemoteNode: /* {coOS_PreOperational} -> coOS_Operational */
      RV(opState) = coOS_Operational;
      break;
    case coNMT_StopRemoteNode:  /* {coOS_PreOperational, coOS_Prepared} -> coOS_Stopped */
      RV(opState) = coOS_Stopped;
      break;
    case coNMT_EnterPreOperationalState: /* {coOS_Operational, coOS_Prepared} -> coOS_PreOperational */
      RV(opState) = coOS_PreOperational;
      break;
    case coNMT_ResetNode:  /* {}  -> coOS_Reset */
      coResetNode(CHP_S);
      break;
    case coNMT_ResetCommunication:
      (void)coResetCommunication(CHP_S);
      break;
  }
}

/* 
*   A Node Guard Request message (an RTR) was received from the Master.
*   We should transmit a response and record the current time (for use
*   in Life Guarding).
*/
static void handleNMTNodeGuarding(CH_S) {
	if (CV(producerHeartBeatTime) == 0){
		(void)transmitHeartbeatMessage(CHP false);
		RV(lastNodeGuardRequest) = kfhalReadTimer();
#if LIFE_GUARDING
		RV(lifeGuardingIsAlive) = true;
#endif
	}
}

/* Transmit a startup message / heartbeat message / node guard response message.
* The message will be placed in the queue and lastHeartBeatTime is assigned.
* Until the message is actually transmitted, heartBeatQueued is non-zero.
* The caller has to check that flag and the time.
* Returns 0 if success.
*
* We are always called by coResetCommunication() (regardless of the value
* of heartBeatQueued).
*
* If the Node Guard Protocol is active (intProducerHeartbeatTime == 0),
* we should toggle bit 7, if the Heartbeat protocol is active
* (intProducerHeartbeatTime != 0), bit 7 should always be 0. In any case,
* the toggle is done for each transmitted message.
*/
static CoStackResult transmitHeartbeatMessage(CH bool isBootMsg) {
  uint8 msg[1];
  CanHalCanStatus stat;
  if (RV(opState) == coOS_Initialising ||  /* For the Bootup Message */
      RV(opState) == coOS_Stopped ||
      RV(opState) == coOS_Operational ||
      RV(opState) == coOS_PreOperational) {
    msg[0] = RV(opState);
	if( !isBootMsg){
    	if (CV(producerHeartBeatTime) == 0 && RV(nodeGuardToggle))
      		msg[0] |= 0x80;
    	RV(nodeGuardToggle) = !RV(nodeGuardToggle);
	}
    /* We record the time for the transmission, but it might be updated when the
     * callback function is called.
     * This might bw right or wrong: if we want heartbeats to be transmitted with
     * an exact period, we can only synchronize the time of queuing.
     */
    RV(heartBeatQueued) = 1;
    stat = coWriteMessage(CHP CO_HEARTBEAT_COBID(RV(nodeId)), msg, 1, TAG_HEARTBEAT);
    if (stat != CANHAL_OK) {
      // The transmission failed
      RV(heartBeatQueued) = 0;
      return (getCanHalError(stat));
    }

    RV(lastHeartBeatTime) = kfhalReadTimer();
    return(coRes_OK);
  } else
    return (coRes_ERR_GEN_HEARTBEAT);   /* Could not transmit heartbeat becuase we're not in valid state */
}

/* 
*  transmitSyncMessage is called by the stack if the node is configured to be 
*  a sync producer. 
*/
static CoStackResult transmitSyncMessage(CH_S) {
  uint8 msg[1];
  CanHalCanStatus stat;
  if (RV(opState) == coOS_Operational ||
      RV(opState) == coOS_PreOperational) {
    msg[0] = RV(opState);
    RV(syncQueued) = 1;
    stat = coWriteMessage(CHP CV(syncCOBID), msg, 1, TAG_SYNC );
    if (stat != CANHAL_OK){
      return (getCanHalError(stat));
	}
    RV(lastSyncTime) = RV(lastSyncTime) + RV(intSyncPeriod);
	if( kfhalReadTimer() - RV(lastSyncTime) > RV(intSyncPeriod)){
		RV(lastSyncTime) = kfhalReadTimer(); /* Will be more accurate in the callback. */
	}
  } else
    return (coRes_ERR_GEN_SYNC); /* Could not generate sync due to operational state */
  return(coRes_OK);
}

/* 
*   handlePDO is called, by the stack, if a PDO is being received. This function 
*   may not be called from the application. 
*/
static void handlePDO(CH int i, uint8 *buf, uint8 dlc) {
  /* We have received a PDO message.
  * We can do one of two things. Either pass it to the application as it is,
  * or scan the PDO mapping and notify the application for each changed object.
  * The first method is useful for better performance if dynamic PDO mapping
  * isn't allowed; in this case, the application can be assumed to know the
  * contents of all PDO's.
  * The second method is necessary if dynamic PDO mapping is supported
  * (unless the application itself breaks down the PDO into objects).
  */
  coPDO *pdo = &CV(PDO)[i];
  uint8 pdoLenBytes = BITSBYTES(CV(PDO)[i].dataLen);
  CoPDOaction action;

  /* Record the time. To be more accurate, the message should contain the */
  /* exact time of reception. Used for RPDO timout monitoring */
  pdo->lastMsgTime = kfhalReadTimer();
#if( USE_RPDO_TIMEOUT_MONITORING)
  if( CV(PDO)[i].eventTimer){
	  CV(PDO)[i].status |= PDO_STATUS_RPDO_MONITORED;
  }
#endif

  /*
  *   Plenware think that it is a better idea to have this nofification here.
  */
  action = appIndicatePDOReception(CHP pdo->pdoNo, buf, BITSBYTES(pdo->dataLen));
  if (action == coPA_Abort) {
    return;
  }
#if MULTIPLEXED_PDO
  /*  
  *   Check if this PDO is configured to be a DAM or SAM PDO, 
  */
  if (checkBM(pdo->status, PDO_STATUS_MPDO)) {
    (void)handleMPDO(CHP buf, dlc);  /* handleMPDO takes care of both DAM and SAM PDO. */
  } else {
#endif 
    if (pdo->mapIdx != PDO_MAP_NONE) {
      /* 
      * If too much data is received, the excess bytes are skipped.
      * If too few bytes are received, it is an error and the message is
      * dropped.
      */
      if (dlc < pdoLenBytes) {
        coEmergencyError(CHP CO_EMCY_MONITORING_PROTOCOL_LENGTH_ERROR);
        return;
      }
/*  
 *  This notification has moved.
 *
      action = appIndicatePDOReception(CHP pdo->pdoNo, buf, BITSBYTES(pdo->dataLen));
      if (action == coPA_Abort)
        return;
*/
      if (action == coPA_Continue) {
  #if CO_DIRECT_PDO_MAPPING
        if (pdo->dmFlag) {
          int j;
          for (j=0; j < pdoLenBytes; j++) {
            *pdo->dmDataPtr[j] = buf[j];
          }
          for (j=0; j<pdo->dmNotifyCount; j++) {
            (void)coODWriteNotify(CHP pdo->dmNotifyObjIdx[j], pdo->dmNotifySubIdx[j], pdo->dmNotifyGroup[j]);
          }
        } else
  #endif
        {
          const coPdoMapItem *m = &PDO_MAP[pdo->mapIdx+1];
          uint32 val;
          coErrorCode res;
          uint8 bit = 0;
          while (CO_PDOMAP(m)) {
			  if( m->len <= 32){
				val = buf2val_bit(buf, bit, m->len);
				res = coWriteObject(CHP m->objIdx, m->subIdx, val);
			  } else if( m->len == 64){
				res = coWriteObjectP(CHP m->objIdx, m->subIdx, buf, 8);
			  } else {
				res = coRes_CAN_ERR;
			  }
			  if( res != CO_OK){
				coError(CHP LOC_CO_ERR_PDO, 6, 0, 0, 0 );
			  }
			bit += m->len;
			m++;
          }
        }
      }
      appNotifyPDOReception(CHP pdo->pdoNo);
    }
#if MULTIPLEXED_PDO
  }
#endif
}

/* 
*   handlePDORequest is called if a PDO-RTR message has been received. 
*/
static void handlePDORequest(CH int i) {
  (void)transmitPDO(CHP i);
}

#if MULTIPLEXED_PDO
/*
*   This function simply builds the databytes of the multiplexed PDO.
*/
CoStackResult packMPDOToBuf(coMPDOFrameDataT pdo, uint8 *buf, uint8 len) {
  CoStackResult res = coRes_OK;
  if (len >= 8) {
    if (pdo.type == destAddrMode) {
      buf[0] = pdo.addr|DESTINATION_ADDRESSING_MODE_FLG;
    } else if (pdo.type == srcAddrMode) {
      buf[0] = pdo.addr;
    } else {
      res = coRes_ERR_MPDO_TYPE_UNKNOWN;
    }
    if (!res) {
      val2buf(pdo.objIdx, buf+1, 2);
      buf[3] = pdo.subIdx;
      val2buf(pdo.dataField, buf+4, 4);
      res = coRes_OK;
    }
  } else {
    res = coRes_ERR_PACK_MPDO_BUF_LEN;
  }
  return res;
}

/*
*   This function checks if it is OK to TX the PDO now
*   or if it is maybe not allowed due to inhitib time
*   for example.  
*/
CoStackResult checkTransmitStatusPDO(CH int i) {
  KfHalTimer elapsedTime;
  CoStackResult res = coRes_OK;
  coPDO *pdo = &CV(PDO)[i];

  if (!(pdo->status & PDO_STATUS_SUB_DISABLED)) {
    if (pdo->status & PDO_STATUS_INHIBIT_ACTIVE) {
      if (pdo->status & PDO_STATUS_QUEUED) {
        res = coRes_ERR_TPDO_INHIBIT_TIME_NOT_ELAPSED;
      } else {
        elapsedTime = kfhalReadTimer() - pdo->lastMsgTime;
        if (elapsedTime < pdo->inhibitTime) {
          res = coRes_ERR_TPDO_INHIBIT_TIME_NOT_ELAPSED;
        } 
      }
    } 
  } else {
    res = coRes_ERR_PDO_DISABLED;
  }
  return res;
}

/*
*   This function sends the physical Multiplexed CAN message on the bus.
*   that is given by the argument.
*/
CoStackResult transmitMPDO(CH int i, coMPDOFrameDataT mpdo) {
  CoStackResult res = coRes_ERR_TPDO_NOT_OPERATIONAL;
  coPDO *pdo = &CV(PDO)[i];
  uint8 buf[8];

  if (RV(opState) == coOS_Operational) {
    res = coRes_OK;
  }
  if (res == coRes_OK) {
    res = packMPDOToBuf(mpdo, buf, sizeof(buf));
    if (res == coRes_OK) {
      res = checkTransmitStatusPDO(CHP i);
      if (res == coRes_OK) {
        uint32 tag;
        setBM(pdo->status, PDO_STATUS_QUEUED);
        if(pdo->inhibitTime) {
          setBM(pdo->status, PDO_STATUS_INHIBIT_ACTIVE);
        }
		tag = ((uint32)i << 16) + pdo->pdoNo;
        res = getCanHalError(coWriteMessage(CHP pdo->COBID, buf, 8, tag));
      }
    }
  }
  return res;
}


/*
*  This API call is used by the application when a 
*  Destination Address Mode PDO should be sent. 
*/
CoStackResult coTransmitDAMPDO(CH uint16 pdoNo, uint8 nodeId, uint16 objIdx, uint8 subIdx) {
  CoStackResult res = coRes_ERR_TPDO_NOT_FOUND;
  int i = findPDO(CHP pdoNo, PDO_TX);
  if (IS_POSITIVE(i)) {
    uint16 mapIdx = CV(PDO)[i].mapIdx;
   /*
    *   Look for the first mapped object for this PDO.
    */
    if (mapIdx != PDO_MAP_NONE) {
      uint32 val;
      coErrorCode err;
#if DYNAMIC_PDO_MAPPING > 0
      coPdoMapItem *m = &PDO_MAP[mapIdx+1];
#else
	  const coPdoMapItem *m = &PDO_MAP[mapIdx+1];
#endif
      val = coReadObject(CHP m->objIdx, m->subIdx, &err);
      if (err == CO_OK) {
        if (CV(PDO)[i].status & PDO_STATUS_DAM_PDO) {
          coMPDOFrameDataT pdo;
          pdo.type = destAddrMode;
          pdo.addr = nodeId;
          pdo.objIdx = objIdx;
          pdo.subIdx = subIdx;
          pdo.dataField = val;
          res = transmitMPDO(CHP i, pdo);
        } else {
          res = coRes_ERR_TPDO_IS_NOT_DAM_PDO;
        }
      }
    } else {
      res = coRes_ERR_NO_MAPPED_OBJ_DAM_PDO;
    }
  } else {
    res = coRes_ERR_TPDO_NOT_FOUND; 
  }
  return res;
}



/*
*  This API call is used by the application when a 
*  Destination Address Mode PDO should be sent.
*/
CoStackResult coTransmitDAMPDOEx(CH uint16 pdoNo, uint8 nodeId, uint16 objIdx, uint8 subIdx, uint32 val) {
  CoStackResult res;
  int i = findPDO(CHP pdoNo, PDO_TX);
  if (IS_POSITIVE(i)) {
    if (CV(PDO)[i].status & PDO_STATUS_DAM_PDO) {
      coMPDOFrameDataT pdo;
      pdo.type = destAddrMode;
      pdo.addr = nodeId;
      pdo.objIdx = objIdx;
      pdo.subIdx = subIdx;
      pdo.dataField = val;
      res = transmitMPDO(CHP i, pdo);
    } else {
      res = coRes_ERR_TPDO_IS_NOT_DAM_PDO;
    }
  } else {
    res = coRes_ERR_TPDO_NOT_FOUND; 
  }
  return res;
}

#endif
/* 
*   The application can call this function to build and transmit a PDO.
*   The function checks the transmission type for the PDO, if OK, 
*   If successful the function transmitPDO() is called and that 
*   is the function that assembles the physical CAN message.
*/
CoStackResult coTransmitPDO(CH uint16 pdoNo) {
  CoStackResult res = coRes_ERR_TPDO_NOT_FOUND;
  int i = findPDO(CHP pdoNo, PDO_TX);
  if (IS_POSITIVE(i)) {
    uint8 pdoTraType = CV(PDO)[i].transmissionType;
    if (pdoTraType == CO_PDO_TT_SYNC_ACYCLIC) {
      CV(PDO)[i].status |= SYNC_TX_PDO_EVENT;
      res = coRes_OK;
    } else if ((pdoTraType >= CO_PDO_TT_SYNC_CYCLIC_MIN && pdoTraType <= CO_PDO_TT_SYNC_CYCLIC_MAX) || pdoTraType == CO_PDO_TT_SYNC_RTR) {
      res = coRes_OK;
    } else {
      res = transmitPDO(CHP i); 
    }
  } else {
    res = coRes_ERR_TPDO_NOT_FOUND;
  }
  return res;
}


/* 
*   coRequestPDO is a fuction call for the application to trigg the remote node 
*   to put it's PDO onto the CAN-bus. 
*/
CoStackResult coRequestPDO(CH uint16 pdoNo) {
  CoStackResult res = coRes_ERR_TPDO_NOT_FOUND;
  int i = findPDO(CHP pdoNo, PDO_RX);
  if (IS_POSITIVE(i)) {
    cobIdT id = CV(PDO)[i].COBID;
    CanId cid;
    if (id & cobPDOinvalid)
      res = getCanHalError(CANHAL_ERR_NOMSG);
    cid = id & CANHAL_ID_MASK;
    if (id & cobExtended) {
      cid |= CANHAL_ID_EXTENDED;
    }
    cid |= CANHAL_ID_RTR;
    res = getCanHalError(canWriteMessageEx(RV(canChan), cid, NULL, BITSBYTES(CV(PDO)[i].dataLen), 0));
  }
  return res;
}

/*
*   coPeriodicService is a routin that preferably is called by a timer-interupt
*   routine periodicaly, as often as you system can afford for best accurency on
*   timeouts aswell as the generation os SYNC-frames. 
*/
void coPeriodicService(CH_S){
  KfHalTimer now = kfhalReadTimer();
  int i;
  CanHalCanStatus stat;
  uint16 txErr = 0;
  uint16 rxErr = 0;
  uint16 ovErr = 0;

  /* Check receive overrun */
  stat = canGetErrorCounters( RV(canChan), &txErr, &rxErr, &ovErr); 
  if( ovErr != RV(overrunCnt)){
	if( ovErr > RV(overrunCnt)){
		/* Dangerous if sending the EMCY may trig another overrun, did cause problems with the Kvaser driver */
		//coEmergencyErrorEx(CHP CO_EMCY_MONITORING_COM_CAN_OVERRUN_ERROR, ovErr, ovErr >> 8, ovErr & 0xFF, 0, 0, 0);
	}
	RV(overrunCnt) = ovErr;
  }
#if( USE_RPDO_TIMEOUT_MONITORING)
  for (i = 0; i < PDO_MAX_COUNT; i++){
    if (CV(PDO)[i].dir == PDO_RX) {
		if( CV(PDO)[i].eventTimer && (CV(PDO)[i].status & PDO_STATUS_RPDO_MONITORED) && now - CV(PDO)[i].lastMsgTime >= kfhalMsToTicks( CV(PDO)[i].eventTimer)){
			CV(PDO)[i].status &= ~PDO_STATUS_RPDO_MONITORED;
			appIndicateRpdoTimeout(CHP CV(PDO)[i].pdoNo);
		}
	}
  }
#endif

#if (USE_INHIBIT_TIMES|USE_TPDO_EVENT_TIMER)
  for (i = 0; i < PDO_MAX_COUNT; i++){
    if (CV(PDO)[i].dir == PDO_TX) {
#if USE_INHIBIT_TIMES
      if ((CV(PDO)[i].status & (PDO_STATUS_INHIBIT_ACTIVE|PDO_STATUS_QUEUED)) == PDO_STATUS_INHIBIT_ACTIVE) {  /* Is inhibit active but not queued? */
        if ((now-CV(PDO)[i].lastMsgTime) >= CV(PDO)[i].inhibitTime){
          CV(PDO)[i].status &= ~PDO_STATUS_INHIBIT_ACTIVE; /* clear inhibit active. */
        }
      }
#endif
#if USE_TPDO_EVENT_TIMER
      /* MikkoR 2005-01-24: Event timer must be obeyed here! Otherwise PDO is build (but not send)
         in every call to coPeriodicService(), while eventTimer has elapsed, but inhibit this has not. */
      if (CV(PDO)[i].eventTimer && (CV(PDO)[i].transmissionType == 254 || CV(PDO)[i].transmissionType == 255) && !(CV(PDO)[i].status & (PDO_STATUS_INHIBIT_ACTIVE|PDO_STATUS_QUEUED))) {
        if ((now-CV(PDO)[i].lastMsgTime) >= kfhalMsToTicks( CV(PDO)[i].eventTimer)) {
          /*
          *   The timer has elapsed, make sure that it is not in que either.
          */
          if (!(CV(PDO)[i].status & PDO_STATUS_QUEUED)) {
            (void)transmitPDO(CHP i);
          }
        }
      }
#endif
    }
  }
#endif

  if (CV(producerHeartBeatTime) && !RV(heartBeatQueued)) {
    /*
    *   We are configured to send heartbeats.
    */
    if (now-RV(lastHeartBeatTime) >= RV(intProducerHeartbeatTime)) {
      /* printf("heartbeat!"); */
      (void)transmitHeartbeatMessage(CHP false);
    }
	}
/*
 *  Sync producer implementaiton, if intSyncPeriod == 0 then we shall not
 *  generate SYNC-messages
 */
  if(CV(syncPeriod) && !RV(syncQueued) && RV(intSyncPeriod)){
    if ((now-RV(lastSyncTime)) > RV(intSyncPeriod)) {
      (void)transmitSyncMessage(CHP_S);
    }
  }


#if NODEGUARD_MASTER

  /* 
   * Send node guarding requests RTR for monitored nodes (MASTER FUNCTIONALITY)
   */

  for (i = 0; i < MAX_GUARDED_NODES; i++) {
	  /* Entries with nodeId 0 are not in use */
	  if( RV(guardedNodes)[i].nodeId == 0){
		continue;
	  }

	  if( now - RV(guardedNodes)[i].lastNodeGuardSendTime > RV(guardedNodes)[i].guardTimeTicks) {
		comNodeGuardRequest(CHP RV(guardedNodes)[i].nodeId);
		RV(guardedNodes)[i].lastNodeGuardSendTime = now;
		if( !RV(guardedNodes)[i].isNodeguardingActive){
		  RV(guardedNodes)[i].isNodeguardingActive = true;
 		  RV(guardedNodes)[i].lastNodeGuardRecvTime = now; /* Reset counter */
		}
	  }
  }

  /* 
   * Check for timed out nodes (MASTER FUNCTIONALITY)
   */
  for (i = 0; i < MAX_GUARDED_NODES; i++) {
	  /* Entries with nodeId 0 are not in use */
	  if (RV(guardedNodes)[i].nodeId == 0){
		  continue;
	  }

	  if (RV(guardedNodes)[i].alive) {
		  if (now - RV(guardedNodes)[i].lastNodeGuardRecvTime > RV(guardedNodes)[i].guardTimeTicks * RV(guardedNodes)[i].lifeTimeFactor) {
			  uint8 nodeId = RV(guardedNodes)[i].nodeId;
			  /*
			   *   A timeout has occured. This should be indicated to application.
			   */
			  RV(guardedNodes)[i].alive = false;
			  RV(guardedNodes)[i].state = coOS_Unknown;
			  appIndicateNodeGuardFailure(CHP nodeId);
		  }
	  }
  }
#endif

/* 
 * Check that heartbeats have been received for all monitored nodes
 */
#if HEARTBEAT_CONSUMER
  for (i = 0; i < MAX_HEARTBEAT_NODES; i++) {
	  /* Entries with nodeId 0 is not in use */
	  if( CV(heartbeatNodes)[i].nodeId == 0 || CV(heartbeatNodes)[i].nodeId > 0x7F || CV(heartbeatNodes)[i].heartBeatTimeout == 0){
		continue;
	  }
    /*
    *   The heartbeat is specificed to start running when the first heart beat
    *   has been received. If we are polling the heartbeats (nodeguarding)
    *   we will timeout if we do not receive the heartbeat
    *   (nodeguarding is simply a way to request the heartbeats
    *   manually.
    */
    if (CV(heartbeatNodes)[i].isHeartBeatControlRunning) {
      if (CV(heartbeatNodes)[i].heartBeatTimeout && now-CV(heartbeatNodes)[i].lastHeartbeatTime > CV(heartbeatNodes)[i].heartBeatTimeout) {
        uint8 nodeId = CV(heartbeatNodes)[i].nodeId;
       /*
        *   A timeout has occured. This should be indicated to application.
        */
        CV(heartbeatNodes)[i].alive = false;
        CV(heartbeatNodes)[i].state = coOS_Unknown;
        if (nodeId > 0 && nodeId < 128) {
          /*
          *   Indicate to the application that the heartbeat for the given node Id 
          *   suddenly dissapered. Then disable the guarding. The gurading will 
          *   re-initalize itself if the heartbeat is back on the bus again
          *   (as described by the CANopen specification for guarding heartbeats)
          */
          appIndicateNodeGuardFailure(CHP nodeId);
		  coEmergencyError(CHP CO_EMCY_MONITORING_COM_LIFE_GUARD_HB_ERROR);

#if ERROR_BEHAVIOR
		switch(CV(CommunicationError)){
			case 0:
				 if( RV(opState)== coOS_Operational){
					RV(opState) = coOS_PreOperational; 
				  }
				break;
			case 1: 
				break;
			case 2: 
				RV(opState) = coOS_Stopped;
				break;
			default:
				break;
		}
#else
		RV(opState) = coOS_PreOperational; 
#endif
          CV(heartbeatNodes)[i].isHeartBeatControlRunning = false; 
        }
      } 
    } 
  }
#endif  /* HEARTBEAT_CONSUMER */
#if LIFE_GUARDING
  {
	uint32 NodeLifeTime =  CV(guardTime) * CV(lifeTimeFactor);
	if( (NodeLifeTime != 0 ) && (RV(lifeGuardingIsAlive) == true) && (now - RV(lastNodeGuardRequest) > kfhalMsToTicks( NodeLifeTime)) ){
		/* Life guarding event state: occurred, reason : timeout */
		appIndicateLifeGuardingEvent(CHP LG_STATE_OCCURRED);
		RV(lifeGuardingIsAlive) = false;

		coEmergencyError(CHP CO_EMCY_MONITORING_COM_LIFE_GUARD_HB_ERROR);

#if ERROR_BEHAVIOR
		switch(CV(CommunicationError)){
			case 0:
				 if( RV(opState)== coOS_Operational){
					RV(opState) = coOS_PreOperational; 
				  }
				break;
			case 1: 
				break;
			case 2: 
				RV(opState) = coOS_Stopped;
				break;
			default:
				break;
		}
#endif	
	}
  }
#endif /* LIFE_GUARDING */


  for (i=0 ; i < SDO_MAX_COUNT; i++) {
    if ((CV(SDO)[i].type != SDO_UNUSED) && (CV(SDO)[i].status != 0)) {
      now = kfhalReadTimer();
      if((now - CV(SDO)[i].timer) > CO_SDO_TIMEOUT) {
#if BOOTUP_DS302
        bool   isValidBootData = false;
        bool   isNodeBooting;
        uint8  otherNodeId;
        uint16 objIdx;
        uint8  subIdx;
        uint32 val;

        /* Check if node is currently busy booting. */
        isNodeBooting = RV(isInstanceBooting);
        if (isNodeBooting) {
          objIdx =  CV(SDO)[i].objIdx;
          subIdx =  CV(SDO)[i].subIdx;
          otherNodeId = CV(SDO)[i].otherNodeId;
          val = CO_ERR_SDO_PROTOCOL_TIMED_OUT;
          isValidBootData = handleReceivedBootData(CHP otherNodeId, objIdx, subIdx, val, false);
        }
        if(!isValidBootData || !isNodeBooting) {
          /* 
          *   Notify app if we are not booting or if the SDO (used for something else) timed out. 
          */
          appTransferErrSDO(CHP CV(SDO)[i].sdoNo, CV(SDO)[i].objIdx, CV(SDO)[i].subIdx, CO_ERR_SDO_PROTOCOL_TIMED_OUT);
        }
#else
        appTransferErrSDO(CHP CV(SDO)[i].sdoNo, CV(SDO)[i].objIdx, CV(SDO)[i].subIdx, CO_ERR_SDO_PROTOCOL_TIMED_OUT);
#endif
        /* 
        * Clean up the SDO and notify remote node that error has occured. 
        */
        //printf("\n\nnow: %lx, timer: %lx", now, CV(SDO)[i].timer);
        abortTransferSDO(CHP i, CV(SDO)[i].objIdx, CV(SDO)[i].subIdx, CO_ERR_SDO_PROTOCOL_TIMED_OUT);
      }
    }
  }
#if BOOTUP_DS302
  if (RV(isInstanceBooting)) {
    superviseBootDS302(CHP_S);
  }
#endif
}

/* 
*   Function returns the no of "ticks" that is left until the inhibit time
*   has elapsed 
*/
#if  USE_INHIBIT_TIMES
KfHalTimer GetRemaingInhibitT(CH int i){
    coPDO *pdo = &CV(PDO)[i];
    if (pdo->status & PDO_STATUS_INHIBIT_ACTIVE) { /* Is inhibit active? */
      if (pdo->status & PDO_STATUS_QUEUED)
        return pdo->inhibitTime;
      else {
        KfHalTimer elapsedTime = (kfhalReadTimer()-(pdo->lastMsgTime));
        if (elapsedTime < pdo->inhibitTime)
          return pdo->inhibitTime-elapsedTime;
        else
          return 0;
      }
    } else
      return 0;
}
#endif



#if  USE_INHIBIT_TIMES
/* Application variant that returns the no of "ticks" that is left until the inhibit time
 * has elapsed for the specific PDO.*/
KfHalTimer coGetRemaingInhibitT(CH uint16 pdoNo){
  int i = findPDO(CHP pdoNo, PDO_TX);
  if(i >= 0){
    return(GetRemaingInhibitT(CHP i)); /*Returns InhibitTime, else 0 if not used */
  }
  else
  {
    return(0);   /* PDO not found */
  }
}
#endif

/* Transmit a PDO[i]. Returns 0 on success.
 * UH defined errorcodes:
 * TPDO_ERR_NOT_OPERATIONAL : Node not in operational mode.
 * TPDO_ERR_NO_MAP_FOUND : No mapping for the PDO.
 * TPDO_ERR_APP_ABORT : Aborted by the application.
 * TPDO_ERR_OBJECT_NOT_FOUND: Objectindex/Subindex not found.
 * TPDO_CAN_ERR : CAN-hardware err.
 * TPDO_INHIBIT_TIME_NOT_ELAPSED: The inhibit timer has not elapsed.
 * canERR_NOMSG : If the ID of the PDO is invalid.
 *  0 : Everything ok.
*/
static CoStackResult transmitPDO(CH int i){
  uint8 buf[8+8]; /* make some extra room should appReadObject() return too much data */
  coPDO *pdo = &CV(PDO)[i];
  uint8 dlc;
  CoPDOaction action;
  KfHalTimer elapsedTime;      /* Local variable for elapsed time since last PDO. */
  uint32 tag;
  CoStackResult stackRes;

  if (RV(opState) != coOS_Operational)
    return (coRes_ERR_TPDO_NOT_OPERATIONAL);

  dlc = BITSBYTES(pdo->dataLen);
  action = appIndicatePDORequest(CHP pdo->pdoNo);    /* Ask appl if ok to transmit PDO */

  /* Assemble the message */
  if ((pdo->mapIdx == PDO_MAP_NONE) && (action == coPA_Continue)){
    /* MikkoR 2005-01-24: Set inhibit time active bit, since PDO is being sent here too. */
    if(pdo->inhibitTime)
        pdo->status |= PDO_STATUS_INHIBIT_ACTIVE;

    /* No map found, send a PDO with dlc = 0 */
	tag = ((uint32)i << 16) + pdo->pdoNo;
    stackRes = getCanHalError(coWriteMessage(CHP pdo->COBID, buf, 0, tag)); /*lint !e603   Symbol 'buf' (line 2122) not initialized */
    if(stackRes == coRes_OK){                                               /* Not a problem, DLC is equal to 0. */
        /* MikkoR 2005-01-21: lastMsgTime must be stored for inhibit time to have any effect! */
        pdo->lastMsgTime = kfhalReadTimer();
    }
    return stackRes;
  }

  if (action == coPA_Abort)
    return (coRes_ERR_TPDO_APP_ABORT);

  if (action == coPA_Continue) {
#if CO_DIRECT_PDO_MAPPING
    if (pdo->dmFlag) {
      int j;
      for (j = 0; j < pdo->dmNotifyCount; j++)
        (void)coODReadNotify(CHP pdo->dmNotifyObjIdx[j], pdo->dmNotifySubIdx[j], pdo->dmNotifyGroup[j]);
      for (j = 0; j < dlc; j++)
        buf[j] = *pdo->dmDataPtr[j];
    } else
#endif
    {
      const coPdoMapItem *m = &PDO_MAP[pdo->mapIdx+1];
      uint8 bit = 0; /* counter of written bits to CAN-message */
      uint32 val;
      coErrorCode err;
      while (CO_PDOMAP(m)) { 
        if (bit + m->len > 8*8) /* already copied written + current < 64 ? */
          coError(CHP LOC_CO_ERR_PDO, 5, 0, 0, 0);
		if( m->len <= 32){
	        val = coReadObject(CHP m->objIdx, m->subIdx, &err);  /* read current object */
        	if (err)
          		return (coRes_ERR_TPDO_OBJECT_NOT_FOUND);
			val2buf_bit(val, m->len, buf, bit);
		} else if( m->len == 64){  
			err = coReadObjectP(CHP m->objIdx, m->subIdx, buf, sizeof(buf));
			if( err){
				return (coRes_ERR_TPDO_OBJECT_NOT_FOUND);
			}
		} else {
			return coRes_CAN_ERR;
		}
        bit += m->len;
        m++;
      }
      /* If the number of bits isn't a multiple of 8, we might have to clear the unused bits. */
    }
  }
#if DYNAMIC_PDO_MAPPING
  if (pdo->status & PDO_STATUS_SUB_DISABLED)
    return(coRes_ERR_PDO_DISABLED);
#endif
  if (pdo->status & PDO_STATUS_INHIBIT_ACTIVE) {  /* Is inhibit active? */
    if (pdo->status & PDO_STATUS_QUEUED)
      return (coRes_ERR_TPDO_INHIBIT_TIME_NOT_ELAPSED);
    elapsedTime = (kfhalReadTimer()-(pdo->lastMsgTime)); /* Inhibit active */
    if (elapsedTime < pdo->inhibitTime)
      return(coRes_ERR_TPDO_INHIBIT_TIME_NOT_ELAPSED);  /* err-code for inhibit times has not elapsed */
  }
  pdo->status |= PDO_STATUS_QUEUED;
  if(pdo->inhibitTime)
    pdo->status |= PDO_STATUS_INHIBIT_ACTIVE;
  tag = ((uint32)i << 16) + pdo->pdoNo;
  stackRes = getCanHalError(coWriteMessage(CHP pdo->COBID, buf, dlc, tag));
  if(stackRes == coRes_OK){
    /* MikkoR 2005-01-21: lastMsgTime must be stored for inhibit time to have any effect! */
    pdo->lastMsgTime = kfhalReadTimer();
  }
  else {
    pdo->status &= ~PDO_STATUS_QUEUED;
  }
  return stackRes;
}

/*
*   coSetState is used to set the node-state for the specifc remote node.
*/

CoStackResult coSetState(CH coOpState opState){
  RV(opState) = opState;
  return(coRes_OK);
}

/*
*   This function is used to read the operational state for this node.
*/
coOpState coGetState(CH_S){
  return(RV(opState));
}


/*
*   This function makes sure that the expected toggles state is the same as 
*   the toggle state of the received CAN frame.
*/
bool isToggleOk(uint16 sdoStatusFlags, uint8 canMessageFlags) {
  bool isToggleOK = false;
  if (sdoStatusFlags & SEG_TRANSFER_TOGGLE) {
    if (canMessageFlags & TOGGLE_MASK) {
      isToggleOK = true;
    }
  } else {
    if (!(canMessageFlags & TOGGLE_MASK)) {
      isToggleOK = true;
    }
  }
  return isToggleOK;
}

/*
*   SDO-Server -> Handle received segment from client.
*   ==================================================
*   This function is being called if a transfer is running and
*   a segment has been received from the client. 
*/
coErrorCode srvHandleSegmentDirectionToServer(CH int i, uint8 *buf) {
  coErrorCode res;
  //uint8 commandFromSdoServer = 0;     // Removed by PN
  coSDO *sdoP = &CV(SDO)[i];
  uint8 respBuf[CAN_MESSAGE_LEN];

  if (sdoP->status & SRV_SEG_DL_INITIATED) {
    bool isToggleOK = isToggleOk(sdoP->status, buf[0]);
    if (isToggleOK) {
      res = handleReceivedDataInSegmentedSDO(CHP i, buf);
      if (res == CO_OK) {
        /*
        *   We have successfully received a proper segment from a SDO client
        *   and the data has been accepted by the application. Send a command
        *   back to the client with the proper contents.
        */
        memset(respBuf, 0, CAN_MESSAGE_LEN);
        if (sdoP->status & SEG_TRANSFER_TOGGLE) {
          respBuf[0] = ((uint8)(SRV_RESPONSE_SEGM_FROM_CLI_TO_SRV << XSC_SHIFT)) | TOGGLE_MASK;;
        } else {
          respBuf[0] = (uint8)(SRV_RESPONSE_SEGM_FROM_CLI_TO_SRV << XSC_SHIFT);
        }
        (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, CAN_MESSAGE_LEN, 0);
        // qqq Whatif the message transmission fails? Retransmit, abort or ignore?
        /*
        *   Check if the tranfer will continue or if the remote node expect that 
        *   this was the last frame to be sent.
        */
        if (buf[0] & C_MASK) {
          sdoP->status = 0; 
        } else {
          sdoP->status ^= SEG_TRANSFER_TOGGLE; 
          sdoP->timer = kfhalReadTimer();
        }
      } 
    } else {
      res = CO_ERR_TOGGLE_BIT_NOT_ALTERED;
    }
  } else {
    res = CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT;
  }
  /*
  *   qqq: TBD: Move this abort and use the return value in handleSDO()
  *        and make the stack abort from there.
  */
  if (res != CO_OK) {
    abortTransferSDO(CHP i, sdoP->objIdx, sdoP->subIdx, res);
  }
  return res;
}

/*
*  SDO-Server -> Handle a write-request from client.
*  =================================================
*  This function handles a transfer request, from client,
*  saying that it want to our the contents of our ("the server")
*  Object Dictionary.
*/
coErrorCode srvHandleTransferRequestDirectionToServer(CH int i, uint8 *buf) {
  coErrorCode res = CO_ERR_GENERAL;
  CoObjLength objLenInBytes;
  uint16 objIdx = (uint16) buf2val(&buf[1], 2);
  uint8  subIdx = buf[3];
  uint32 val;
  coSDO *sdoP = &CV(SDO)[i];
  uint8 respBuf[CAN_MESSAGE_LEN];
  /*
  *   Remote node tries to start a download to our local object dictionary or
  *   a object held by the application. Check if it is expedited or segmented 
  *   protocol. DS301, see p. 9-20.
  */
  if (buf[0] & EXPEDITED_MASK) {
//    coErrorCode err = CO_ERR_GENERAL;
    CoObjLength locObjSizeInBytes;
	/*
    *   Expedited write to our local object dictionary.
    */
    if (buf[0] & SIZE_MASK) {
      objLenInBytes = 4 - ((buf[0] & INITIATE_N_MASK) >> INITIATE_N_MASK_SHIFT );
    } else {
      objLenInBytes = 4;
    }
    locObjSizeInBytes = BITSBYTES((CoObjLength)coGetObjectLen(CHP objIdx, subIdx, &res));
    if( objLenInBytes != locObjSizeInBytes) {
      abortTransferSDO(CHP i, objIdx, subIdx, CO_ERR_OBJECT_LENGTH);
	  return CO_ERR_OBJECT_LENGTH;
	}
    val = buf2val(buf+4, (uint8)objLenInBytes);
    res = coWriteObject(CHP objIdx, subIdx, val);
    if (res == CO_OK) { 
      memset(respBuf, 0, CAN_MESSAGE_LEN);
      respBuf[0] = SRV_RESPONSE_INIT_TRANSF_FROM_CLI_TO_SRV << XSC_SHIFT;
      memcpy(&respBuf[1], &buf[1], 3);
      (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, CAN_MESSAGE_LEN, 0);
      // qqq Whatif the message transmission fails? Retransmit, abort or ignore?
      CV(SDO)[i].timer = kfhalReadTimer();
    } else {
      abortTransferSDO(CHP i, objIdx, subIdx, res);
    }
  } else {
    /*
    *   Segmented transfer to our local OD.
    */
    uint8 locAttr = 0;
    uint32 rmtObjSize = 0;
    coErrorCode err = CO_ERR_GENERAL;
    if (buf[0] & SIZE_MASK) {
      rmtObjSize = buf2val(&buf[4], 4);
    } else {
      rmtObjSize = NO_SIZE;
    }
    locAttr = coGetObjectAttr(CHP objIdx, subIdx);
    if (locAttr & CO_OD_ATTR_WRITE) {
      CoObjLength locObjSizeInBytes = BITSBYTES((CoObjLength)coGetObjectLen(CHP objIdx, subIdx, &err));
      if ((rmtObjSize != NO_SIZE) && (rmtObjSize <= locObjSizeInBytes)) {
        sdoP->status |= SRV_SEG_DL_INITIATED; /* Segmented download initiated */
        sdoP->status &= ~SEG_TRANSFER_TOGGLE;
        sdoP->objToTransLen = rmtObjSize; /* N.o. bytes expected. */
        sdoP->objIdx = objIdx;
        sdoP->subIdx = subIdx;
        sdoP->timer = kfhalReadTimer();
        sdoP->bufPos = 0;
        sdoP->pos = 0;
        memset(respBuf, 0, CAN_MESSAGE_LEN);
        respBuf[0] = SRV_RESPONSE_INIT_TRANSF_FROM_CLI_TO_SRV << XSC_SHIFT;
        memcpy(&respBuf[1], &buf[1], 3);
        (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, CAN_MESSAGE_LEN, 0);            
        // qqq Whatif the message transmission fails? Retransmit, abort or ignore?
        err = CO_OK;
      } else {
        err = CO_ERR_OBJECT_LENGTH;
      }
    } else {
      err = CO_ERR_OBJECT_READ_ONLY;
    }
    if (err) {
      abortTransferSDO(CHP i, objIdx, subIdx, err);
    }
  }
  return res;
}

/*
*   SDO-Server -> Handle a read request from our OD
*   ===============================================
*   We have received an request from the remote client that he wants
*   to read from our object dicitonary. If we are lucky our response
*   will be given in one single frame (expedited transfer) but that 
*   requires that the size of this object, in our OD, fits into the
*   epedited frame (4 bytes or 32 bits).   
*/
coErrorCode srvHandleTransferRequestDirectionFromServer(CH int i, uint8 *buf) {
  uint16 objIdx = (uint16) buf2val(&buf[1], 2);
  uint8 subIdx = buf[3];
  coErrorCode res = CO_OK;
  CoObjLength objLen = coGetObjectLen(CHP objIdx, subIdx, &res);
  uint32 val;
  coSDO *sdoP = &CV(SDO)[i];
  uint8 respBuf[CAN_MESSAGE_LEN];

  /*
   * Set object index and subindex to SDO so that correct error frame can be
   * sent in case of an error.
   */
  sdoP->objIdx = objIdx;
  sdoP->subIdx = subIdx;

  if (res != CO_OK) {
    abortTransferSDO(CHP i, sdoP->objIdx, sdoP->subIdx, res);
    return res;
  }

  /*
  *   Figure out that size of the object. If it is in the range of 0-4 bytes
  *   the response can be done immidately, else segmented transfer has to be 
  *   used.
  *   If segmented transfer is used, but the object fit into the local SDO
  *   transfer buffer - copy the current content of the asked for object to
  *   this buffer.
  */
  if ((0 < objLen) && (objLen <= 32)) {
    /*
    *   Expedited transfer response can be given.
    */
    val = coReadObject(CHP objIdx, subIdx, &res);
    if (res == CO_OK) {
      /*
      *   We could successfully read the contents of the requested object from our
      *   object dictionary. Jam it into the expedited responseframe.
      */
      memset(respBuf, 0, CAN_MESSAGE_LEN);
      respBuf[0] = ((SRV_RESPONSE_INIT_TRANSF_FROM_SRV_TO_CLI << XSC_SHIFT) |
                    (EXPEDITED_MASK | SIZE_MASK)) + (uint8)((uint8)(4-(objLen/8)) << 2);
      memcpy(&respBuf[1], &buf[1], 3);
      val2buf(val, &respBuf[4], (uint8)(objLen/8));
      (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, 8, 0);
      // qqq Whatif the message transmission fails? Retransmit, abort or ignore?
      CV(SDO)[i].timer = kfhalReadTimer();
    }
  } else if (objLen > 32) {
    /*
    *   We need to send the object in segments. If the object fit into the
    *   local buffer of the SDO, copy it to that location.
    */
    if (objLen <= CONV_FROM_BYTES(SIZE_LOCAL_SDO_BUF)) {
      /*
      *   Copy the current contents of the object to the local SDO buffer.
      */
      res = coReadObjectP(CHP objIdx, subIdx, sdoP->localBuf, sizeof(sdoP->localBuf));
    } else {
      res = CO_OK; /* We will fetch the data from the application. */
    }
    if (res == CO_OK) {
      sdoP->status |= SRV_SEG_UL_INITIATED;
      sdoP->status &= ~SEG_TRANSFER_TOGGLE;
      sdoP->objToTransLen = BITSBYTES(objLen);
      sdoP->objIdx = objIdx;
      sdoP->subIdx = subIdx;
      sdoP->pos = 0;
      sdoP->bufPos = 0;
      sdoP->timer = kfhalReadTimer();
      respBuf[0] = ((SRV_RESPONSE_INIT_TRANSF_FROM_SRV_TO_CLI << XSC_SHIFT) & ~EXPEDITED_MASK) | SIZE_MASK;
      memcpy(&respBuf[1], &buf[1], 3);
      val2buf(BITSBYTES(objLen), &respBuf[4], 4); /* The SDO protocol always talk i bytes */
      (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, CAN_MESSAGE_LEN, 0);
      // qqq Whatif the message transmission fails? Retransmit, abort or ignore?
    } 
  } else {
    res = CO_ERR_OBJECT_DOES_NOT_EXISTS;
  }
  if (res != CO_OK) {
     abortTransferSDO(CHP i, sdoP->objIdx, sdoP->subIdx, res);
  }
  return res;
}


/*
*  SDO-Server -> Handle a "please send be a new segment" request from client
*  =========================================================================
*  This function handles the segment request that has been received from
*  the client (the client is receiving data from our (server) object
*  dictionaty).
*/
coErrorCode srvHandleSegmentRequestDirectionToServer(CH int i, uint8 *buf) {
  CoSegTranAction seg_action;
  coErrorCode res;
  uint8 seg_n;
  coSDO *sdoP = &CV(SDO)[i];
  uint8 respBuf[CAN_MESSAGE_LEN];

  /*
  *   The remote node is reading from our local object dictionary and the 
  */
  if (sdoP->status & SRV_SEG_UL_INITIATED) {
    bool isToggleOK = isToggleOk(sdoP->status, buf[0]);
    if (isToggleOK) {
      memset(respBuf, 0, CAN_MESSAGE_LEN);
      res = constructTransmitDataInSegmentedSDO(CHP i , respBuf, &seg_n, &seg_action);
      if (res == CO_OK) {
        /*
        *   We have successfully received data from either the OD automatically or from the
        *   application. Figure out if the stack it self, or the application that maybe
        *   holds this object for us, thinks that this specific slice of data or if the
        *   transfer shall continue in the future.
        */
        bool isOK = false;
        respBuf[0] = (uint8)(SRV_RESPONSE_SEGM_FROM_SRV_TO_CLI << XSC_SHIFT) | ((uint8)(seg_n << SEG_N_MASK_SHIFT));
        if (sdoP->status & SEG_TRANSFER_TOGGLE) {
          respBuf[0] |= TOGGLE_MASK;
        } else {
          respBuf[0] &= ~TOGGLE_MASK;
        }
        if (seg_action == CO_SEG_CONTINUE) {
          respBuf[0] &= ~C_MASK; /* clear c-flag in SEG_UP_RSP since we will continue */
          sdoP->status ^= SEG_TRANSFER_TOGGLE;
          sdoP->timer = kfhalReadTimer();
          isOK = true;
        } else if (seg_action == CO_SEG_DONE) {
          respBuf[0] |= C_MASK;
          sdoP->status = 0;
          isOK = true;
        } else {
          res = CO_ERR_GENERAL;
        }
        if (isOK) {
          (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, CAN_MESSAGE_LEN, 0);
          // qqq Whatif the message transmission fails? Retransmit, abort or ignore?
        }
      } 
    } else {
      res = CO_ERR_TOGGLE_BIT_NOT_ALTERED;
    } 
  } else {
    res = CO_ERR_GENERAL;
  }
  if (res) {
    abortTransferSDO(CHP i, sdoP->objIdx, sdoP->subIdx, res);
  }
  return res;
}

/*
*  SDO-Server -> Handle a abortframe sent from the client.
*  ======================================================
*  This function handles as received abortframe sent from
*  the client SDO in the remote node.
*/
coErrorCode srvHandleReceivedAbortFromClient(CH int i, uint8 *buf) {
  uint16 objIdx = (uint16) buf2val(&buf[1], 2);
  uint8 subIdx = buf[3];
  coErrorCode coErrorCodeFromClient;
  coSDO *sdoP = &CV(SDO)[i];
  if (sdoP->objIdx == objIdx && sdoP->subIdx == subIdx) {
    coErrorCodeFromClient =  buf2val(buf+4, 4); /* res = error code */
    appTransferErrSDO(CHP sdoP->sdoNo, objIdx, subIdx, coErrorCodeFromClient);
    sdoP->status = 0;    
  } else {
	coErrorCodeFromClient = CO_ERR_GENERAL;
    appTransferErrSDO(CHP sdoP->sdoNo, objIdx, subIdx, CO_ERR_GENERAL);
    sdoP->status = 0;
  }
  return coErrorCodeFromClient;
}


#if SDO_CLIENT
/*
*  SDO-Client -> Handle a abortframe sent from the client.
*  ======================================================
*  Handle that a requested segment has been received from the server. 
*  This is good because this is what we are waiting for in this
*  state.
*/
coErrorCode cliHandleSegmentResponseOnSegmentRequest(CH int i, uint8 *buf) {
  coSDO *sdoP = &CV(SDO)[i];
  coErrorCode res;  
  uint8 respBuf[CAN_MESSAGE_LEN];
  uint8 validBytesInSegment;

  if (sdoP->status & CLI_SEG_UL_INITIATED) {
    bool isToggleOK = isToggleOk(sdoP->status, buf[0]);
    if (isToggleOK) {
      //      res = handleReceivedDataInSegmentedSDO(CHP i, buf);
      validBytesInSegment = 7 - ((buf[0] & SEG_N_MASK) >> SEG_N_MASK_SHIFT);
      
      if (validBytesInSegment > 7) {
        validBytesInSegment = 7;  /* Make sure that we are in range, maybe the CAN-msg is corrupt. */
      }
      /*
      *   Check if the "c-flag" which is set by the remote node if it thinks that there 
      *   will be more data to download after this frame or if the transfer is finished.
      */
      if (buf[0] & C_MASK) {
        uint8 sdoNo = sdoP->sdoNo;
        uint16 objIdx = sdoP->objIdx;
        uint8 subIdx = sdoP->subIdx;
        CoObjLength pos = sdoP->pos;
        
        sdoP->status = 0;

        res = appReadObjectResponseSDO(CHP sdoNo, objIdx, subIdx, 
          (buf+1), pos, validBytesInSegment, CO_SEG_DONE);
      } else {
        /*
        *   If the specific SDO is a client this means that it is a download going on to the application
        *   and not the local object dictioinary.
        */
        res = appReadObjectResponseSDO(CHP sdoP->sdoNo, sdoP->objIdx, sdoP->subIdx, 
          (buf+1), sdoP->pos, validBytesInSegment, CO_SEG_CONTINUE);
        if (res == CO_OK) {
          sdoP->pos += validBytesInSegment;
          /*
          *   Toggle - and change the toggle. The client is always first with the toggle.
          */
          memset(respBuf, 0, CAN_MESSAGE_LEN);
          sdoP->status ^= TOGGLE_MASK;
          if (sdoP->status & TOGGLE_MASK) {
            respBuf[0] = (CLI_REQUEST_SEGM_FROM_SRV_TO_CLI << XSC_SHIFT) | TOGGLE_MASK;
          } else {
            respBuf[0] = (CLI_REQUEST_SEGM_FROM_SRV_TO_CLI << XSC_SHIFT);
          }
          
          (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, CAN_MESSAGE_LEN, 0);              
          // qqq Whatif the message transmission fails? Retransmit, abort or ignore?
        }
      } 
    } else {
      res = CO_ERR_TOGGLE_BIT_NOT_ALTERED;
    }
  } else {
    res = CO_ERR_GENERAL;
  }
  if (res) {
    abortTransferSDO(CHP i, sdoP->objIdx, sdoP->subIdx, res);
  }
  return res;
}

/*
*  SDO-Client -> Handle the acknowledge frame from server on a sent segment to server..
*  ==================================================================================
*  This frames handles the acknowledge frame that is sent from
*  the server when we, the client, are writing data to 
*  the object dictionary of server.
*/
coErrorCode cliHandleSegmentResponseOnSentSegment(CH int i, uint8 *buf) {
  CoSegTranAction seg_action;
  coSDO *sdoP = &CV(SDO)[i];
  uint8 respBuf[CAN_MESSAGE_LEN];
  coErrorCode res;
  uint8 seg_n;

  if (sdoP->status & CLI_SEG_DL_INITIATED) {
    /*
    *   Segmented transfer is running.
    */
    bool isToggleOK = isToggleOk(sdoP->status, buf[0]);
    if (isToggleOK) {
      memset(respBuf, 0, CAN_MESSAGE_LEN);
      sdoP->status ^= SEG_TRANSFER_TOGGLE;
      res = constructTransmitDataInSegmentedSDO(CHP i, respBuf, &seg_n, &seg_action);
      if (sdoP->status & SEG_TRANSFER_TOGGLE) {
        respBuf[0] = (CLI_REQUEST_SEGM_FROM_CLI_TO_SRV << XSC_SHIFT) | TOGGLE_MASK | (seg_n << SEG_N_MASK_SHIFT);
      } else {
        respBuf[0] = (CLI_REQUEST_SEGM_FROM_CLI_TO_SRV << XSC_SHIFT) | (seg_n << SEG_N_MASK_SHIFT);
      }
      if (seg_action == CO_SEG_DONE) {
        respBuf[0] |= C_MASK;
        sdoP->status |= CLI_SEG_DL_DONE_INDICATED;
        sdoP->status &= ~CLI_SEG_DL_INITIATED; /* Only waiting for our last acknowledge! */
      }
      (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, CAN_MESSAGE_LEN, 0);
    } else {
      res = CO_ERR_TOGGLE_BIT_NOT_ALTERED;
    }
  } else if (sdoP->status & CLI_SEG_DL_DONE_INDICATED) {
    /*
    *   Segmented transfer is running but all we expect now is acknowledge that the 
    *   transfer is finished and no new data.
    */
    bool isToggleOK = isToggleOk(sdoP->status, buf[0]);
    if (isToggleOK) {
	  /*
	  *		Copy the contents of the sdo params and release the sdo ASAP.
	  */
      uint8 tempvalid = 0;
	  uint8 sdoNo = sdoP->sdoNo;
	  uint16 objIdx = sdoP->objIdx;
	  uint8	subIdx = sdoP->subIdx;
	  CoObjLength pos = sdoP->pos;
      CV(SDO)[i].status = 0;  /* Transmission done */
      (void)appWriteObjectResponseSDO(CHP sdoNo, objIdx, subIdx, NULL, pos, &tempvalid, 0);          
      res = CO_OK;
    } else {
      res = CO_ERR_TOGGLE_BIT_NOT_ALTERED;
    }
  } else {
    res = CO_ERR_GENERAL;
  }
  if (res) {
    abortTransferSDO(CHP i, sdoP->objIdx, sdoP->subIdx, res);
  }
  return res;

}


/*
*  SDO-Client -> Handle acknowledge on transfer request from server (server->client transfer)
*  ================================================================
*  In this state we are waiting for a init transfer and we have just received a 
*  COBID with, hopefully, the proper contents. It can either be a expedited 
*  response (meaning that there will be no segmented transfers, or it can
*  be an ack from the server that a segmented transfer will start since the
*  size of this object has a size bigger than 4 bytes. 
*/
coErrorCode cliHandleSegmentResponseOnInitTransferFromServer(CH int i, uint8 *buf) {
  coErrorCode res;  // May produce a warning depending on stack configuration
  uint32 val;       // May produce a warning depending on stack configuration
  uint16 objIdx = (uint16) buf2val(&buf[1], 2);
  uint8  subIdx = buf[3];
  CoObjLength objLen;
  coSDO *sdoP = &CV(SDO)[i];
  uint8 respBuf[CAN_MESSAGE_LEN];
  uint8 tempBuf[CAN_MESSAGE_LEN];
  uint8 sdoNo = sdoP->sdoNo;

  if ((sdoP->status & CLI_INITIATE_UL_REQUESTED) && (!(sdoP->status & CLI_SEG_UL_INITIATED))) {
    if (sdoP->objIdx == objIdx && sdoP->subIdx == subIdx) {
      if (buf[0] & EXPEDITED_MASK) {
        if (buf[0] & SIZE_MASK) {
          objLen = 4 - ((buf[0] & INITIATE_N_MASK) >> INITIATE_N_MASK_SHIFT);
        } else {
          objLen = 4;
        }
        val = buf2val(buf+4, (uint8)objLen);
        // Copy the values so that we can release the SDO
        memset(tempBuf, 0, CAN_MESSAGE_LEN);
        memcpy(tempBuf, buf+4, objLen);
#if BOOTUP_DS302
        /* 
        *  Check if the current instance of the stack is working in DS302 boot. 
        */
        if (RV(isInstanceBooting)) {
          int   nId = sdoP->otherNodeId;
          bool  isValidBootData = handleReceivedBootData(CHP nId, objIdx, subIdx, val, false);
          CV(SDO)[i].status = 0; // Message is taken care of either boot data or copied
          if (!isValidBootData) {
            /* 
            *   The received data was not aimed for the boot, forward to application 
            */
            (void)appReadObjectResponseSDO(CHP sdoNo, objIdx, subIdx, tempBuf, 0, (uint8)objLen, CO_SEG_DONE);  /* qqq Debug this for sure. */
          } 
        }
        else {
          // Normal operation
#endif
        CV(SDO)[i].status = 0; // Message is taken care of either boot data or copied
        (void)appReadObjectResponseSDO(CHP sdoNo, objIdx, subIdx, tempBuf, 0, (uint8)objLen, CO_SEG_DONE);  /* qqq Debug this for sure. */
#if BOOTUP_DS302
        }
#endif
        res = CO_OK;
      } else {
        /* 
        *   The response was in segmented format.; 
        */
        if (buf[0] & SIZE_MASK) {
          objLen = buf2val(buf+4, 4);
        } else {
          objLen = 0;
        }
        CV(SDO)[i].objToTransLen = objLen;
        res = appReadObjectResponseSDO(CHP sdoP->sdoNo, objIdx, subIdx, (buf+4), objLen, 0, CO_SEG_INIT_DATA); /* qqq Fixed */
        /* 
        *   Check if app. can handle the  data it earlier requested to upload. 
        */
        if (res == CO_OK) {
          memset(respBuf, 0, CAN_MESSAGE_LEN);
          if (sdoP->status & TOGGLE_MASK) {
            respBuf[0] = (CLI_REQUEST_SEGM_FROM_SRV_TO_CLI << XSC_SHIFT) | TOGGLE_MASK;
          } else {
            respBuf[0] = CLI_REQUEST_SEGM_FROM_SRV_TO_CLI << XSC_SHIFT;
          }
          (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, CAN_MESSAGE_LEN, 0);
          // qqq Whatif the message transmission fails? Retransmit, abort or ignore?
          sdoP->status &= ~CLI_INITIATE_UL_REQUESTED;
          sdoP->status |= CLI_SEG_UL_INITIATED;
          sdoP->timer = kfhalReadTimer();  /* qqq handle timeout */
        } else {
          abortTransferSDO(CHP i, objIdx, subIdx, res);
        }
      }
    }  else {
      res = CO_ERR_GENERAL;
    }
  } else {
    res = CO_ERR_GENERAL;
  }
  if (res) {
    abortTransferSDO(CHP i, objIdx, subIdx, res);
  }
  return res;
}

/*
*  SDO-Client -> Handle the response from the server in a init
*                transfer from client to server. 
*  ===========================================================
*  In this state we are waiting for acknowledge a transfer request
*  from client to server. This response can either be a response 
*  ("acknowledge") from the server becuase the write from the 
*  client was in expedited mode OR it can be a "please start 
*  transfer your string now" ("segmented") from the server.
*/
coErrorCode cliHandleSegmentResponseOnInitTransferToServer(CH int i, uint8 *buf) {
  coErrorCode res = CO_ERR_GENERAL;
  CoSegTranAction seg_action;
  uint8 seg_n;
  uint16 objIdx = (uint16) buf2val(&buf[1], 2);
  uint8 subIdx = buf[3];
  coSDO *sdoP = &CV(SDO)[i];
  uint8 respBuf[CAN_MESSAGE_LEN];

  if ((sdoP->status & (CLI_EXPEDITED_DL_REQUESTED | CLI_SEG_DL_REQUESTED)) &&
      !(sdoP->status & CLI_SEG_DL_INITIATED)) {
    if (sdoP->objIdx == objIdx && sdoP->subIdx == subIdx) {
      if (sdoP->status & CLI_EXPEDITED_DL_REQUESTED) {
        uint8 tempvalid = 0;
        uint8 sdoNo = sdoP->sdoNo;
        sdoP->status = 0;
        (void)appWriteObjectResponseSDO(CHP sdoNo, objIdx, subIdx, 0, 0, &tempvalid, 0);
        res = CO_OK;
      } else if (sdoP->status & CLI_SEG_DL_REQUESTED) {
        sdoP->status &= ~CLI_SEG_DL_REQUESTED;
        sdoP->status |= CLI_SEG_DL_INITIATED;

        memset(respBuf, 0, CAN_MESSAGE_LEN);
        res = constructTransmitDataInSegmentedSDO(CHP i, respBuf, &seg_n, &seg_action);
        respBuf[0] = CLI_REQUEST_SEGM_FROM_CLI_TO_SRV << XSC_SHIFT;
        respBuf[0] |= (uint8)(seg_n << SEG_N_MASK_SHIFT);
        if (res == CO_OK) {
          if (seg_action == CO_SEG_CONTINUE || seg_action == CO_SEG_DONE) {
            if (seg_action == CO_SEG_CONTINUE) {
              respBuf[0] &= ~C_MASK;
              sdoP->status &= ~CLI_EXPEDITED_DL_REQUESTED;
              sdoP->status |= CLI_SEG_DL_INITIATED;
            } else if (seg_action == CO_SEG_DONE) {
              respBuf[0] |= C_MASK;
              sdoP->status |= CLI_SEG_DL_DONE_INDICATED;
              sdoP->status &= ~CLI_EXPEDITED_DL_REQUESTED;
              sdoP->status &= ~CLI_SEG_DL_INITIATED;
            }
            (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, CAN_MESSAGE_LEN, 0);
            // qqq Whatif the message transmission fails? Retransmit, abort or ignore?
          } else {
            res = CO_ERR_GENERAL;
          }
        } 
      }
    } else {
      res = CO_ERR_GENERAL;
    }
  } else {
    res = CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT;
  }
  if (res) {
    abortTransferSDO(CHP i, objIdx, subIdx, res);
  }
  return res;
}

/*
*  SDO-Client -> Handle the response from the server in a init
*                transfer from client to server. 
*  ===========================================================
*  This function handles a received abortframe from the server.
*/
coErrorCode cliHandleReceivedAbortFromServer(CH int i, uint8 *buf) {
  coErrorCode res = CO_ERR_GENERAL;
  uint16 objIdx = (uint16) buf2val(&buf[1], 2);
  uint8 subIdx = buf[3];
  coSDO *sdoP = &CV(SDO)[i];

  if ((sdoP->objIdx == objIdx) && (sdoP->subIdx == subIdx)) {
    /*
	*	The contents of the received abortframe was ok!
	*/
	res =  buf2val(buf+4 , 4); /* val = Length of file */
    sdoP->status = 0;
#if BOOTUP_DS302
    /* Check if the current instance of the stack is working in DS302 boot. */
    if (RV(isInstanceBooting)) {
      int   nId = sdoP->otherNodeId;
      bool  isValidBootData = handleReceivedBootData(CHP nId, objIdx, subIdx, res, true);
      if (!isValidBootData) {
        /* The received data was not aimed for the boot, forward to application */
#endif
        appTransferErrSDO(CHP sdoP->sdoNo, objIdx, subIdx, res);
#if BOOTUP_DS302
      }
    } else {
	  /*
	  *		Instance is not booting and a abortframe has been
	  *		received by the application. Bugfix UH 050125.
	  */
	  appTransferErrSDO(CHP sdoP->sdoNo, objIdx, subIdx, res);
	}
#endif
  } else {
    CV(SDO)[i].status = 0;
    appTransferErrSDO(CHP sdoP->sdoNo, objIdx, subIdx, CO_ERR_GENERAL);
  }
  return res;
} 

#endif

/*
*   handleSDO(..) handles what has to be taken care of if a SDO message
*   has been received. Currently we only support expedited and 
*   segmented transfers.
*/
static void handleSDO(CH int i, uint8 *buf, uint8 dlc) {
  coErrorCode res;  // May produce a warning depending on stack configuration

  /* The SDO must have 8 databytes */

  if (dlc == 8) { 
    if (CV(SDO)[i].type == SDO_SERVER) { 
      /*
      *   The received frame corresponds to 
      *   to a configured identifier for a server-SDO.
      *   Get the command from the sending SDO client and try to
      *   find out what is going on.
      */
      uint8 commandFromSdoClient = buf[0] >> 5;
      if (commandFromSdoClient == CLI_REQUEST_SEGM_FROM_CLI_TO_SRV) {
        res = srvHandleSegmentDirectionToServer(CHP i, buf);
      } else if (commandFromSdoClient == CLI_REQUEST_INIT_TRANSF_FROM_CLI_TO_SRV) {
        res = srvHandleTransferRequestDirectionToServer(CHP i, buf);
      } else if (commandFromSdoClient == CLI_REQUEST_INIT_TRANSF_FROM_SRV_TO_CLI) {
        res = srvHandleTransferRequestDirectionFromServer(CHP i, buf);
      } else if (commandFromSdoClient == CLI_REQUEST_SEGM_FROM_SRV_TO_CLI) {
        res = srvHandleSegmentRequestDirectionToServer(CHP i, buf);
      } else if (commandFromSdoClient == XCS_ABORTCODE) {
        res = srvHandleReceivedAbortFromClient(CHP i, buf);
      } else {
		uint16 objIdx = (uint16) buf2val(&buf[1], 2);
  		uint8 subIdx = buf[3];
		abortTransferSDO(CHP i, objIdx, subIdx, CO_ERR_UNKNOWN_CMD);

        /*  
        *     qqq: Todo: Unsupported commmand. Typically block transfer requests
        *          will end up here.
        */
      }
  #if SDO_CLIENT
    } else if (CV(SDO)[i].type == SDO_CLIENT) {
      /*
      *   The received frame was sent form a server to one of the SDO clients
      *   on this node. Get the command. Since it is a server the received
      *   command will only be a response to a request for the corresponding
      *   client SDO.
      */
      uint8 commandFromSdoServer = buf[0] >> 5;
      if (commandFromSdoServer == SRV_RESPONSE_SEGM_FROM_SRV_TO_CLI) {
        res = cliHandleSegmentResponseOnSegmentRequest(CHP i, buf);
      } else if (commandFromSdoServer == SRV_RESPONSE_SEGM_FROM_CLI_TO_SRV) {
        res = cliHandleSegmentResponseOnSentSegment(CHP i, buf);
      } else if (commandFromSdoServer == SRV_RESPONSE_INIT_TRANSF_FROM_SRV_TO_CLI) {
        res = cliHandleSegmentResponseOnInitTransferFromServer(CHP i, buf);
      } else if (commandFromSdoServer == SRV_RESPONSE_INIT_TRANSF_FROM_CLI_TO_SRV) {
        res = cliHandleSegmentResponseOnInitTransferToServer(CHP i, buf);
      } else if (commandFromSdoServer == XCS_ABORTCODE) {
        res = cliHandleReceivedAbortFromServer(CHP i, buf);
      } else {
		uint16 objIdx = (uint16) buf2val(&buf[1], 2);
  		uint8 subIdx = buf[3];
		abortTransferSDO(CHP i, objIdx, subIdx, CO_ERR_UNKNOWN_CMD);
        /*  
        *     qqq: Todo: Unsupported commmand. Typically block transfer requests
        *          will end up here.
        */
      }
  #endif 
    }
  }
}
/* Transmit a client SDO Abort Message */
CoStackResult coAbortTransferSDO(CH uint8 sdoNo, uint16 objIdx, uint8 subIdx, coErrorCode err){
	int i = findSDO(CHP sdoNo, SDO_CLIENT);

	if( i == -1){
		return coRes_ERR_UNDEFINED_SDO_NO;
	}
	abortTransferSDO(CHP i, objIdx, subIdx, err);	
	return coRes_OK;
}

/* Transmit a SDO Abort Message for SDO[i]. */
STATIC void abortTransferSDO(CH int i, uint16 objIdx, uint8 subIdx, uint32 err) {
  uint8 respBuf[CAN_MESSAGE_LEN];
  coSDO *sdoP = &CV(SDO)[i];
  sdoP->status = 0; 
  sdoP->pos = 0;
  sdoP->bufPos = 0;

  memset(respBuf, 0, CAN_MESSAGE_LEN);
  respBuf[0] = XCS_ABORTCODE << XSC_SHIFT;
  val2buf(objIdx, &respBuf[1], 2);
  respBuf[3] = subIdx;
  val2buf(err, &respBuf[4], 4);
  (void)coWriteMessage(CHP sdoP->COBID_Tx, respBuf, CAN_MESSAGE_LEN, 0);
  // qqq Whatif the message transmission fails? Retransmit, abort or ignore?
  /*
  *  Notify the application on the error.
  */
  appTransferErrSDO(CHP sdoP->sdoNo, objIdx, subIdx, err);
} 

/* 
*   coGetNodeIdSDO is used to read out the node ID that a specific SDO is communicating with. 
*/
uint8 coGetNodeIdSDO(CH uint8 sdoNo, uint8 type){
  int i;
  i = findSDO(CHP sdoNo,type);
  if (( 0<=i ) && (i < SDO_MAX_COUNT)) {
    return CV(SDO)[i].otherNodeId;
  } else {
    return 0;
  }
}

/* 
*   coSetNodeId is used to read out the node Id for this node. 
*/
uint8 coGetNodeId(CH_S){
  return RV(nodeId);
}

/* 
*   coSetNodeId is used to setup the node Id for the specific node. 
*/
CoStackResult coSetNodeId(CH uint8 nodeId){
  if ((nodeId > 0) && (nodeId <= 128)) {
    RV(nodeId) = nodeId;
    return coRes_OK;
  }
  return coRes_ERR_NODE_ID_OUT_OF_RANGE;
}


/* 
*   An internal error, or error in the application setup, has been discovered.
*   Execution can't continue, so we just pass it on to the application and stop.
*/
STATIC void coError(CH uint16 errType, uint8 pos, uint16 param1, uint8 param2, uint32 param3){
  appError(CHP errType, pos, param1, param2, param3);
/*  for (;;)
    ;*/
}

/*
*   coEmergencyError simply wraps coEmergencyErrorEx to be backwards compatible.
*/
void coEmergencyError(CH uint16 errCode) {
//  coEmergencyErrorEx(CHP errCode, 0, 0, 0, 0, 0, 0);  
}

/* 
*   Transmit an Emergency Error message on the CAN bus.
*   The message will contain the error code and the value of the Error Register.
*   This is the "Ex"tended version of the API call with support for 
*   manufacturer specific error field.
*/
void coEmergencyErrorEx(CH uint16 errCode, uint16 additionalInfo, uint8 b1, uint8 b2, uint8 b3, uint8 b4, uint8 b5) {
#if EMERGENCY_PRODUCER_HISTORY
  int i;
#endif
  uint8 msgBuf[CAN_MESSAGE_LEN];
  cobIdT cid;
  CanId halCid;

  if (RV(opState) == coOS_Operational || RV(opState) == coOS_PreOperational) {
    // Add the error code to the Pre-defined Error Field vector
	CV(errorRegister) |= CO_ERREG_GENERIC_ERROR;
    if( errCode < CO_EMCY_GENERIC_ERRORS) {
		/* Error reset or no error -> Do not save */
	} else if( errCode < CO_EMCY_CURRENT_ERRORS){ // 0x1XXX generic error
		CV(errorRegister) |= CO_ERREG_GENERIC_ERROR;  
	} else if( errCode < CO_EMCY_VOLTAGE_ERRORS){ // 0x2XXX current error
		CV(errorRegister) |= CO_ERREG_CURRENT;  
	} else if( errCode < CO_EMCY_TEMPERATURE_ERRORS){ // 0x3XXX voltage error
		CV(errorRegister) |= CO_ERREG_VOLTAGE;  
	} else if( errCode < CO_EMCY_HARDWARE_ERRORS){ // 0x4XXX temperature error
		CV(errorRegister) |= CO_ERREG_TEMPERATURE; 
	} else if( errCode >= CO_EMCY_MONITORING_COMMUNICATION_ERRORS && errCode < CO_EMCY_MONITORING_PROTOCOL_ERRORS){
		CV(errorRegister) |= CO_ERREG_COMM;
	}

	memset(msgBuf, 0, CAN_MESSAGE_LEN);
    val2buf(errCode, msgBuf, 2);
    msgBuf[2] = CV(errorRegister);
    msgBuf[3] = b1;
    msgBuf[4] = b2;
    msgBuf[5] = b3;
    msgBuf[6] = b4;
    msgBuf[7] = b5;

	/* Need to send directly to CAN driver as the coWrite function may generate
	 * en EMCY and we would easily get into a recursive loop.
	 */
    cid = long2cob(CV(emcyCOBID));
    halCid = cid & CANHAL_ID_MASK;
	if (cid & cobExtended){
      halCid |= CANHAL_ID_EXTENDED;
	}
    canWriteMessageEx(RV(canChan), halCid, msgBuf, CAN_MESSAGE_LEN, 0);

//	(void)coWriteMessage(CHP long2cob(CV(emcyCOBID)), msgBuf, CAN_MESSAGE_LEN, 0);

#if EMERGENCY_PRODUCER_HISTORY
    /*
    *  Copy the this error into slot 0 of this list and the rest 
    *  should be shifted one single step. We will have to begin
    *  to do the shifting and when we are done we write the lastest 
    *  error to the array. The oldest error will be thrown away.
    *
    *  qqq: A ring buffer with a good old write and read pointer
    *       would be more effiecent (if we store more then 5-10 
    *       abort codes. This will be slow if application want to
    *       store thousands of emcy codes but I do not think 
    *       someone ever will. :-) 5 is normal I think.
    */
    i = MAX_EMCY_PRODUCER_HISTORY_RECORDED-1;
    while (i > 0) {
      CV(emcyProdHistList)[i] = CV(emcyProdHistList)[i-1];
      i--;
    }
    /*
    *   Copy the data to the slot first in line! (subIdx 1)!
    */
    CV(emcyProdHistList)[0].errorCode = errCode;
    CV(emcyProdHistList)[0].additionalInfo = additionalInfo;
    CV(emcyProdHistList)[0].manufSpecField[0] = b1; /* Manufacurer specific byte 1 */
    CV(emcyProdHistList)[0].manufSpecField[1] = b2;
    CV(emcyProdHistList)[0].manufSpecField[2] = b3;
    CV(emcyProdHistList)[0].manufSpecField[3] = b4;
    CV(emcyProdHistList)[0].manufSpecField[4] = b5; /* Manufacurer specific byte 5 */
#endif
  }
}

#if EMERGENCY_PRODUCER_HISTORY
/*
*   This function is being called if someone want to read object 0x1003.
*   Object 0x1003 is described on p.9-65 of DS301. 
*/
static uint32 readNodeEmcyHist(CH uint8 subIdx, coErrorCode *err) {
  uint8   i = 0;
  uint32  res = 0;
  *err = CO_ERR_GENERAL;
  if (subIdx == 0) {
    bool lastSlotFound = false;
    uint16 errCode;
    uint16 addiInfo;
    /*
    *   Read of subIdx 0 means that application want to read the number
    *   of records that are stored in our array.
    */
    i = 0;
    while (!lastSlotFound && (i < MAX_EMCY_PRODUCER_HISTORY_RECORDED)) {
      errCode = CV(emcyProdHistList)[i].errorCode;
      addiInfo = CV(emcyProdHistList)[i].additionalInfo;
      if (errCode == 0 && addiInfo == 0) {
        lastSlotFound = true;
      } else {
        i++;
      }
    }
    res = (uint32)i;
    *err = CO_OK;
  } else {
    /*
    *   Read the given subIdex if it exists.
    */
    i = subIdx - 1;
    if (i < MAX_EMCY_PRODUCER_HISTORY_RECORDED) {
      uint16 errCode = CV(emcyProdHistList)[i].errorCode;
      uint16 additionalInfo = CV(emcyProdHistList)[i].additionalInfo;
      if (errCode||additionalInfo) {
        res = ((uint32)additionalInfo << 16) | errCode;
        *err = CO_OK;
      } else {
        *err = CO_ERR_INVALID_SUBINDEX; // This subIdx is not in use (yet).
      }
    } else {
      *err = CO_ERR_INVALID_SUBINDEX;
    }
  }
  return res;
}

/* 
*   This function is being called if someone want to write to object 0x1003.
*   Object 0x1003 is described on p.9-65 of DS301. 
*/
static coErrorCode writeNodeEmcyHist(CH uint8 subIdx, uint32 val) {
  coErrorCode res = CO_ERR_GENERAL;
  if (subIdx == 0) {
    if (val == 0) {
      int i;
      /*
      * Clear the list of emergency messages that have been sent! 
      */
      for (i=0; i < MAX_EMCY_PRODUCER_HISTORY_RECORDED; i++) {
        CV(emcyProdHistList)[i].errorCode = 0;
        CV(emcyProdHistList)[i].additionalInfo = 0;
      }
      res = CO_OK;
    }  else { 
      /* 
      * val != 0 is not allowed according to specification. 
      */
      res = CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED;
    }
  } else {
    /* 
    *   All subIndxes except 0 are read only.
    */
    res = CO_ERR_OBJECT_READ_ONLY;
  }
  return res;
}
#endif



#if EMERGENCY_CONSUMER_OBJECT
/*
*	  This function is used to find out the index, in the emcyNodes-array, 
*   for the given nodeId.
*/
static int findEmcyListIdx (CH uint8 nodeId) {
  int i = 0;
  int res;
  bool isNodeIdFound = false;
  while ((i < MAX_EMCY_GUARDED_NODES) && !isNodeIdFound) {
    if (CV(emcyNodes)[i].nodeId == nodeId) {
      isNodeIdFound = true;
    } else {
      i++;
    }
  }
  if (isNodeIdFound) {
    res = i;
  } else {
    res = -1;
  }
  return res;
}

/*
*	  This function implements the read of object 0x1028 which is
*   the emergency consumer object. It is described in DS301 p.11-13.
*/
static uint32 readEmcyConsObj(CH uint8 subIdx, coErrorCode *err) {
  uint32 res;
  *err = CO_ERR_GENERAL;
  if (subIdx == 0) {
    res = MAX_EMCY_GUARDED_NODES;
    *err = CO_OK; 
  } else if (subIdx > 0 && subIdx <= MAX_EMCY_GUARDED_NODES) {
    int i;
    uint8 nodeId = subIdx;
    /*
    *   read the EMCY COBID for the specific nodeId.
    */
    i = findEmcyListIdx(CHP nodeId);
    if (IS_POSITIVE(i)) {
      res = (uint32) CV(emcyNodes)[i].COBID;
      *err = CO_OK;
    } else {
      /*
      *   If the nodeId was not found, act as if it was and return that
      *   the EMCY cobid is inactivated (CO_EMCY_CONSUMER_INACTIVE set, rest 0).
      *   This is our "default" value.
      */
      res = CO_EMCY_CONSUMER_INACTIVE;
      *err = CO_OK;
    }
  } else {
    res = 0;    
    *err = CO_ERR_INVALID_SUBINDEX;
  }
  return res;
}

/*
*	  This function implements the write of object 0x1028 which is
*   the emergency consumer object. It is described in DS301 p.11-13.
*/
static coErrorCode writeEmcyConsObj(CH uint8 subIdx, uint32 val) {
  coErrorCode res = CO_ERR_GENERAL;
  if (subIdx == 0) {
    res = CO_ERR_OBJECT_READ_ONLY;
  } else if (subIdx > 0 && subIdx <= MAX_EMCY_GUARDED_NODES) {
    uint8 nodeId;
    int i;
    /*
    *   The given subIdx equals the node Id according to DS301.
    */
    nodeId = subIdx;
    i = findEmcyListIdx(CHP nodeId);
    if (val != CO_EMCY_CONSUMER_INACTIVE) {
      /*
      *   Application want to write some data that we have to remeber.
      */
      if (i >= 0) {
        CV(emcyNodes)[i].COBID = (cobIdT) val;
        res = CO_OK;
      } else {
        /*
        *   If the nodeId was not found, we need to create a new slot.
        *   A unused slot is defined to have the nodeId equal to 0, 
        *   so therefore looking for nodeId = 0 is to look for a free slot.
        */
        i = findEmcyListIdx(CHP 0);
        if (i >= 0) {
          CV(emcyNodes)[i].nodeId = nodeId;
          CV(emcyNodes)[i].COBID = (cobIdT) val;
          res = CO_OK;
        } else {
          /*
          *   We have no room left to store the COBID!
          */
          res = CO_ERR_OUT_OF_MEMORY;
        }
      }
    } else {
      /*
      *   The application wants to inactivate a emcy consumer cobid. 
      */
      i = findEmcyListIdx(CHP nodeId);
      if (i >= 0) {
        while (i < MAX_EMCY_GUARDED_NODES) {
          if (i <= (MAX_EMCY_GUARDED_NODES - 2)) {
            CV(emcyNodes)[i] = CV(emcyNodes)[i+1];
          } else {
            /* 
            *   CV(emcyNodes)[i+1] does not exist any more. 
            */ 
            CV(emcyNodes)[i].nodeId = 0;
            CV(emcyNodes)[i].COBID = 0;
          }
          i++;
        }
        res = CO_OK;
      } else {
        /*
        *  Do nothing. Application is deleting something that does not exist.
        */
        res = CO_OK;
      }
    }
  } else {
    res = CO_ERR_INVALID_SUBINDEX;
  }
  return res;
}
#endif


/* Setting and clearing error conditions must be handled somehow.
* Maybe we could call coEMCYerror(uint16 errCode), where errCode is a value
* from table 21 in 9.2.5.1, and the functions then determines what bit in
* the Error Register to set.
* To clear an error is harder, as Error Codes split the error bits into
* several states, there might be both an Ambient and a Device Temperature error,
* and if only one is cleared, the CO_ERREG_TEMPERATURE bit should still be set.
*/
void coEMCYclearError(CH uint8 err) {
  if (CV(errorRegister) & err) {
    if (err & CO_ERREG_GENERIC_ERROR) {
      // Clear all events
      CV(errorRegister) = 0;
    }
    else {
      CV(errorRegister) &= ~err;
    }

    coEmergencyError(CHP CO_EMCY_ERROR_RESET); // Error Code 'Error Reset'
  }
}


/* Writes a CAN message given a COBID instead of a CANid; some bits needs to
* be translated.
*
* The tag is used to quickly discover what message is meant when a callback
* is received.
* For a PDO, its value is
*  ((uint16)pdoIdx << 16)+pdoNo
* where pdoIdx is the index in PDO[] and pdoNo is the PDO number (1..512)
*
* tag    Function
* ---   ------------------
*  0    No special message
* {0..1023}{1..512}  {0..0x3ff}{1..0x200}  {00000xxx xxxxxxxx 00000xxx xxxxxxxx}
*       A PDO message
*  0x80000001   TAG_HEARTBEAT
*       A HeartBeat message
*/
CanHalCanStatus coWriteMessage(CH cobIdT id, const uint8 *msg, uint8 dlc, uint32 tag) {
  CanId cid;
  CanHalCanStatus stat;
  
  cid = id & CANHAL_ID_MASK;
  if (id & cobExtended)
    cid |= CANHAL_ID_EXTENDED;

  stat = canWriteMessageEx(RV(canChan), cid, msg, dlc, tag);
  if( stat == CANHAL_ERR_OVERFLOW){
  	coEmergencyError(CHP CO_EMCY_MONITORING_COM_CAN_OVERRUN_ERROR);
  }
  return stat;
}

/* This is the CAN callback function. */
STATIC unsigned char coCanCallback(CanChannel chan, CanCbType cbType, const CANMessage *msg) {
 
  CoRuntimeData *ch = &coRuntime[0];  // May produce a warning depending on stack configuration
  uint16 txErr;
  uint16 rxErr;
  uint16 ovErr;

#if CO_MULTIPLE_INSTANCES
  int chIdx=0;
  while (chIdx < CO_INST_MAX) {
    if (coInstancesInUse[chIdx]){
      ch = &coRuntime[chIdx];
      if(RV(canChan)==chan){
        break; /* The correct stack instance has been found */
      }
      chIdx++;
      if(chIdx == CO_INST_MAX)
        return(0); /* qqq: configured callback channel was not found. Report an error? */
    }
  }
#else
  ch = &coRuntime[0]; /* We know it is the correct channel as we have opened only one channel. */
#endif
  if (cbType == CANHAL_CB_TX_DONE && msg->tag) {
    if (msg->tag == TAG_HEARTBEAT) {
      RV(lastHeartBeatTime) = kfhalReadTimer();
      RV(heartBeatQueued) = 0;
    } else if(msg->tag == TAG_SYNC) {
//      RV(lastSyncTime) = kfhalReadTimer();
      RV(syncQueued) = 0;
    } else {
      int pdoIdx = (msg->tag >> 16) & 0xffff;
      int pdoNo = msg->tag & 0xffff;
/* For the moment the tag only contains pdoIdx and pdoNo, if pdoNo is equal
 * to zero then it's no pdo - don't care */
      if ((pdoNo == 0) || (pdoNo > 512) || (pdoIdx >= PDO_MAX_COUNT) ) {
        return 0; /* if pdo = 0 or invalid range - RTS from INRPT func*/
      } else {
        /* The QUEUED flag should be set now, and we are the only one that are allowed to clear it.
         * Should we not be called for a message, that PDO will never be transmitted again.
         */
        coPDO *pdo = &CV(PDO)[pdoIdx];
        if(pdo->pdoNo == pdoNo)
        {  
          pdo->lastMsgTime = kfhalReadTimer();  /* update timer of PDO */
          clrBM(pdo->status, PDO_STATUS_QUEUED);
          if( pdo->inhibitTime != 0 )
            pdo->status = PDO_STATUS_INHIBIT_ACTIVE;
          return 0; /*RTS*/
        }
      }
    }
  } else if (cbType == CANHAL_CB_RX_OVERRUN)  {
    /*
    *   Pass rx buffer overrun error to client code
    */
    rxErr = 0; ovErr = 0;
    canGetErrorCounters(chan, &txErr, &rxErr, &ovErr);
    coError(CHP LOC_CO_ERR_CANHAL, 1, (uint16)ovErr, (uint8)rxErr, msg->id);
  } else if (cbType == CANHAL_CB_ERR_PASSIVE) {
    txErr = 0; 
    rxErr = 0; 
    ovErr = 0;
   /*
    *   Pass error passive notification to client code
    */
    canGetErrorCounters(chan, &txErr, &rxErr, &ovErr);
    coError(CHP LOC_CO_ERR_CANHAL, 2, (uint16)txErr, (uint8)rxErr, (uint32)ovErr);
  } else if (cbType == CANHAL_CB_BUS_OFF) {
    CanHalCanStatus status;
    txErr = 0; 
    rxErr = 0; 
    ovErr = 0;
   /*
    *   Pass bus off notification to client code
    */
    canGetErrorCounters(chan, &txErr, &rxErr, &ovErr);
    coError(CHP LOC_CO_ERR_CANHAL, 3, (uint16)txErr, (uint8)rxErr, (uint32)ovErr);
#if ERROR_BEHAVIOR
		switch(CV(CommunicationError)){
		case 0:
			 if( RV(opState)== coOS_Operational){
				RV(opState) = coOS_PreOperational; 
			 }
			 break;
		case 1: 
			break;
		case 2: 
			RV(opState) = coOS_Stopped;
			break;
		default:
			break;
		}
#endif
	/* 
    *   MikkoR 2004-08-02: Try to put card back on bus
    */
    status = canGoBusOn(chan);
    if (status != CANHAL_OK) {
      coError(CHP LOC_CO_ERR_CANHAL, 4, 0, status, 0);
    }
  } else if (cbType == CANHAL_CB_ERROR) {
    /*
    *   Pass canhal errors to client code
    */
    uint32 id = 0;
    uint8 dlc = 0;
    if(msg)
    {
      id = msg->id;
      dlc = msg->dlc;
    }
    coError(CHP LOC_CO_ERR_CANHAL, 5, 0, dlc, id);
  }
  return 0;
}

/*
**  handleReceivedDataInSegmentedSDO is called from the handleSDO function.
**  
**  This means that we shall take care of all the data that is
**  attached data in this received SDO frame.
** 
**  This function can both handle when a remote node is using
**  it's client SDO and write to our local object dictionaty
**  or if we (this node) received data ("downloads data") using
**  our client SDO. In the second case (we are downloading data
**  using our client SDO) we will only pass this data to the
**  applicaition.
*/

static coErrorCode handleReceivedDataInSegmentedSDO(CH int i, uint8 *bufP) {
  uint8 validBytesInSegment = 7 - ((bufP[0] & SEG_N_MASK) >> SEG_N_MASK_SHIFT);
  coErrorCode res = CO_ERR_GENERAL;
  coSDO *sdoP = &(CV(SDO)[i]);
  CoSegTranAction state;

  if (validBytesInSegment > 7) {
    validBytesInSegment = 7;  /* Make sure that we are in range, maybe the CAN-msg is corrupt. */
  }
  /*
  *   Check if the "c-flag" which is set by the remote node if it thinks that there 
  *   will be more data to download after this frame or if the transfer is finished.
  */
  if (bufP[0] & C_MASK) {
    state = CO_SEG_DONE;
  } else {
    state = CO_SEG_CONTINUE;
  }

  if (sdoP->type == SDO_SERVER) {
    /*
    *   The SDO is a server meaning that 
    */
    if (sdoP->objToTransLen <= SIZE_LOCAL_SDO_BUF) {
      /*
      *   Copy the contents of the received CAN message to the local receive 
      *   buffer. The first databyte of the can-message is not data it is flags
      *   and control-bits therefor it should not be copied of course.
      */
      memcpy(&sdoP->localBuf[sdoP->bufPos], (bufP + 1), validBytesInSegment);
      sdoP->bufPos += validBytesInSegment;
      sdoP->pos += validBytesInSegment;
      /*
      *
      */
      if ((sdoP->objToTransLen == sdoP->pos) && (state == CO_SEG_DONE)) {
        res = coWriteObjectP(CHP sdoP->objIdx, sdoP->subIdx, sdoP->localBuf, (uint8)sdoP->objToTransLen);
      } else if ((sdoP->objToTransLen > sdoP->pos) && (state == CO_SEG_CONTINUE)) {
        res = CO_OK;
      } else {
        res = CO_ERR_GENERAL;
      }
    } else {
      /*
      *   The object was too big to fit the local OD and therefore it is passed to the
      *   application.
      */
      res = appWriteObject(CHP sdoP->objIdx, sdoP->subIdx, (bufP+1), sdoP->pos, validBytesInSegment, state);
      sdoP->pos += validBytesInSegment;
    }
  } else if (sdoP->type == SDO_CLIENT) {
    /*
    *   If the specific SDO is a client this means that it is a download going on to the application
    *   and not the local object dictioinary.
    */
    res = appReadObjectResponseSDO(CHP sdoP->sdoNo, sdoP->objIdx, sdoP->subIdx, 
                                (bufP+1), sdoP->pos, validBytesInSegment, state);
    if (res == CO_OK) {
      sdoP->pos += validBytesInSegment;
    }
  }
  return res;
}


/*
*   constuctSegmentedServerSDO writes to the datafield of the CAN message that
*   is being sent.
*/
coErrorCode constuctSegmentedServerSDO(CH coSDO *sdoP, uint8 *canDataP, uint8 *bytesInBuf) {
  coErrorCode res;
  CoObjLength noBytesToBuf = 0;
  uint8 tValid = 0; 
  uint8 tReadCntr; 

  /*
  *   Add code for communicating to the local SDOs buffer so that object with a size of (1->local SDO buffer size) 
  *   can be transfered without notifing the application.
  */
  CoObjLength noBytesLeftToTransmit = sdoP->objToTransLen - sdoP->pos;
  uint8 *dataP = canDataP + 1;

  if (sdoP->objToTransLen <= SIZE_LOCAL_SDO_BUF) {
   /*
    *   We have a maximum of 7 bytes where we can write the segemented data. Check where we are in the object.
    *   The data to be sent is copied into the local SDO buffert.
    */
    if (noBytesLeftToTransmit <= 7) {
      noBytesToBuf = noBytesLeftToTransmit;
      /*
      *   Copy the amount of bytes to the buffer that is left to transmit.
      */
      memcpy(dataP, &(sdoP->localBuf[sdoP->bufPos]), noBytesToBuf);
      /*
      *   Clear up the rest of the buffert.
      */
      sdoP->bufPos += (uint8)noBytesToBuf;
      sdoP->pos += noBytesToBuf;
      *bytesInBuf = (uint8)noBytesToBuf;
      res = CO_OK;
    } else if (noBytesLeftToTransmit > 7) {
      noBytesToBuf = 7;
      /*
      *   Fill upp the total datafield of the CAN message.
      */
      memcpy(dataP, &(sdoP->localBuf[sdoP->bufPos]), noBytesToBuf);
      sdoP->bufPos += (uint8)noBytesToBuf;
      sdoP->pos += noBytesToBuf;
      *bytesInBuf = (uint8)noBytesToBuf;
      res = CO_OK;
    } else {
      /* 
      *   This should never happen.
      */
      res = CO_ERR_GENERAL;
    }

  } else {
    /*
    *   If the object to be transfersed is bigger than the local transfer buffer of the SDO
    *   then it will be handeled by the application.
    */
  /*  res = appReadObject(CHP sdoP->objIdx, sdoP->subIdx, (canDataP + 1), sdoP->pos, (uint8*)&noBytesToBuf, 7); */
    if (noBytesLeftToTransmit <= 7) {
      noBytesToBuf = noBytesLeftToTransmit;
	} else {
      noBytesToBuf = 7;
	}
	tReadCntr = 0;
    while( tReadCntr < noBytesToBuf){
          res = coReadObjectEx(CHP sdoP->objIdx, sdoP->subIdx, (canDataP + 1 + tReadCntr), sdoP->pos + tReadCntr, &tValid, (uint8)(noBytesToBuf - tReadCntr));
          if(res != CO_OK){
              return res; 
          }
          if( tValid > noBytesToBuf - tReadCntr || tValid == 0 ){
              return CO_ERR_GENERAL; /* Amount of written data to the buffer can not be bigger than the buffer itself, used to crash!  */ 
          }
          tReadCntr = tReadCntr + tValid;
	}
	sdoP->pos += noBytesToBuf;
    *bytesInBuf = (uint8)noBytesToBuf;
  }

  return res;
}



/*
*   This 
*/
coErrorCode constuctSegmentedClientSDO(CH coSDO *sdoP, uint8 *canDataP, uint8 *bytesToBufP) {
  coErrorCode res = CO_ERR_GENERAL;
  uint8 noBytesToBuf = 0; // PN Changed from   uint8 noBytesToBuf = -1;
  /*
  *   The SDO is a client. This means that we are currently "uploading" data from our application (note that there
  *   are no such things as uploading data from our local object dictionary to the remote node) and the data
  *   is asked for via this callback function.
  */    
  res = appWriteObjectResponseSDO(CHP sdoP->sdoNo, sdoP->objIdx, sdoP->subIdx, (canDataP + 1), sdoP->pos, &noBytesToBuf, 7);
  if (res == CO_OK) {
    if (noBytesToBuf <= 7) {
      sdoP->pos += noBytesToBuf;
      *bytesToBufP = noBytesToBuf;
    } else {
      res = CO_ERR_GENERAL;  /* Application can not write that much data! */
    }
  }
  return res;
}


/*
*   constructTransmitDataInSegmentedSDO. This function generates the datafield of the CAN message that will be
*   transmitted in the segmented transfer.
*/
coErrorCode constructTransmitDataInSegmentedSDO(CH int i, uint8 *canDataP, uint8 *seg_n, CoSegTranAction *transferStat) {
  coErrorCode res;
  coSDO *sdoP = &(CV(SDO)[i]);
  uint8  bytesInBuf;

  if (sdoP->type == SDO_SERVER) {
    res = constuctSegmentedServerSDO(CHP sdoP, canDataP, &bytesInBuf);
  } else if (sdoP->type == SDO_CLIENT) {
    res = constuctSegmentedClientSDO(CHP sdoP, canDataP, &bytesInBuf);
  } else {
    res = CO_ERR_GENERAL;
  }
  if (res == CO_OK) {
    *seg_n = 7 - bytesInBuf;
    if (sdoP->objToTransLen != NO_SIZE) {
      if (sdoP->pos < sdoP->objToTransLen) {
        *transferStat = CO_SEG_CONTINUE;
      } else if (sdoP->pos == sdoP->objToTransLen) {
        *transferStat = CO_SEG_DONE;
      } else {
        *transferStat = CO_SEG_UNKNOWN;
      }
    } else {
      if (bytesInBuf == 0) {
        *transferStat = CO_SEG_DONE;
      } else {
        *transferStat = CO_SEG_CONTINUE;
      }
    }
  }
  return res;
}

/* coReadObjectSDO is used by the node to initate SDO transfer (expedited or not  */
/* is not up to us to decide from a SDO-client (read from Object Dictionary of remote node). */

#if CLIENT_SDO_SUPPORT
CoStackResult coReadObjectSDO(CH uint8 sdoNo, uint16 objIdx, uint8 subIdx) {
	uint8 msgBuf[CAN_MESSAGE_LEN];
	CanHalCanStatus res;
	int i = findSDO(CHP (uint8)sdoNo, SDO_CLIENT );
	if (IS_POSITIVE(i) == false){
		return coRes_ERR_UNDEFINED_SDO_NO;
	}
	if (CV(SDO)[i].status != 0) {
		return coRes_ERR_SDO_BUSY;
	}
    CV(SDO)[i].objIdx = objIdx;
    CV(SDO)[i].subIdx = subIdx;
    CV(SDO)[i].objToTransLen = 0;
    CV(SDO)[i].pos = 0;
    CV(SDO)[i].bufPos = 0;
    CV(SDO)[i].status = CLI_INITIATE_UL_REQUESTED;
    CV(SDO)[i].timer = kfhalReadTimer();

    memset(msgBuf, 0, CAN_MESSAGE_LEN);
    msgBuf[0] = (CLI_REQUEST_INIT_TRANSF_FROM_SRV_TO_CLI << XSC_SHIFT);
    val2buf(objIdx, &msgBuf[1], 2);
    msgBuf[3] = subIdx;

    res = coWriteMessage(CHP CV(SDO)[i].COBID_Tx, msgBuf, CAN_MESSAGE_LEN, 0);
    if (res != CANHAL_OK) {
      CV(SDO)[i].status = 0;
      return getCanHalError(res);  /* Convert CAN-error to CoStackResult */
	} else{
      return coRes_OK;
	}
}
#endif

/* coWriteObjectExpSDO is used by the node to initate expedited SDO transfer */
/* from a SDO-client (write to Object Dictionary of remote node). */

#if CLIENT_SDO_SUPPORT
CoStackResult coWriteObjectExpSDO(CH uint8 sdoNo, uint16 objIdx, uint8 subIdx, uint32 val, uint8 objLen) {
	CanHalCanStatus res;
	int i;
	uint8 msgBuf[CAN_MESSAGE_LEN];
	i = findSDO(CHP sdoNo, SDO_CLIENT); /* get SDO[]-array index */
	if ( IS_POSITIVE(i) == false){
		return coRes_ERR_UNDEFINED_SDO_NO;
	}
	if ((objLen != 0) && (objLen <= 4) && (CV(SDO)[i].status == 0)) {
		memset(msgBuf, 0, CAN_MESSAGE_LEN);
		msgBuf[0] = (CLI_REQUEST_INIT_TRANSF_FROM_CLI_TO_SRV << XSC_SHIFT) | 
		  SIZE_MASK | EXPEDITED_MASK | ((4 - objLen) << INITIATE_N_MASK_SHIFT); /*lint !e701 Shift left of signed quantity (int) */
		val2buf(objIdx, &msgBuf[1], 2);
		msgBuf[3] = subIdx;
		val2buf(val, &msgBuf[4], objLen);
		CV(SDO)[i].objIdx = objIdx;
		CV(SDO)[i].subIdx = subIdx;
		CV(SDO)[i].status &= ~SEG_TRANSFER_TOGGLE; /* Initate toggle value to zero */
		CV(SDO)[i].status |= CLI_EXPEDITED_DL_REQUESTED;
		CV(SDO)[i].objToTransLen = objLen;
		CV(SDO)[i].pos = 0;
		CV(SDO)[i].bufPos = 0;
		CV(SDO)[i].timer = kfhalReadTimer();
		res = coWriteMessage(CHP CV(SDO)[i].COBID_Tx, msgBuf, CAN_MESSAGE_LEN, 0);
		if (res != CANHAL_OK) {
		  CV(SDO)[i].status = 0; /* Something wrong in the CAN-HW, clear status */
		  return getCanHalError(res);
		} else {
		  return coRes_OK;
		}
	} else {
		return coRes_ERR_SDO_BUSY;
	}
}
#endif

/* coWriteObjectSDO is used by the node to initate SDO segmented transfer */
/* from a SDO-client (write to Object Dictionary of remote node). */

#if CLIENT_SDO_SUPPORT
CoStackResult coWriteObjectSDO(CH uint8 sdoNo, uint16 objIdx, uint8 subIdx, CoObjLength objLen) {
  int i = -1;
  uint8 msgBuf[CAN_MESSAGE_LEN];

  CanHalCanStatus res;
  i = findSDO(CHP (uint8)sdoNo, SDO_CLIENT ); /* get SDO[]-array index */
  if (IS_POSITIVE(i)) {
    if (CV(SDO)[i].status == 0) {
      memset(msgBuf, 0, CAN_MESSAGE_LEN);
      msgBuf[0] = (CLI_REQUEST_INIT_TRANSF_FROM_CLI_TO_SRV << XSC_SHIFT);
      if (objLen == NO_SIZE) {
        msgBuf[0] &= ~SIZE_MASK;  /* clear size flag. */
      } else {
        msgBuf[0] |= SIZE_MASK;   /* set size flag. */
      }
      val2buf(objIdx, &msgBuf[1], 2);
      msgBuf[3] = subIdx;
      val2buf(objLen, &msgBuf[4], 4);
      CV(SDO)[i].objIdx = objIdx;
      CV(SDO)[i].subIdx = subIdx;
      CV(SDO)[i].status &= ~SEG_TRANSFER_TOGGLE; /* Initate toggle value to zero */
      CV(SDO)[i].status = CLI_SEG_DL_REQUESTED;
      CV(SDO)[i].objToTransLen = objLen;
      CV(SDO)[i].pos = 0;
      CV(SDO)[i].bufPos = 0;
      CV(SDO)[i].timer = kfhalReadTimer();
      res = coWriteMessage(CHP CV(SDO)[i].COBID_Tx, msgBuf, CAN_MESSAGE_LEN, 0);
      if (res != CANHAL_OK) {
        CV(SDO)[i].status = 0;
        return getCanHalError(res);
      }
      return coRes_OK;
    } else {
      return coRes_ERR_SDO_BUSY;
    }
  }
  return coRes_ERR_UNDEFINED_SDO_NO;
}
#endif

/* 
*   coReadStatusSDO is used to read out the momentary status of a specific SDO. 
*/
coStatusSDO coReadStatusSDO(CH uint8 sdoNo, uint8 sdoType) {
  int i;
  i = findSDO(CHP (uint8)sdoNo, sdoType );
  if( i > 0 ){
    if( (CV(SDO)[i].status == 0) && (sdoType == SDO_CLIENT || sdoType == SDO_SERVER))
      return(SDO_READY);
    if( sdoType == SDO_CLIENT ){
      if(CV(SDO)[i].status & CLI_INITIATE_UL_REQUESTED)
        return(SDO_WAIT_RESPONSE_EXPEDITED_UL);
      if(CV(SDO)[i].status & CLI_EXPEDITED_DL_REQUESTED)
        return(SDO_WAIT_RESPONSE_EXPEDITED_DL);
      if(CV(SDO)[i].status & CLI_SEG_DL_INITIATED)
        return(SDO_WAIT_RESPONSE_SEGMENTED_DL);
      if(CV(SDO)[i].status & CLI_EXPEDITED_UL_REQUESTED)  /* qqq Never assigned to status! */
        return(SDO_WAIT_RESPONSE_EXPEDITED_UL);
      if(CV(SDO)[i].status & CLI_SEG_UL_INITIATED)
        return(SDO_WAIT_RESPONSE_SEGMENTED_UL);
      return SDO_STATUS_NOT_AVAILABLE;
    } else if ( sdoType == SDO_SERVER ){
      if(CV(SDO)[i].status & SRV_SEG_DL_INITIATED)
        return(SDO_WAIT_REQUEST_SEGMENTED_DL);
      if(CV(SDO)[i].status & SRV_SEG_UL_INITIATED)
        return(SDO_WAIT_REQUEST_SEGMENTED_UL);
      return SDO_STATUS_NOT_AVAILABLE;
    } else {
      return(SDO_STATUS_NOT_AVAILABLE);
    }
  } else {
    return(SDO_NOT_AVAILABLE);
  }
}

/* 
 *    The appGetLocalBufferSize functions is used so that the application can *
 *    read out the max size of the receive/transmit-buffer.
 *    This function is, I think, permanetly shut off becuase there where plenty
 *    of code and still complicated to the application. No gain.
 */

uint8 coGetBufferSize(uint8 sdoNo){
  return( 0 );
} /*lint !e715   Symbol 'sdoNo' (line 2842) not referenced */



/*
*   handleSyncEvent() is called if a syncmessage has been received,
*/
static void handleSyncEvent(CH_S){
  int i;
  bool inSyncWindow = true;   /* Becomes false when sync window has expired */

#if SYNC_WINDOW_LENGTH
  RV(syncRecvTime) = kfhalReadTimer();
#endif

  for (i = 0; i < PDO_MAX_COUNT; i++){
#if SYNC_WINDOW_LENGTH
	if( CV(syncWindowLen)){
	    KfHalTimer syncWindowTicks = kfhalUsToTicks(CV(syncWindowLen));
		if( syncWindowTicks == 0){
			syncWindowTicks = 1;
		}
		if( kfhalReadTimer() - RV(syncRecvTime) > syncWindowTicks){
			inSyncWindow = false;
		}
	}
#endif
	if(CV(PDO)[i].transmissionType == CO_PDO_TT_SYNC_ACYCLIC)
    {
      if((CV(PDO)[i].dir == PDO_TX) && (CV(PDO)[i].status & SYNC_TX_PDO_EVENT)){
        CV(PDO)[i].status &= ~SYNC_TX_PDO_EVENT;
		if( inSyncWindow){
	        (void)transmitPDO(CHP i); /* put on CAN-bus */
		}
	  } else if((CV(PDO)[i].dir == PDO_RX) && (CV(PDO)[i].status & SYNC_RX_PDO_EVENT)) {
        CV(PDO)[i].status &= ~SYNC_RX_PDO_EVENT;
        handlePDO(CHP i, CV(PDO)[i].cmBuf, CV(PDO)[i].cmDlc); /* give to appl. */
      }
    } else if( (CV(PDO)[i].transmissionType <= CO_PDO_TT_SYNC_CYCLIC_MAX) &&
        (CV(PDO)[i].transmissionType >= CO_PDO_TT_SYNC_CYCLIC_MIN)    ){
      if( CV(PDO)[i].dir == PDO_TX ){
        if(CV(PDO)[i].pdoSyncCntr > 0){
          CV(PDO)[i].pdoSyncCntr--;
        } else {
          CV(PDO)[i].pdoSyncCntr = CV(PDO)[i].transmissionType-1; /* re-init counter */
		  if( inSyncWindow){
		    (void)transmitPDO(CHP i);  /* Send PDO! */
		  }
#if SYNC_WINDOW_LENGTH
		  else{
			  /* we indicate app user the sync window length expires */
			 appIndicateSyncWindowLengthExpire(CHP CV(PDO)[i].pdoNo); 
		  }
#endif
		}
      } else if ((CV(PDO)[i].dir == PDO_RX) && (CV(PDO)[i].status  & SYNC_RX_PDO_EVENT)) {
        /* we don't have to bother about the counter if PDO_RX */
        CV(PDO)[i].status &= ~SYNC_RX_PDO_EVENT; /*clear event-flag */
        handlePDO(CHP i, CV(PDO)[i].cmBuf, CV(PDO)[i].cmDlc);
      }

    } else if (CV(PDO)[i].transmissionType == CO_PDO_TT_SYNC_RTR) {
      if((CV(PDO)[i].dir == PDO_TX) && (CV(PDO)[i].status & SYNC_TX_PDO_RTR ))
      {
        CV(PDO)[i].status &= ~SYNC_TX_PDO_RTR;
		if( inSyncWindow){
		  (void)transmitPDO(CHP i);
		}
	  }
    }
  }
}

/* Set node guarding parameters for a remote node or ourselves (when nId == 0xff).
* If nodeGuard is true for a remote node, the guardTime is set to heartBeatPeriod
* and a Node Guard Request message will be transmitted periodically for the node.
* In any case, heartBeatPeriod and lifeTimeFactor are remembered to get the time
* after which the node is regarded as lost if no heart beat message is received.
* If node guardnig is enabled, the togge bit has to toggle.
*
* For ourselve, if nodeGuard is true, we will not transmit any heartbeats
* automatically. If it is false, heartbeat messages will be transmitted with period
* heartBeatPeriod. In any case, lifeTimeFactor is not used.
*
*/

#if NODEGUARD_MASTER
CoStackResult coSetNodeGuard(CH uint8 nId, uint16 nodeGuardTime, uint8 lifeTimeFactor) {
	int i;

	/* Search for node, if already configured */
	for( i=0; i<MAX_GUARDED_NODES; i++){
		if( nId == RV(guardedNodes[i].nodeId)){
			break;
		}
	}
	/* If node was not found, try to find an empty slot instead */
	if( i == MAX_GUARDED_NODES){
		for( i=0; i<MAX_GUARDED_NODES; i++){
			if( RV(guardedNodes[i].nodeId) == 0){
				break;
			}
		}
	}
	/* If no empty slots left, FAIL */
	if( i == MAX_GUARDED_NODES){
		return coRes_ERR_CFG_NODEGUARD_LIST_FULL;
	}

	RV(guardedNodes)[i].nodeId = nId;
	RV(guardedNodes)[i].guardTimeMs = nodeGuardTime;
	RV(guardedNodes)[i].guardTimeTicks = kfhalMsToTicks(RV(guardedNodes)[i].guardTimeMs);
	RV(guardedNodes)[i].lifeTimeFactor = lifeTimeFactor;

	return ( coRes_OK );
}
#endif



/* 
*   coSetEmcyControl is set up if the specific node whishes to trigg on 
*   received EMERGENCY objects, this functions only reconfigures already 
*   initiated EMERGENCY objects and do not create them.
*   This function is convinient for the application engineer to use because
*   he will not have to deal with the EMCY consumer object as described 
*   on p. 11-13 in DS301. 
*/
#if EMERGENCY_MASTER
CoStackResult coSetEmcyControl(CH uint8 nId, cobIdT COBID) {
  int i=0;
  while ((i < MAX_EMCY_GUARDED_NODES) && (128 > nId)) {
    /* 
    *   Serach for NodeId (if nodeId is in valid range,  1-127)) 
    */
    if(CV(emcyNodes)[i].nodeId == nId){
      /* 
      *   The specific node has been found, reconfigure entry 
      */
      if (COBID == 0) {
        CV(emcyNodes)[i].COBID = 0x80 + nId; /* This is default value. */
      } else {
        CV(emcyNodes)[i].COBID = COBID; /* Write new COBID into array */
      }
      updateUsedCobIds();
	  return coRes_OK; /* COBID reconfigured successfully */
    } else {
      i++;
    }
  }
  return coRes_ERR_EMCY_CNTRL_NODE_NOT_FOUND; /* Could not reconfigure or add a new entry */
}
#endif

/* 
*   coSetEmcyControlCreate works exactly as coSetEmcyControl with the exception
*   that if the specific node id is not found, it is being created instead.
*/
#if EMERGENCY_MASTER
CoStackResult coSetEmcyControlCreate(CH uint8 nId, cobIdT COBID) {
  uint8 res;
  int i=0;
  if (nId > 128) {
    return coRes_ERR_EMCY_CNTRL_ID;
  }
  res = coSetEmcyControl(CHP nId, COBID);
  if (res == coRes_ERR_EMCY_CNTRL_NODE_NOT_FOUND) {
    bool freeSlotFound = false;
    /* 
    *   If the given nodeId was not found in the array it needs to be created.
    *   The first free slot is being looked for and a free slot is a slot where
    *   the nodeId is equal to zero (0).
    */
    while (i < MAX_EMCY_GUARDED_NODES && !freeSlotFound) {
      /*
      *   Look for a free slot (free slot equals zero in nodeId field);
      */
      if (CV(emcyNodes)[i].nodeId == 0) {
        /* 
        *   A free slot has been found, write entry into array. 
        */
        CV(emcyNodes)[i].nodeId = nId;
        if (COBID == 0) {
          CV(emcyNodes)[i].COBID = 0x80 + nId; /* This is default value. */
        } else {
          CV(emcyNodes)[i].COBID = COBID; /* Write new COBID into array */
        }
        freeSlotFound = true;
        res = coRes_OK;        
   	    updateUsedCobIds();
      } else {
        i++;
      }
    }
    if (!freeSlotFound) {
      res = coRes_ERR_EMCY_CNTRL_LIST_FULL; /* Could not reconfigure or add a new entry */
    }
  } else {
    res = coRes_OK;
  }
  return res;
}
#endif


/* qqq. This should be moved to the periodic-service routine */

/* void comNodeGuardControl(CH_S) {
/ *  CanId id;  * /
/ *  uint8 msg[8]; * /
/ *   uint8 dlc; * /
  int i; / *j* /
  bool restartNodes = false;
  KfHalTimer now = kfhalReadTimer();

  coPeriodicService(CHP_S);

  / * Verify that the other nodes are operating. * /
  for (i = 0; i < CV(nodeCount); i++) {
    if (CV(nodeInfo)[i].guardTime && now-CV(nodeInfo)[i].lastNodeGuardTime > CV(nodeInfo)[i].guardTime) {
      comNodeGuardRequest(CHP CV(nodeInfo)[i].nodeId);
      CV(nodeInfo)[i].lastNodeGuardTime = now;
    }
    if (now-CV(nodeInfo)[i].lastHeartbeatTime > CV(nodeInfo)[i].heartBeatTimeout) {
      CV(nodeInfo)[i].valid = false;
      if (!CV(nodeInfo)[i].valid || CV(nodeInfo)[i].state != coOS_Operational)
        restartNodes = true;
    }
  }
}
*/

#if NODEGUARD_MASTER

/* handleNodeguardMessage handles the received nodeguard messages. This function is not */
/* called from the application. */

static void handleNodeguardMessage(CH uint8 mNodeId, uint8 mToggle, coOpState mState) {
  int i;
  for (i = 0; i < MAX_GUARDED_NODES; i++) {
    if (RV(guardedNodes)[i].nodeId == mNodeId && mNodeId != 0) {

	  /* 
	   * Only handle nodeguard messages after an RTR has been sent 
	  */
	  if(!RV(guardedNodes)[i].isNodeguardingActive) {
		  return;
      }

      /*
       *  Check that the received message has the proper toggelbit
       */
      if( RV(guardedNodes)[i].alive && RV(guardedNodes)[i].toggle == mToggle)
        return; /* Message didn't toggle */

      /*
       * Kalle (Plenware) 2005-02-11:
       * check if NMT state has changed and indicate change to application
       */
      if(RV(guardedNodes)[i].state != mState)
      {
        appIndicateNodeGuardStateChange( CHP mNodeId, mState);
      }
      /*
       * Store received values
       */
      RV(guardedNodes)[i].alive = true; /* set toggle */
      RV(guardedNodes)[i].toggle = mToggle; /* toggle */
      RV(guardedNodes)[i].state = mState; /* write status */
      RV(guardedNodes)[i].lastNodeGuardRecvTime = kfhalReadTimer();
      break;
    }
  }
}
#endif

#if HEARTBEAT_CONSUMER

/* handleHeartbeat handles the received heart-beat messages. This function is not */
/* called from the application. */

static void handleHeartbeat(CH uint8 mNodeId, coOpState mState) {
  int i;
  for (i = 0; i < MAX_HEARTBEAT_NODES; i++) {
    if (CV(heartbeatNodes)[i].nodeId == mNodeId && mNodeId != 0) {
      /*
       *  We have at this point received a message from a that we have
       *  in our list of nodes that we do need to control.
       *  Check if the heartbeat control flag is set for this node and
       *  if is not set the flag.
       */
      if(!CV(heartbeatNodes)[i].isHeartBeatControlRunning) {
        /*  
         *  start the heartbeat control since it is obviously not
         *  started. From this point the heart beat gurading will be
         *  executed.
         */
        CV(heartbeatNodes)[i].isHeartBeatControlRunning = true;
      }

      /*
       * Kalle (Plenware) 2005-02-11:
       * check if NMT state has changed and indicate change to application
       */
      if(CV(heartbeatNodes)[i].state != mState)
      {
        appIndicateNodeGuardStateChange(CHP mNodeId, mState);
      }
      /*
       * Store received values
       */
      CV(heartbeatNodes)[i].alive = true; /* set toggle */
      CV(heartbeatNodes)[i].state = mState; /* write status */
      CV(heartbeatNodes)[i].lastHeartbeatTime = kfhalReadTimer();
      break;
    }
  }
}

/* coGetNodeState returns the status (operational state) for a node that is */
/* being monitored through the nodeguarding or heartbeat consumer protocols. */

coOpState coGetNodeState( CH uint8 mNodeId){
  int i;
  if( mNodeId == 0 || mNodeId > 0x7F){
	return (coOS_Unknown);
  }
#if HEARTBEAT_CONSUMER
  for( i = 0; i < MAX_HEARTBEAT_NODES; i++ ){
    if ( CV(heartbeatNodes)[i].nodeId == mNodeId ) {
      return (CV(heartbeatNodes)[i].state);
    }
  }
#endif
#if NODEGUARD_MASTER
  for( i = 0; i < MAX_GUARDED_NODES; i++ ){
    if ( RV(guardedNodes)[i].nodeId == mNodeId ) {
      return (RV(guardedNodes)[i].state);
    }
  }
#endif
  return (coOS_Unknown);
}
#endif /* HEARTBEAT_CONSUMER */

/* coGetChannel is just a secret communication-channel between canopen.c and canopenM.c */
/* beacuse the functions are declared as staic and therefore not reachable otherwise. */

uint8 coGetChannel(CH_S){return((uint8)RV(canChan)); }

/* resetDynMapList copies the default (from ROM) to the RAM memory. This function */
/* is only used by systems that uses the dynamic PDO mapping. */

#if DYNAMIC_PDO_MAPPING
static CoStackResult resetDynMapList(CH_S){
  int i=0;
  if (RV(pdoMap)) {                        /* Pointer to default PDO-map */
    const coPdoMapItem *m = RV(pdoMap);
    while( i < DYNAMIC_PDO_MAPPING_MAP_ITEMS ){
      CV(dynPdoMap[i]) = *(m);             /* Copy to the dynamic memory */
      if(m->len == CO_PDOMAP_END)
        break;
      m++; i++;
    }
    if(m->len == CO_PDOMAP_END)
      return(coRes_OK);
    else
      return(coRes_ERR_DYNAMIC_MAP_RESET);
  }
  return(coRes_ERR_DYNAMIC_MAP_RESET);
}
#endif



#if DYNAMIC_PDO_MAPPING
/*
*   Verify that the object that application want to map is mapable, else
*   return coErrorCode error code. Fn also verifies that the expected and 
*   real length of the objects are equal.
*
*   qqq: Signal that a object that is read only is in RX PDO. Or that a 
*        write only is in a transmit PDO.
*/
static coErrorCode isMapParamsOK(CH uint8 dir, uint16 objIdx, uint8 subIdx, uint8 objBitLen, cobIdT pdoCobId)		// Modified by PN
{
  coErrorCode res = CO_ERR_GENERAL;
  uint8 attr = 0;
  coErrorCode err = 0;

  if( dir == PDO_RX && objIdx > 0 && objIdx < 8){
  	return CO_OK;
  }

  attr = coGetObjectAttr(CHP objIdx, subIdx);
  if (attr & CO_OD_ATTR_PDO_MAPABLE) 
  {
    uint8 localObjLenBits = 0;
    localObjLenBits = (uint8)coGetObjectLen(CHP objIdx, subIdx, &err);
    if (localObjLenBits == objBitLen) {
      res = CO_OK;
    } else {
      res = CO_ERR_NOT_MAPABLE;		// Changed by PN to suite the CANopen specification
    }

	// Added by PN so we check that the dirrections are correct
	if( dir == PDO_RX )
		if(!(attr & CO_OD_ATTR_WRITE))
			res = CO_ERR_NOT_MAPABLE;
	if( dir == PDO_TX )
		if(!(attr & CO_OD_ATTR_READ))
			res = CO_ERR_NOT_MAPABLE;
#if PDO_MAP_COBID_CHECK						// Added by PN
	if(!(pdoCobId & cobPDOinvalid))
		res = CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT;
#endif

  } else {
    res = CO_ERR_NOT_MAPABLE;
  }
  return res;
}



/*
*   This function is used to find the mappings connected to the PDO 
*   at the given object index.
*   If the asked for PDO is not found and "createPdoIfNotFound" is set
*   the PDO will be created. If the PDO is found (or created) but no
*   mapping is found and flag "createMapIfNotExist" is set, mapping 
*   entry will be created.
*/
static int findPdoIdxForMapParms(CH uint16 objIdx, bool createPdoIfNotFound, bool createMapIfNotExist, coErrorCode *err) {
  coErrorCode coError;
  int pdoIdx = -1;
  if (objIdx >= CO_OBJDICT_PDO_RX_MAP && objIdx <= CO_OBJDICT_PDO_TX_MAP_END) {
    uint8 pdoNo = 0;
    uint8 dir = 0;
    if (objIdx >= CO_OBJDICT_PDO_RX_MAP && objIdx < CO_OBJDICT_PDO_TX_COMM) {
      dir = PDO_RX;
      pdoNo = objIdx + 1 - CO_OBJDICT_PDO_RX_MAP;
    } else if (objIdx >= CO_OBJDICT_PDO_TX_MAP && objIdx <= CO_OBJDICT_PDO_TX_MAP_END) {
      dir = PDO_TX;
      pdoNo = objIdx + 1 - CO_OBJDICT_PDO_TX_MAP;
    }
    if (createPdoIfNotFound) {
      pdoIdx = findPDOc(CHP pdoNo, dir);
    } else {
      pdoIdx = findPDO(CHP pdoNo, dir);
    }
    if (IS_POSITIVE(pdoIdx)) {
      uint16 mapIdx = CV(PDO)[pdoIdx].mapIdx;
      if (mapIdx == PDO_MAP_NONE && createMapIfNotExist) {        
        int oldEndOfLstIdx = findMapListEndIdx(CHP 0);
        //printPDOmap(CHP_S);
        if (IS_POSITIVE(oldEndOfLstIdx)) {
          coError = createPdoMapSpaceInList(CHP oldEndOfLstIdx, 2);
          if (coError == CO_OK) {
            coPdoMapItem *pdoItmP = PDO_MAP;
            if (dir == PDO_RX) {
              pdoItmP[oldEndOfLstIdx].len = CO_PDOMAP_RX;
            } else {
              pdoItmP[oldEndOfLstIdx].len = CO_PDOMAP_TX;
            }
            pdoItmP[oldEndOfLstIdx].objIdx = pdoNo;
            pdoItmP[oldEndOfLstIdx].subIdx = 0;
            pdoItmP[oldEndOfLstIdx+1].objIdx = 0;
            pdoItmP[oldEndOfLstIdx+1].subIdx = 0;
            pdoItmP[oldEndOfLstIdx+1].len = 0;
            CV(PDO)[pdoIdx].mapIdx = oldEndOfLstIdx;
          }
          //printPDOmap(CHP_S);
        } else {
          coError = CO_ERR_GENERAL;
        }
      } else {
        coError = CO_OK;
      }
    } else {
      if (createPdoIfNotFound) {
        coError = CO_ERR_OUT_OF_MEMORY;
      } else {
        coError = CO_ERR_OBJECT_DOES_NOT_EXISTS;
      }
    }
  } else { // (objIdx >= CO_OBJDICT_PDO_RX_MAP && objIdx <= CO_OBJDICT_PDO_TX_MAP_END)
    coError = CO_ERR_OBJECT_DOES_NOT_EXISTS;
  }

  *err = coError;
  return pdoIdx;
}

/*
*   This functions adds a mapping entry to dyanmic PDO mapping list. 
*/
static int findPdoIdxForMapParmsEx(CH uint16 objIdx, uint8 subIdx, uint32 val, coErrorCode *err) {
  coErrorCode coError  = CO_ERR_GENERAL;
  int pdoIdx = -1;
  if (subIdx == 0) {
    if (val <= 64) {
      pdoIdx = findPdoIdxForMapParms(CHP objIdx, false, true, &coError);
      if (coError == CO_ERR_OBJECT_DOES_NOT_EXISTS) {
        if (val == 0) {
          coError = CO_OK;
        } else {
          coError = CO_ERR_SERVICE_PARAM_TOO_HIGH;
        }
      }
    } else if (val == SUB_0_DAMPDO || val == SUB_0_SAMPDO) {
      pdoIdx = findPdoIdxForMapParms(CHP objIdx, true, false, &coError); 
    } else {
      /*
      *   Valid values for subIdx 0 is 0...64 and 254, 255. Rest if errors.
      */
      coError = CO_ERR_PDO_LENGHT;
    }
  } else {
    pdoIdx = findPdoIdxForMapParms(CHP objIdx, true, true, &coError);
  }
  *err = coError;
  return pdoIdx;
}


/*
*   This function extracts the parameters found in the uint32 val
*   variable and writes into the pdoItem structure.
*/
static void extractPdoMapParams(uint32 val, coPdoMapItem *pdoItem) {
  pdoItem->objIdx = (uint16)(0xffff & (val >> 16));
  pdoItem->subIdx = (uint8) (0xff & (val >> 8));
  pdoItem->len = (uint8) (0xff & val);
}

/*
*   This function will will look for the last index in the PDO map
*   list index. It will start from the very beginning and parse
*   the list until the END OF LIST flag.
*   Point at the beginning of the list and parse it until the entry
*   describing "end of list" has been found.
*/
static int findMapListEndIdx(CH int startSearchIdx) {
  int i = -1;

  if (PDO_MAP) {
    bool endOfListIsFound = false;
    coPdoMapItem *pdoItmP = PDO_MAP;
    i = startSearchIdx;  
    endOfListIsFound = false;
    while (i < DYNAMIC_PDO_MAPPING_MAP_ITEMS && !endOfListIsFound) {
      if (pdoItmP[i].len == CO_PDOMAP_END) {
        endOfListIsFound = true;
      } else {
        i++;
      }
    }
    if (!endOfListIsFound) {
      i = -1;
    }
  }
  return i;
}


/*
*   This function creates space in the reference PDO mapping list.
*   The space will be from the given "freeSlotIdx" and the size of this
*   slot will be "space" wide.
*   qqq: Maybe we should change all this to a linked list.
*/
static coErrorCode createPdoMapSpaceInList(CH int startIdx, int space) {
  coErrorCode err = CO_OK;
  int oldEndOfListIdx = -1;

  if (space > 0) {
    oldEndOfListIdx = findMapListEndIdx(CHP startIdx);
    if (IS_POSITIVE(oldEndOfListIdx)) {
      int freeMem = DYNAMIC_PDO_MAPPING_MAP_ITEMS - 1 - oldEndOfListIdx;
      if (freeMem >= space) {
        coPdoMapItem *pdoItmP = PDO_MAP;
        bool copyFinished = false;
        int i = oldEndOfListIdx + space; /* i is the "new end of list bookmark. */
        /*
        *   Copy the entries of the list. The idea is to shift all the enties
        *   below "endOfListIdx" space n.o. steps.
        */
        while (space && !err && !copyFinished) {
          if (i < DYNAMIC_PDO_MAPPING_MAP_ITEMS && IS_POSITIVE(i-space)) {
            pdoItmP[i] = pdoItmP[i-space];
          } else {
            // We are at the begining of the list, alas terminate shift
            break;
          } 
          if (i == startIdx) {
            pdoItmP[i].len = 0;
            pdoItmP[i].objIdx = 0;
            pdoItmP[i].subIdx = 0;
            copyFinished = true;
          } else {
            i--;
          }
        }
      } else {
        err = CO_ERR_OUT_OF_MEMORY;
      }
    } else {
      err = CO_ERR_GENERAL;
    }
  } else {
    err = CO_ERR_GENERAL;
  } 
  if (!err) {
    int i;
    for (i = 0; i < PDO_MAX_COUNT; i++){
      if(CV(PDO)[i].dir != PDO_UNUSED){
        if(CV(PDO)[i].mapIdx != PDO_MAP_NONE) {
          if(CV(PDO)[i].mapIdx  >= startIdx) {
            CV(PDO)[i].mapIdx += space;  
          }
        }
      }
    }
  }
  return err;
}

/*
*   This function deletes memoryslots from the memory. More precise this 
*   means that the from the given listIndx, with a size "space". 
*/
static coErrorCode deletePdoMapSpaceInList(CH int listIdx, int space) {
  coPdoMapItem *pdoMapItem = PDO_MAP;
  bool endOfListFound = false;
  bool err = false;
  coErrorCode res = CO_ERR_GENERAL;
  int i = listIdx + space;
  
  while (!endOfListFound && !err) {
    if (i < DYNAMIC_PDO_MAPPING_MAP_ITEMS && IS_POSITIVE(i-space)) {
      pdoMapItem[i-space] = pdoMapItem[i];
      if (pdoMapItem[i].len ==  CO_PDOMAP_END) {
        endOfListFound = true;
        res = CO_OK;
      } else {
        i++;
      }
    } else {
      err = true;
    }
  }
  if (!err) {
    /*
    *   At this point we have deleted "space" number of slots 
    *   form entry "listIdx", this means that we have to parse
    *   all the entries "CV(PDO)[i].mapIdx=q" and if we find 
    *   a q bigger than listIdx we have to decrement if "space"
    *   number of slots.
    */
    for (i = 0; i < PDO_MAX_COUNT; i++){
      if(CV(PDO)[i].dir != PDO_UNUSED){
        if(CV(PDO)[i].mapIdx != PDO_MAP_NONE) {
          if(CV(PDO)[i].mapIdx  > listIdx) {
            CV(PDO)[i].mapIdx -= space;  /* Decrement the index. */
          }
        }
      }
    }
  }
  if (err) {
    res = CO_ERR_GENERAL;
  }
  return res;
}

/*
*   This function returns the number of mapped objects for 
*   a specific PDO (addressed via its PDO-memory index
*   (meaning address PDO_MAP + pdoMemIdx for a pointer 
*   of type "coPdoMapItem" 
*   and not PDO number)
*/
static int calcNumberOfMappedObjPdo(CH int pdoMemIdx) {
  int res = -1;
  if (IS_POSITIVE(pdoMemIdx)) {
    if (pdoMemIdx < DYNAMIC_PDO_MAPPING_MAP_ITEMS) {
      coPdoMapItem *pdoMapItemP = (coPdoMapItem*)(PDO_MAP + 1 + pdoMemIdx);
      int counter = 0;
      while (CO_PDOMAP(pdoMapItemP)) {
        counter++;
        pdoMapItemP++;
      }
      res = counter;
    }
  }
  return res;
}

/*
*   CANopen DS301, p.9-87.
*   This function handles the writes to subIdx 0 for object
*
*   Object 1600h - 17FFh: Receive  PDO Communication Parameter
*   Object 1800h - 19FFh: Transmit PDO Communication Parameter
*/
static coErrorCode writeNoMapedApplObj(CH int pdoIdx, int mapIdx, int oldNoMapedObj, int newNoMapedObj) {
	coErrorCode res;

#if PDO_MAP_COBID_CHECK
	if(CV(PDO)[pdoIdx].COBID & cobPDOinvalid) {
#endif
		if (newNoMapedObj == 0) {
			/*
			*   Disable the PDO.
			*/
			CV(PDO)[pdoIdx].status |= PDO_STATUS_SUB_DISABLED; /* Allowed to change mapping of PDO */
#if MULTIPLEXED_PDO
			CV(PDO)[pdoIdx].status &= ~PDO_STATUS_MPDO; 
#endif
			res = CO_OK;
		} else if (newNoMapedObj == oldNoMapedObj) {
			/*
			*   Enable the PDO. 
			*/
			if(IS_POSITIVE(mapIdx) && IS_POSITIVE(pdoIdx)) {
				res = coReCalcAndRemapDirectPDO(CHP mapIdx, pdoIdx);
				if(res == CO_OK) {
					CV(PDO)[pdoIdx].status &= ~PDO_STATUS_SUB_DISABLED; 
#if MULTIPLEXED_PDO
					CV(PDO)[pdoIdx].status &= ~(PDO_STATUS_SAM_PDO|PDO_STATUS_SAM_PDO); 
#endif
				}
			} else {
				res = CO_ERR_GENERAL;
			}
		} else if (newNoMapedObj < oldNoMapedObj) {
			int noObjToDelete;
			/*
			*   Delete entries of the PDO and enable the PDO if success.
			*/
			noObjToDelete = oldNoMapedObj - newNoMapedObj;
			//res = deletePdoMapSpaceInList(CHP newNoMapedObj, noObjToDelete);
			res = deletePdoMapSpaceInList(CHP (mapIdx+1)+newNoMapedObj, noObjToDelete);
			if (res == CO_OK) {
				if(IS_POSITIVE(mapIdx) && IS_POSITIVE(pdoIdx)) {
					res = coReCalcAndRemapDirectPDO(CHP mapIdx, pdoIdx);
					if(res == CO_OK) {
						clrBM(CV(PDO)[pdoIdx].status, PDO_STATUS_SUB_DISABLED);
#if MULTIPLEXED_PDO
						clrBM(CV(PDO)[pdoIdx].status, PDO_STATUS_SAM_PDO|PDO_STATUS_SAM_PDO);
#endif
					}
				} else {
					res = CO_ERR_GENERAL; 
				}
			}
#if MULTIPLEXED_PDO
		} else if (newNoMapedObj == SUB_0_DAMPDO) {
			/*
			*   Configure PDO to be a "Destination Address Mode (DAM)" PDO.
			*/
			clrBM(CV(PDO)[pdoIdx].status, PDO_STATUS_SAM_PDO|PDO_STATUS_SUB_DISABLED);
			setBM(CV(PDO)[pdoIdx].status, PDO_STATUS_DAM_PDO);
			res = CO_OK;
		} else if (newNoMapedObj == SUB_0_SAMPDO) {
			uint8 pdoTraType = CV(PDO)[pdoIdx].transmissionType;
			/*
			*   Configure PDO to be a "Source Address Mode (SAM)" PDO. The
			*   transmission type must be 254 or 255 if it should be possible
			*   to jump into SAM PDO settings for the PDO.
			*/
			if (pdoTraType == CO_PDO_TT_ASYNC_MANUF || pdoTraType == CO_PDO_TT_ASYNC_PROF) {
				bool isWriteOk = false;
				/*
				*   It is only allowed to have one and only one SAM MPDO producer for each
				*   node (see DS301, p.11-8), thefore we have to make sure that  
				*   there is no other TX PDO already configured to be a SAM MPDO.
				*/
				if (CV(PDO)[pdoIdx].dir == PDO_TX) {
					int i = findSAMPDO(CHP PDO_TX);
					if (IS_POSITIVE(i)) {
						if (i == pdoIdx) {
							isWriteOk = true;
						} 
					} else {
						isWriteOk = true;
					}
				} else {
					/*
					*   It is OK to setup as many RX SAMPDO as we want to!
					*/
					isWriteOk = true;  
				}
				if (isWriteOk) {
					clrBM(CV(PDO)[pdoIdx].status, PDO_STATUS_DAM_PDO|PDO_STATUS_SUB_DISABLED);
					setBM(CV(PDO)[pdoIdx].status, PDO_STATUS_SAM_PDO);
					res = CO_OK;
				} else {
					res = CO_ERR_DATA_CAN_NOT_BE_STORED;
				}
			} else {
				res = CO_ERR_DATA_CAN_NOT_BE_STORED;
			}
#endif
		} else {
			/*
			*   Signal error because user tries to enable PDO mappings that does not exist.
			*/
			res = CO_ERR_NOT_MAPABLE;
		}
#if PDO_MAP_COBID_CHECK
	} else {
		res = CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT;	}
#endif

	return res;
}

/*
*   CANopen DS301, p.9-87.
*   This function handles the writes to subIdx 1-64 for objects:
*
*   Object 1600h - 17FFh: Receive  PDO Communication Parameter
*   Object 1800h - 19FFh: Transmit PDO Communication Parameter
*/
static coErrorCode writeMapedApplObj(CH int pdoIdx, int mapIdxOffsetForPdo, int oldNumOfMappedObjForPdo, int subIdx, uint32 val) {
  coErrorCode err;
  coPdoMapItem pdo;

  extractPdoMapParams(val, &pdo);
  err = isMapParamsOK(CHP CV(PDO)[pdoIdx].dir, pdo.objIdx, pdo.subIdx, pdo.len, CV(PDO)[pdoIdx].COBID);		// Added by PN so we can control PDO direction

  if (!err ) {
    if ((CV(PDO)[pdoIdx].status & PDO_STATUS_SUB_DISABLED) == 0) {
		err = CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT;					// Changed by PN to follow the CiA spec
    }
  }
  if (!err) {
    if (PDO_MAP) {
      coPdoMapItem *pdoMapForPdo = (PDO_MAP + mapIdxOffsetForPdo);
      if (subIdx <= oldNumOfMappedObjForPdo) {
        pdoMapForPdo[subIdx] = pdo;
      } else if (subIdx == oldNumOfMappedObjForPdo+1) {
        int newSlotIdx = oldNumOfMappedObjForPdo + (mapIdxOffsetForPdo + 1); // + 1 beacuse of mapIdx is is pointing at the header
        err = createPdoMapSpaceInList(CHP newSlotIdx, 1);
        if (!err) {
          pdoMapForPdo[subIdx] = pdo;
        }
      } else {
        err = CO_ERR_INVALID_SUBINDEX;
      }
    }
  }
  return err;
}


/* 
* writeDynamicMapPDO modifies the mapping for the PDOs and validates that the 
* mapping is allowed for the specific object aswell as that the size is correct. 
* writeDynamicMapPDO is called from the coWriteObject(..) function.
*/
static coErrorCode writeDynamicMapPDO(CH uint16 objIdx, uint8 subIdx, uint32 val){
  coErrorCode err = CO_ERR_GENERAL;
  int pdoIdx = -1;
  int pdoMapIdx = -1;
  int numOfMappedApplObjInPDO = -1;

  pdoIdx = findPdoIdxForMapParmsEx(CHP objIdx, subIdx, val, &err);
  if (IS_POSITIVE(pdoIdx) && err == CO_OK) {
    pdoMapIdx = CV(PDO)[pdoIdx].mapIdx;
    if (IS_POSITIVE(pdoMapIdx) && pdoMapIdx != PDO_MAP_NONE) {
      numOfMappedApplObjInPDO = calcNumberOfMappedObjPdo(CHP pdoMapIdx);
      if (!IS_POSITIVE(numOfMappedApplObjInPDO)) {
        err = CO_ERR_GENERAL;
      }
    } else {
      if (subIdx == 0 && (val == SUB_0_DAMPDO || val == SUB_0_SAMPDO)) {
        /*
        *   We are writing multiplexed PDO setting to a PDO that did
        *   not have any mapping. qqq: DAMPDO should have one mapping
        *   (which is stange) but obey the spec!
        */
        err = CO_OK;
      } else {
        err = CO_ERR_GENERAL;
      }
    }
  } 
  
  if (err == CO_OK) {
    if (subIdx == 0) {
      err = writeNoMapedApplObj(CHP pdoIdx, pdoMapIdx, numOfMappedApplObjInPDO, val);
    } else {
      err = writeMapedApplObj(CHP pdoIdx, pdoMapIdx, numOfMappedApplObjInPDO, subIdx, val);    
    }
  }
  return err;
}
#endif


#if DYNAMIC_PDO_MAPPING
static coErrorCode coReCalcAndRemapDirectPDO(CH int pdoMapIdx, int pdoIdx) {
  uint8 pdoLen = 0;
  uint8 pdoPos = 0; /* Byte counter in PDO */
  coPdoMapItem *m = PDO_MAP+pdoMapIdx+1;
#if CO_DIRECT_PDO_MAPPING
  CV(PDO)[pdoIdx].dmFlag = 1;           /* assume that dm will work */
  CV(PDO)[pdoIdx].dmNotifyCount = 0;    /* the counter for n.o. entries set to zero. */
#endif

  while (CO_PDOMAP(m)) {
    pdoLen += m->len;
#if CO_DIRECT_PDO_MAPPING
    if (CV(PDO)[pdoIdx].dmFlag) {
      const coObjDictEntry *ode = odFind(CHP m->objIdx);
      if (ode && (m->len & 7) == 0) { /* Make sure it is a even n.o. bytes? */
        /* We know from the previous calls that (objIdx, subIdx) is valid */
        uint8 subIdx = m->subIdx;
        coObjDictVarDesc *vd = ode->objDesc;
        uint8 *objPtr = (uint8*)ode->objPtr;
        uint8 itemLen = m->len/8; /* Length of each mapped object in bytes */
        /* Locate the address of the data and the variable descriptor.
         * Compare to coReadObject()/coWriteObject(). */
        if (subIdx)
          subIdx--;
        while (subIdx) {
          if (vd->attr & CO_OD_ATTR_VECTOR) {
            objPtr += vd->size*subIdx;
            break;
          } else {
            objPtr += vd->size;
            subIdx--;
            vd++;
          }
        }
        if (vd->attr & CO_OD_WRITE_VERIFY)
          CV(PDO)[pdoIdx].dmFlag = 0;   /* Direct PDO map is disabled ? */
        if ((CV(PDO)[pdoIdx].dir == PDO_RX && (vd->attr & CO_OD_WRITE_NOTIFY)) ||
            (CV(PDO)[pdoIdx].dir == PDO_TX && (vd->attr & CO_OD_READ_NOTIFY))) {

          /* Is the same group (or object) already notified? */
          uint8 j;
          for (j = 0; j < CV(PDO)[pdoIdx].dmNotifyCount; j++) {
            if ((CV(PDO)[pdoIdx].dmNotifyObjIdx[j] == m->objIdx && CV(PDO)[pdoIdx].dmNotifySubIdx[j] == m->subIdx) ||
               (vd->notificationGroup && CV(PDO)[pdoIdx].dmNotifyGroup[j] == vd->notificationGroup))
            break;
          }
          if (j >= CV(PDO)[pdoIdx].dmNotifyCount) {
            CV(PDO)[pdoIdx].dmNotifyObjIdx[CV(PDO)[pdoIdx].dmNotifyCount] = m->objIdx;
            CV(PDO)[pdoIdx].dmNotifySubIdx[CV(PDO)[pdoIdx].dmNotifyCount] = m->subIdx;
            CV(PDO)[pdoIdx].dmNotifyGroup[CV(PDO)[pdoIdx].dmNotifyCount] = vd->notificationGroup;
            CV(PDO)[pdoIdx].dmNotifyCount++;
          }
        }
#if BIG_ENDIAN
        objPtr += (uint8)(coGetObjectLen(CHP m->objIdx, m->subIdx)/8)-1; /* If itemLen shold differ from coGetObjectLen(). */
        while(itemLen) {
          CV(PDO)[pdoIdx].dmDataPtr[pdoPos++] = objPtr--;
          itemLen--;
        }
#else
        while(itemLen) {
          CV(PDO)[pdoIdx].dmDataPtr[pdoPos++] = objPtr++;
          itemLen--;
        }
#endif
      } else
        CV(PDO)[pdoIdx].dmFlag = 0;
    }
#endif // CO_DIRECT_PDO_MAPPING
    m++;
  }   /* While */

  if (pdoLen > 64) {
    coError(CHP LOC_CO_ERR_PDO, 2, RV(nodeId), 0, 0);
    return(CO_ERR_PDO_LENGHT);
  }
  // qqq i undefined
  CV(PDO)[pdoIdx].dataLen = (uint8)pdoLen;    /* pdoIndx used to be i, why? */

  return CO_OK;
} /*lint !e529   Warning 529: Symbol 'pdoPos' (line 3742) not subsequently */
#endif

#if BOOTUP_DS302
/*
*   writeNMTStartupSettings function writes to the variables handling the DS302
*   boot.  The reason for this function and the interface is becuase it is being
*   called from the function "locWriteObject" which is called from the function
*   coWriteObject (...) which is the inteface to the OD from the application.
*/
static coErrorCode writeNMTStartupSettings(CH uint16 objIdx, uint8 subIdx, uint32 val) {
  coErrorCode res = CO_ERR_GENERAL;
  int i = -1;
  if (objIdx == CO_OBJDICT_VER_CONFIG) { /* 0x1020, p.11-3 DS301. */
    if (subIdx == 0) {
      res = CO_ERR_OBJECT_READ_ONLY;     /* no supported entries. */
    } else if (subIdx == CO_CONF_DATE_SUBIDX) {
      CV(verCfgDate) = val;
      res = CO_OK;
    } else if (subIdx == CO_CONF_TIME_SUBIDX) {
      CV(verCfgTime) = val;
      res = CO_OK;
    } else {
      res = CO_ERR_INVALID_SUBINDEX;
    }
  } else if (objIdx == CO_OBJDICT_VER_APPL_SW) {
    if (subIdx == 0) {
      res = CO_ERR_OBJECT_READ_ONLY;     /* no supported entries. */
    } else if (subIdx == CO_VER_APPL_SW_DATE_SUBIDX) {
      CV(appSwDate) = val;
      res = CO_OK;
    } else if (subIdx == CO_VER_APPL_SW_TIME_SUBIDX) {
      CV(appSwTime) = val;
      res = CO_OK;
    } else {
      res = CO_ERR_INVALID_SUBINDEX;
    }
#if BOOTUP_DS302
  } else if (objIdx == CO_OBJDICT_NMT_STARTUP) {  /* 0x1f80, p.15 DS302. */
    if (subIdx == 0) {
      CV(NMTStartup) = val;
      res = CO_OK;
    } else {
      res = CO_ERR_INVALID_SUBINDEX;
    }
#endif
  } else if (objIdx == CO_OBJDICT_NMT_BOOTTIME) {  /* 0x1f89, p.16 DS302. */
    if (subIdx == 0) {
      CV(NMTBootTime) = val;
      RV(intNMTBootTime) = kfhalMsToTicks(val);
      res = CO_OK;
    } else {
      res = CO_ERR_INVALID_SUBINDEX;
    }
  } else if (((objIdx >= CO_OBJDICT_NMT_STARTUP) &&
             (objIdx <= CO_OBJDICT_NMT_STARTUP_EXPECT_SERIAL_NUMBER)) ||
             (objIdx == CO_OBJDICT_NMT_EXPECTED_SW_DATE) ||
             (objIdx == CO_OBJDICT_NMT_EXPECTED_SW_TIME)) {
    /* The structure for all these objects are identical. SubIdx = 0 is always
    * 127 and read-only. */
    if (subIdx == 0) {
      res = CO_ERR_OBJECT_READ_ONLY;
    } else if (subIdx >= 1 && subIdx <= 127) {
      uint8 nodeId = subIdx; /* In this range subIdx equals remote node id. */
      i = findNetworkListNodeIdC(CHP nodeId);
      if (i >= 0) {
        switch (objIdx) {
          case CO_OBJDICT_NMT_EXPECTED_SW_DATE:  /* 0x1f53 */
            CV(netWorkList)[i].softwareDateExpected = val;
            res = CO_OK;
            break;
          case CO_OBJDICT_NMT_EXPECTED_SW_TIME:  /* 0x1f54 */
            CV(netWorkList)[i].softwareTimeExpected = val;
            res = CO_OK;
            break;
          case CO_OBJDICT_NMT_NETWORK_LIST: /* 0x1f81 */
            CV(netWorkList)[i].slaveAssignment = val;
            res = CO_OK;
            break;
          case CO_OBJDICT_NMT_STARTUP_EXPECT_DEVICETYPE: /* 0x1f84 */
            CV(netWorkList)[i].deviceTypeExpected = val;
            res = CO_OK;
            break;
          case CO_OBJDICT_NMT_STARTUP_EXPECT_VENDOR_ID: /* 0x1f85 */
            CV(netWorkList)[i].vendorIdExpected = val;
            res = CO_OK;
            break;
          case CO_OBJDICT_NMT_STARTUP_EXPECT_PRODUCT_CODE: /* 0x1f86 */
            CV(netWorkList)[i].productCodeExpected = val;
            res = CO_OK;
            break;
          case CO_OBJDICT_NMT_STARTUP_EXPECT_REV_NUMBER:  /* 0x1f87 */
            CV(netWorkList)[i].revisionNumberExpected = val;
            res = CO_OK;
            break;
          case CO_OBJDICT_NMT_STARTUP_EXPECT_SERIAL_NUMBER: /* 0x1f88 */
            CV(netWorkList)[i].serialNumberExpected = val;
            res = CO_OK;
            break;
          default:
            res = CO_ERR_OBJECT_DOES_NOT_EXISTS;
            break;
        }
      } else {
        res = CO_ERR_OUT_OF_MEMORY; /* Object index/subindex is within range but write error. */
      }
    } else {
      res = CO_ERR_INVALID_SUBINDEX;
    }
  } else {
    res = CO_ERR_OBJECT_DOES_NOT_EXISTS;
  }
  return res;
}


/*
*   readNMTStartupSettings function fetches to the variables handling the DS302
*   boot.  The reason for this function and the interface is becuase it is being
*   called from the function "locReadObject" which is called from the function
*   coReadObject (...) which is the inteface to the OD from the application.
*/
static uint32 readNMTStartupSettings(CH uint16 objIdx, uint8 subIdx, coErrorCode *err) {
  int     i = -1;
  uint32  res = 0;
  *err = CO_ERR_GENERAL;
  if (objIdx == CO_OBJDICT_VER_CONFIG) { /* 0x1020, p.11-3 DS301. */
    switch (subIdx) {
      case 0:
        res = 2;
        *err = CO_OK;
        break;
      case CO_CONF_DATE_SUBIDX:
        res = CV(verCfgDate);
        *err = CO_OK;
        break;
      case CO_CONF_TIME_SUBIDX:
        res = CV(verCfgTime);
        *err = CO_OK;
        break;
      default:
        *err = CO_ERR_INVALID_SUBINDEX;
        break;
    }
  } else if (objIdx == CO_OBJDICT_VER_APPL_SW) {
    switch (subIdx) {
      case 0:
        res = 2;
        break;
      case CO_VER_APPL_SW_DATE_SUBIDX:
        res = CV(appSwDate);
        *err = CO_OK;
        break;
      case CO_VER_APPL_SW_TIME_SUBIDX:
        res = CV(appSwTime);
        *err = CO_OK;
        break;
      default:
        *err = CO_ERR_INVALID_SUBINDEX;
        break;
    }
#if BOOTUP_DS302
  } else if (objIdx == CO_OBJDICT_NMT_STARTUP) {  /* 0x1f80, p.15 DS302. */
    switch (subIdx) {
      case 0:
        res = CV(NMTStartup);
        *err = CO_OK;
        break;
      default:
        *err = CO_ERR_INVALID_SUBINDEX;
        break;
    }
#endif
  } else if (objIdx == CO_OBJDICT_NMT_BOOTTIME) {  /* 0x1f89, p.16 DS302. */
    switch (subIdx) {
      case 0:
        res = CV(NMTBootTime);
        *err = CO_OK;
        break;
      default:
        *err = CO_ERR_INVALID_SUBINDEX;
        break;
    }
  } else if  (((objIdx >= CO_OBJDICT_NMT_STARTUP) &&
             (objIdx <= CO_OBJDICT_NMT_STARTUP_EXPECT_SERIAL_NUMBER)) ||
             (objIdx == CO_OBJDICT_NMT_EXPECTED_SW_DATE) ||
             (objIdx == CO_OBJDICT_NMT_EXPECTED_SW_TIME)) {
    /*
    *   The structure for all these objects are identical. SubIdx = 0 is always
    *   127 and read-only.
    */
    if (subIdx == 0) {
      res = 127;     /* Always 127! */
      *err = CO_OK;
    } else if (subIdx >= 1 && subIdx <= 127) {
      bool  isNodeIdFound = false;
      uint8 nodeId = subIdx; /* In this range subIdx equals remote node id. */
      /*
      *   To avoid having to waste a lot of unused memeory (it is very
      *   unlikey that we do have 127 nodes) we introduce the to a list.
      *   The drawback is ofcourse that we have to scan for the specific node.
      */
      i = findNetworkListNodeId(CHP nodeId);
      if (i < 0) {
        isNodeIdFound = false;
      } else {
        isNodeIdFound = true;
      }
      switch (objIdx) {
        case CO_OBJDICT_NMT_EXPECTED_SW_DATE:  /* 0x1f53 */
          if (isNodeIdFound) {
            res = CV(netWorkList)[i].softwareDateExpected;
          } else {
            res = 0;  /* qqq: default val is zero? */
          }
          *err = CO_OK;
          break;
        case CO_OBJDICT_NMT_EXPECTED_SW_TIME:  /* 0x1f54 */
          if (isNodeIdFound) {
            res = CV(netWorkList)[i].softwareTimeExpected;
          } else {
            res = 0;  /* qqq: default val is zero? */
          }
          *err = CO_OK;
          break;
        case CO_OBJDICT_NMT_NETWORK_LIST: /* 0x1f81 */
          if (isNodeIdFound) {
            res = CV(netWorkList)[i].slaveAssignment;
          } else {
            res = 0;  /* qqq: default val is zero? */
          }
          *err = CO_OK;
          break;
        case CO_OBJDICT_NMT_STARTUP_EXPECT_DEVICETYPE: /* 0x1f84 */
          if (isNodeIdFound) {
            res = CV(netWorkList)[i].deviceTypeExpected;
          } else {
            res = 0;  /* qqq: default val is zero? */
          }
          *err = CO_OK;
          break;
        case CO_OBJDICT_NMT_STARTUP_EXPECT_VENDOR_ID: /* 0x1f85 */
          if (isNodeIdFound) {
            res = CV(netWorkList)[i].vendorIdExpected;
          } else {
            res = 0;  /* qqq: default val is zero? */
          }
          *err = CO_OK;
          break;
        case CO_OBJDICT_NMT_STARTUP_EXPECT_PRODUCT_CODE: /* 0x1f86 */
          if (isNodeIdFound) {
            res = CV(netWorkList)[i].productCodeExpected;
          } else {
            res = 0;  /* qqq: default val is zero? */
          }
          *err = CO_OK;
          break;
        case CO_OBJDICT_NMT_STARTUP_EXPECT_REV_NUMBER:  /* 0x1f87 */
          if (isNodeIdFound) {
            res = CV(netWorkList)[i].revisionNumberExpected;
          } else {
            res = 0;  /* qqq: default val is zero? */
          }
          *err = CO_OK;
          break;
        case CO_OBJDICT_NMT_STARTUP_EXPECT_SERIAL_NUMBER: /* 0x1f88 */
          if (isNodeIdFound) {
            res = CV(netWorkList)[i].serialNumberExpected;
          } else {
            res = 0;  /* qqq: default val is zero? */
          }
          *err = CO_OK;
          break;
        default:
          *err = CO_ERR_OBJECT_DOES_NOT_EXISTS;
          break;
      }
    } else {
      *err = CO_ERR_OUT_OF_MEMORY; /* Object index/subindex is within range but write error. */
    }
  } else {
    *err = CO_ERR_OBJECT_DOES_NOT_EXISTS;
  }
  return res;
}

/*
* This function finds the settings for the specified node Id. If node id
* do not exist function returns -1.
*
* qqq: Function could been made more efficient, such as break the for-loop
*      on a free slot. But make sure that there are no free slots in the
*      array (pack if removed) but this function will most likely not be
*      called during runtime and therefore maybe this is OK.
*/
static int findNetworkListNodeId(CH uint8 nodeId) {
  int i;
  for (i = 0; i < BOOTUP_MAX_COUNT; i++) {
    if (CV(netWorkList)[i].otherNodeId == nodeId) {
      return i;
    }
  }
  return -1;
}


/*
* This function finds the settings for the specified node Id. If node id
* do not exist function looks for a free slot and creates the slot with
* the specific node id. If we are out of memory function returns -1.
*/
static int findNetworkListNodeIdC(CH uint8 nodeId) {
  int i;
  /* Try to find the slot, if i < 0 then the node id was not found. */
  i = findNetworkListNodeId(CHP nodeId);
  if (i < 0) {
    /* Id == 0 equals a unused slot */
    i = findNetworkListNodeId(CHP 0);
    if (i >= 0) {
      /* Free slot found - take it! */
      CV(netWorkList)[i].otherNodeId = nodeId;
    }
  }
  return i;
}



/*  coMasterBootDS302:
*   Object CO_OBJDICT_NMT_STARTUP (0x1f80) carries information with configuration
*   saying that this node should preform master boot
*/
CoStackResult coMasterBootDS302(CH_S) {
  int             i;
  bool            keepAliveActive = false;
  CoStackResult   res;
  uint8           slaveNodeId;
  /*
  *   Get the timestamp when the boot was being started.
  */
  RV(bootStartTimeStamp) = kfhalReadTimer();
  /*
  *   Begin with parsing the list of all the configured nodes and verify
  *   if any of the nodes has the keep alive bit set (meaning that we are
  *   not allowed to send the "reset all node" network management message
  *   to the bus and reset all the nodes). If the bit is set we have to
  *   reset the nodes "node-by-node" (qqq: node by node reset is yet not
  *   implemented.)
  */
  for (i = 0; i < BOOTUP_MAX_COUNT; i++) {
    slaveNodeId = CV(netWorkList)[i].otherNodeId;
    if (slaveNodeId) {
      uint32 slaveNodeData; /* Slave assigment is a stupid name! */
      slaveNodeData = CV(netWorkList)[i].slaveAssignment;
      if (slaveNodeData & CO_MASTER_KEEP_ALIVE) {
        /*
        * If the keep alive flag is set for at least one of the nodes, the
        * nodes must be resetted node by node ()
        */
        keepAliveActive = true;
        break;
      }
    }
  }
  /*
  *  Was the "keepAliveActive" bit set? This function is currently disabled
  *  since it is not implemented (not on the wish-list from Sandvik.
  */
  if (keepAliveActive & 0) { /* qqq: "0" because function should not be used according to Sandvik.*/
    res = resetNodeByNode(CHP_S);
  } else {
    res = resetAllNodes(CHP_S);
  }
  if(res == coRes_OK) {
    /*
    * start the background parallel boot handled by periodicService.
    */
    res = initBackgroundBootParamDS302(CHP_S);
  }
  return  res;
}

static CoStackResult initBackgroundBootParamDS302(CH_S) {
  int i;
  RV(isInstanceBooting) = false;

  // Contains the number of nodes to boot
  RV(networkListIdxBootNode) = 0;

  for (i = 0; i < BOOTUP_MAX_COUNT; i++) {
    if (CV(netWorkList)[i].otherNodeId != 0) {
      // A registered slave
      RV(networkListIdxBootNode)++;
      CV(netWorkList)[i].bootState = stNodeBootInit;
      CV(netWorkList)[i].oneSecDelayTimeStamp = 0;
    }
    else {
      CV(netWorkList)[i].bootState = stNotInUse;
    }

    CV(netWorkList)[i].nodeBootRes = noErrorReported;
  }

  RV(isInstanceBooting) = true; // Now the boot will be caught by backgrd process
  return coRes_OK;
}

/*  
*   This function is being used if the keep alive bit is set for any of the
*   nodes in the list.
*/
static CoStackResult resetNodeByNode(CH_S) {
  /* qqq: ToDo: This function not implemented yet, it was not on the wish list
          from Sandvik! */
  return coRes_ERR_NODE_BY_NODE_RST_GENERAL;
}

/*
*  This function is being used used if it is OK to reset all networks in
*  the list..
*/
static CoStackResult resetAllNodes(CH_S) {
  CoStackResult res;
  /*
  *   Send the network management command to reset all nodes.
  */
  res = comNMT(CHP 0, coNMT_ResetCommunication);
  return res;
}

/*
 *  This function connects client SDO to 1 to the default SDO server of
 *  the node that is supposed to be booted.
 */
coErrorCode connectClientSdoToNodeId(CH int clientSdoNo, int nodeId) {
  coErrorCode res;
  /*
  * Write COBID for client Tx.
  */
  res = coWriteObject(CHP 0x1280+(clientSdoNo-1), 1, 0x600 + nodeId);
  if (res != CO_OK) {
    return res;
  }
  /*
  * Write COBID for client Rx.
  */
  res = coWriteObject(CHP 0x1280+(clientSdoNo-1), 2, 0x580 + nodeId);
  if (res != CO_OK) {
    return res;
  }
  /*
  * Write "other node Id" to object dictionary.
  */
  res = coWriteObject(CHP 0x1280+(clientSdoNo-1), 3, nodeId);
  return res;
}

/*
 * This function is being called from the SDO transfer upon reception of
 * SDO frame if Stack instace is currently working on the DS302 boot.
 */
static bool handleReceivedBootData(CH int nodeId, uint16 objIdx, uint8 subIdx, uint32 val, bool isErrorData) {
  int   i;
  bootStateMachine state;
  bool  isDataProcessed = false;

  i = findNetworkListNodeId(CHP nodeId);
  if (i >= 0) {
    state = CV(netWorkList)[i].bootState;
    switch (state) {
      case waitingForDeviceType:
        if (objIdx == CO_OBJDICT_DEVICE_TYPE && subIdx == 0) {
          if (isErrorData) {
            CV(netWorkList)[i].isDeviceTypeResErrCode = true;
          }
          CV(netWorkList)[i].deviceTypeRes = val;
          isDataProcessed = true;
        }
        break;
      case waitingForVendorId:
        if (objIdx == CO_OBJDICT_IDENTITY && subIdx == CO_IDENTITY_VENDOR_ID_SUBIDX) {
          if (isErrorData) {
            /*
            * The received data is a SDO error abort code!
            */
            CV(netWorkList)[i].isVendorIdResErrCode = true;
          }
          CV(netWorkList)[i].vendorIdRes = val;
          isDataProcessed = true;
        }
        break;
      case waitingForProductCode:
        if (objIdx == CO_OBJDICT_IDENTITY && subIdx == CO_IDENTITY_PROD_CODE_SUBIDX) {
          if (isErrorData) {
            /*
            * The received data is a SDO error abort code!
            */
            CV(netWorkList)[i].isProductCodeResErrCode = true;
          }
          CV(netWorkList)[i].productCodeRes = val;
          isDataProcessed = true;
        }
        break;
      case waitingForRevNmber:
        if (objIdx == CO_OBJDICT_IDENTITY && subIdx == CO_IDENTITY_REV_NUMBR_SUBIDX) {
          if (isErrorData) {
            /*
            * The received data is a SDO error abort code!
            */
            CV(netWorkList)[i].isRevisionNumberResErrCode = true;
          }
          CV(netWorkList)[i].revisionNumberRes = val;
          isDataProcessed = true;
        }
        break;
      case waitingForSerialNmbr:
        if (objIdx == CO_OBJDICT_IDENTITY && subIdx == CO_IDENTITY_SER_NUM_SUBIDX) {
          if (isErrorData) {
            /*
            * The received data is a SDO error abort code!
            */
            CV(netWorkList)[i].isSerialNumberResErrCode = true;
          }
          CV(netWorkList)[i].serialNumberRes = val;
          isDataProcessed = true;
        }
        break;
      case waitingForSoftwareDate:
        if (objIdx == CO_OBJDICT_VER_APPL_SW && subIdx == CO_VER_APPL_SW_DATE_SUBIDX) {
          if (isErrorData) {
            /*
            * The received data is a SDO error abort code!
            */
            CV(netWorkList)[i].isSoftwareDateResErrCode = true;
          }
          CV(netWorkList)[i].softwareDateRes = val;
          isDataProcessed = true;
        }
        break;
      case waitingForSoftwareTime:
        if (objIdx == CO_OBJDICT_VER_APPL_SW && subIdx == CO_VER_APPL_SW_TIME_SUBIDX) {
          if (isErrorData) {
            /*
            * The received data is a SDO error abort code!
            */
            CV(netWorkList)[i].isSoftwareTimeResErrCode = true;
          }
          CV(netWorkList)[i].softwareTimeRes = val;
          isDataProcessed = true;
        }
        break;
      case waitingForConfigurationDate:
        if (objIdx == CO_OBJDICT_VER_CONFIG && subIdx == CO_CONF_DATE_SUBIDX) {
          if (isErrorData) {
            /*
            * The received data is a SDO error abort code!
            */
            CV(netWorkList)[i].isConfigurationDateResErrCode = true;
          }
          CV(netWorkList)[i].configurationDateRes = val;
          isDataProcessed = true;
        }
        break;
      case waitingForConfigurationTime:
        if (objIdx == CO_OBJDICT_VER_CONFIG && subIdx == CO_CONF_TIME_SUBIDX) {
          if (isErrorData) {
            /*
            * The received data is a SDO error abort code!
            */
            CV(netWorkList)[i].isConfigurationTimeResErrCode = true;
          }
          CV(netWorkList)[i].configurationTimeRes = val;
          isDataProcessed = true;
        }
        break;
      default :
        isDataProcessed = false;
    }
  }
  return isDataProcessed;
}


/*
 * This function keeps track of the booting process and it is being called
 * in the "background-task" periodic service. It is nothing but a big
 * state machime.
 */
static void superviseBootDS302 (CH_S) {
  int                 i;
  bootStateMachine    state;
  uint8               nodeId;
  coStatusSDO         clientSdoStat;
  coErrorCode         coErr;
  CoStackResult       coStackRes;
  bool                contNormBoot = false;
  bool                isHeartReceived = false;
  bool                isHeartbeatTimedOut = false;
  bool                isOkEnterOperationalMode = false;
  KfHalTimer          now;
  int                 iNumberOfBootingSlaves = 0;
  bootResultStates    bootErrCode;

  /*
  *   Return directly if the "isInstanceBooting" flag is cleared, meaning
  *   that the boot is not running and it is stange that we are here but
  *   we handle it by returning
  */
  if (!RV(isInstanceBooting)) {
    return;
  }
  /*
  *   Fetch the network list index for this boot. With this index we are able
  *   to access all the parameters of the node for the boot.
  */
  for (i = 0; i < BOOTUP_MAX_COUNT; i++) {
    nodeId = CV(netWorkList)[i].otherNodeId;
    if (nodeId) {
      if (CV(netWorkList)[i].bootState != stNotInUse) {
        iNumberOfBootingSlaves++;
      }
    /*
    *   Make sure that the client SDO 1 is not occupied doing. If it is, wait
    *   until finished.
      */
      clientSdoStat = coReadStatusSDO(CHP i + 1, SDO_CLIENT);
      if ((clientSdoStat == SDO_READY) || 
          ((clientSdoStat == SDO_NOT_AVAILABLE) && 
           (CV(netWorkList)[i].bootState == stNodeBootInit))) {
        state = CV(netWorkList)[i].bootState;
        switch (state) {
        case stNotInUse:
          break;
        case stNodeBootInit:
        /*
        *   We have just begun booting a new node. Connect client SDO 1 (the
        *   SDO that will be used thru the whole booting process) and ask for
        *   the device type using the SDO transfer protocol.
        *   The device type will be found on object index 0x1000 in remote node.
        *   The master have the expeceted corresponding value on
        *   0x1f84 : nodeId.
        *
        *
        *   Check if 0x1f81 : 0 is set for the specific node
          */
          if (CV(netWorkList)[i].slaveAssignment & CO_NODE_IS_SLAVE) {
            uint16  guardTime;
            uint8   retryFactor;
            uint32  slaveAssignment;
            /*
            *   The slave assignment (32 bit) is coded like this:
            *     Byte 0:   Flags (see DS302, p.16).
            *     Byte 1:   Retryfactor.
            *     Byte 2-3: Guard time.
            */
            slaveAssignment = CV(netWorkList)[i].slaveAssignment;
            guardTime =  (uint16)(slaveAssignment >> 16) & 0xffff;
            retryFactor = (uint8)(slaveAssignment >> 8) & 0xff;
            /*
            *   Setup node guardning for the node if we are configured to do so.
            *   (depends on wether guardTime is not equal to zero).
            */
            if (guardTime) {
            /*
            *   Strange as it seems we do not configure the heartbeat supervising
            *   settings here, they are found on object index 0x1016.
              */
              coStackRes = coSetNodeGuard(CHP nodeId, guardTime, retryFactor);
              if (coStackRes != coRes_OK) {
                coError(CHP 1, 1, 2, 3, 4);
                CV(netWorkList)[i].bootState = stNotInUse;
              }
            }
            coErr = connectClientSdoToNodeId(CHP i + 1, nodeId);
            if (coErr == CO_OK) {
              CV(netWorkList)[i].bootState = waitingForDeviceType; /* Always request */
              coStackRes = coReadObjectSDO(CHP i + 1, CO_OBJDICT_DEVICE_TYPE, 0x0);
              if (coStackRes  != coRes_OK) {
                coError(CHP 1, 1, 2, 3, 4);
                CV(netWorkList)[i].bootState = stNotInUse;
              }
            } else {
              coError(CHP 1, 1, 2, 3, 4);
              CV(netWorkList)[i].bootState = stNotInUse;
            }
          } else {
            RV(networkListIdxBootNode)--;
            CV(netWorkList)[i].bootState = stNotInUse;
          }
          break;
        case waitingForDeviceType:
        /*
        *   In this state we are waiting for the response on the Device Type
        *   request made in "stNodeBootInit"-state.
        *   The response from node is found in :
        *     CV(netWorkList)[i].deviceTypeRes
        *   If no response was received before the SDO client timed out the
        *   contents of CV(netWorkList)[i].deviceTypeRes will be  "CO_ERR_SDO_PROTOCOL_TIMED_OUT".
        *   If we either receive the timeout or unexpected we jump tp the state
        *   checkIfNodeIsMandatory. (see "case checkIfNodeIsMandatory:" below).
        *   That state determines if the boot will proceed or not (=can we run
        *   without this specific node?)
          */
          if (CV(netWorkList)[i].deviceTypeRes == CO_ERR_SDO_PROTOCOL_TIMED_OUT) {
          /*
          *   Timeout SDO DeviceType request => boot status 'B'
          *   Next step is to check if node is mandatory.
            */
            CV(netWorkList)[i].nodeBootRes = B;  /* Response timed out! */
            CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
            contNormBoot = false;
          } 
          else {
            if ((CV(netWorkList)[i].deviceTypeExpected != 0) && 
              (CV(netWorkList)[i].deviceTypeRes != CV(netWorkList)[i].deviceTypeExpected)) {
              /*
              *   Faulty devicetype on node => boot status 'C'
              *   Next step is to check if node is mandatory.
              */
              CV(netWorkList)[i].nodeBootRes = C; /* We received a response from remote but it was faulty! */
              CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
              contNormBoot = false;
            } else {
              contNormBoot = true;
            }
          }
          if (contNormBoot) {
          /*
          *   The next data to ask for is the Vendor Id (if it is expected).
          *   The vendor id is found on object index 0x1018, sub index: 1 in remote.
            */
            CV(netWorkList)[i].bootState = waitingForVendorId;
            if (CV(netWorkList)[i].vendorIdExpected) {
            /*
            *   We do want to know the vendor id, found at 0x1018, 1.
              */
              coStackRes = coReadObjectSDO(CHP i + 1, CO_OBJDICT_IDENTITY, CO_IDENTITY_VENDOR_ID_SUBIDX);
              if (coStackRes  != coRes_OK) {
                coError(CHP 1, 1, 2, 3, 4);
                CV(netWorkList)[i].bootState = stNotInUse;
              }
            }
          }
          
          break;
        case waitingForVendorId:
        /*
        * Vendor Id expected is found at 0x1f85 : nodeId.
          */
          if (CV(netWorkList)[i].vendorIdRes || !CV(netWorkList)[i].vendorIdExpected) {
            if (CV(netWorkList)[i].vendorIdExpected) {
              if (CV(netWorkList)[i].vendorIdRes == CV(netWorkList)[i].vendorIdExpected) {
              /*
              * Devicetype is OK.
                */
                contNormBoot = true;
              } else {
              /*
              *   Either time out on response or unexpected data received.
                */
                CV(netWorkList)[i].nodeBootRes = D; /* Actual Vendor ID (object 1018h) of the slave node did not match*/
                CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
                contNormBoot = false;
              }
            } else {
            /*
            * We are not expecting the stVendorId, continue normal boot!
              */
              contNormBoot = true;
            }
            if (contNormBoot) {
              CV(netWorkList)[i].bootState = waitingForProductCode;
              if (CV(netWorkList)[i].productCodeExpected) {
                coStackRes = coReadObjectSDO(CHP i + 1, CO_OBJDICT_IDENTITY, CO_IDENTITY_PROD_CODE_SUBIDX);
                if (coStackRes  != coRes_OK) {
                  coError(CHP 1, 1, 2, 3, 4);
                  CV(netWorkList)[i].bootState = stNotInUse;
                }
              }
            }
          }
          break;
        case waitingForProductCode:
        /*
        *   Product code expected is found at 0x1f86 : nodeId.
          */
          if (CV(netWorkList)[i].productCodeRes || !CV(netWorkList)[i].productCodeExpected) {
            if (CV(netWorkList)[i].productCodeExpected) {
              if (CV(netWorkList)[i].productCodeRes == CV(netWorkList)[i].productCodeExpected) {
              /*
              *   Product code is OK.
                */
                contNormBoot = true;
              } else {
              /*
              *   Either time out on response or unexpected data received.
                */
                CV(netWorkList)[i].nodeBootRes = M; /* Actual ProductCode (object 1018h) of the slave node did not match*/
                CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
                contNormBoot = false;
              }
            } else {
            /*
            * We are not expecting the stVendorId, continue normal boot!
              */
              contNormBoot = true;
            }
            if (contNormBoot) {
              CV(netWorkList)[i].bootState = waitingForRevNmber;
              if (CV(netWorkList)[i].revisionNumberExpected) {
                coStackRes = coReadObjectSDO(CHP i + 1, CO_OBJDICT_IDENTITY, CO_IDENTITY_REV_NUMBR_SUBIDX);
                if (coStackRes  != coRes_OK) {
                  coError(CHP 1, 1, 2, 3, 4);
                  CV(netWorkList)[i].bootState = stNotInUse;
                }
              }
            }
          }
          break;
        case waitingForRevNmber:
        /*
        *   Rev number expected is found at 0x1F87 : nodeId.
          */
          if (CV(netWorkList)[i].revisionNumberRes || !CV(netWorkList)[i].revisionNumberExpected) {
            if (CV(netWorkList)[i].revisionNumberExpected) {
              if (CV(netWorkList)[i].revisionNumberRes == CV(netWorkList)[i].revisionNumberExpected) {
              /*
              * Revnmr is OK.
                */
                contNormBoot = true;
              } else {
              /*
              * Faulty rev-number!
                */
                CV(netWorkList)[i].nodeBootRes = N; /* Actual RevisionNumber (object 1018h) of the slave node did not match*/
                CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
                contNormBoot = false;
              }
            } else {
            /*
            *   We are not expecting the revnumber, continue normal boot!
              */
              contNormBoot = true;
            }
            if (contNormBoot) {
              CV(netWorkList)[i].bootState = waitingForSerialNmbr;
              if (CV(netWorkList)[i].serialNumberExpected) {
                coStackRes = coReadObjectSDO(CHP i + 1, CO_OBJDICT_IDENTITY, CO_IDENTITY_SER_NUM_SUBIDX);
                if (coStackRes  != coRes_OK) {
                  coError(CHP 1, 1, 2, 3, 4);
                  CV(netWorkList)[i].bootState = stNotInUse;
                }
              }
            }
          }
          break;
        case waitingForSerialNmbr:
        /*
        *   Rev number expected is found at 0x1F88H : nodeId.
          */
          if (CV(netWorkList)[i].serialNumberRes || !CV(netWorkList)[i].serialNumberExpected) {
            if (CV(netWorkList)[i].serialNumberExpected) {
              if (CV(netWorkList)[i].serialNumberRes == CV(netWorkList)[i].serialNumberExpected) {
              /*
              *   Serial number is OK.
                */
                contNormBoot = true;
              } else {
              /*
              *   Faulty serial number!
                */
                CV(netWorkList)[i].nodeBootRes = O; /* Actual SerialNumber (object 1018h) of the slave node did not match*/
                CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
                contNormBoot = false;
              }
            } else {
            /*
            *   We are not expecting the serial number, continue normal boot!
              */
              contNormBoot = true;
            }
            // qqq We should check the keep-alive bit for this node
            // At this point there should not be any nodes with this active, 
            // but it is a check point that should be present.
            if (contNormBoot) {
            /*
            *   If we should check the SW version or not is slighty different
            *   then for all other objects since it is detemined with flags.
              */
              if (CV(netWorkList)[i].slaveAssignment & CO_MASTER_MUST_CHECK_SW_VERSION) {
                // If we must check software version we also must expect date and time
                if (!((CV(netWorkList)[i].softwareDateExpected) && 
                  (CV(netWorkList)[i].softwareTimeExpected))) {
                  // No date or time to verify against
                  CV(netWorkList)[i].nodeBootRes = G; // Application software version Date or Time did not exist
                  CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
                }
                else {
                  CV(netWorkList)[i].bootState = waitingForSoftwareDate;
                  if (CV(netWorkList)[i].softwareDateExpected) {
                    coStackRes = coReadObjectSDO(CHP i + 1, CO_OBJDICT_VER_APPL_SW, CO_VER_APPL_SW_DATE_SUBIDX);
                    if (coStackRes  != coRes_OK) {
                      coError(CHP 1, 1, 2, 3, 4);
                      CV(netWorkList)[i].bootState = stNotInUse;
                    }
                  }
                }
              } else {
              /*
              *   We will skip the software version check for this node!
                */
                CV(netWorkList)[i].bootState = waitingForConfigurationDate;
                if (CV(netWorkList)[i].configurationDateExpected) {
                  coStackRes = coReadObjectSDO(CHP i + 1, CO_OBJDICT_VER_CONFIG, CO_CONF_DATE_SUBIDX);
                  if (coStackRes  != coRes_OK) {
                    coError(CHP 1, 1, 2, 3, 4);
                    CV(netWorkList)[i].bootState = stNotInUse;
                  }
                }
              }
            }
          }
          break;
        case waitingForSoftwareDate:
        /*
        *   Software date expected is found at 1F53h : nodeId.
          */
          if (CV(netWorkList)[i].softwareDateRes || !CV(netWorkList)[i].softwareDateExpected) {
            if (CV(netWorkList)[i].softwareDateExpected) {
              if (CV(netWorkList)[i].softwareDateRes == CV(netWorkList)[i].softwareDateExpected) {
              /*
              *   Software date is OK.
                */
                contNormBoot = true;
              } else {
              /*
              *   Faulty software date or it was not received at all!
                */
                CV(netWorkList)[i].nodeBootRes = H; /* Actual application software version Date or Time did not match*/
                CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
                contNormBoot = false;
              }
            } else {
            /*
            *   We are not expecting the software date, continue normal boot!
              */
              contNormBoot = true;
            }
            if (contNormBoot) {
              CV(netWorkList)[i].bootState = waitingForSoftwareTime;
              if (CV(netWorkList)[i].softwareTimeExpected) {
                coStackRes = coReadObjectSDO(CHP i + 1, CO_OBJDICT_VER_APPL_SW, CO_VER_APPL_SW_TIME_SUBIDX);
                if (coStackRes  != coRes_OK) {
                  coError(CHP 1, 1, 2, 3, 4);
                  CV(netWorkList)[i].bootState = stNotInUse;
                }
              }
            }
          }
          break;
        case waitingForSoftwareTime:
        /*
        *   Software time expected is found at 1F54h : nodeId.
          */
          if (CV(netWorkList)[i].softwareTimeRes || !CV(netWorkList)[i].softwareTimeExpected) {
            if (CV(netWorkList)[i].softwareTimeExpected) {
              if (CV(netWorkList)[i].softwareTimeRes == CV(netWorkList)[i].softwareTimeExpected) {
              /*
              *   Software time is OK.
                */
                contNormBoot = true;
              } else {
              /*
              *   Faulty software time or it was not received at all!
                */
                CV(netWorkList)[i].nodeBootRes = H; /* Actual application software version Date or Time did not match*/
                CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
                contNormBoot = false;
              }
            } else {
            /*
            *   We are not expecting the software time, continue normal boot!
              */
              contNormBoot = true;
            }
            if (contNormBoot) {
              CV(netWorkList)[i].bootState = waitingForConfigurationDate;
              if (CV(netWorkList)[i].configurationDateExpected) {
                coStackRes = coReadObjectSDO(CHP i + 1, CO_OBJDICT_VER_CONFIG, CO_CONF_DATE_SUBIDX);
                if (coStackRes  != coRes_OK) {
                  coError(CHP 1, 1, 2, 3, 4);
                  CV(netWorkList)[i].bootState = stNotInUse;
                }
              }
            }
          }
          break;
        case waitingForConfigurationDate:
        /*
        *   configuration date expected is found at 0x1F87 : nodeId.
          */
          if (CV(netWorkList)[i].configurationDateRes || !CV(netWorkList)[i].configurationDateExpected) {
            if (CV(netWorkList)[i].configurationDateExpected) {
              if (CV(netWorkList)[i].configurationDateRes == CV(netWorkList)[i].configurationDateExpected) {
              /*
              *   Configuration date is OK.
                */
                contNormBoot = true;
              } else {
              /*
              *   Faulty configuration date!
                */
                CV(netWorkList)[i].nodeBootRes = J; /* Automatic configuration download failed */
                CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
                contNormBoot = false;
              }
            } else {
            /*
            *   We are not expecting the configuration date, continue normal boot!
              */
              contNormBoot = true;
            }
            if (contNormBoot) {
              CV(netWorkList)[i].bootState = waitingForConfigurationTime;
              if (CV(netWorkList)[i].configurationTimeExpected) {
                coStackRes = coReadObjectSDO(CHP i + 1, CO_OBJDICT_VER_CONFIG, CO_CONF_TIME_SUBIDX);
                if (coStackRes  != coRes_OK) {
                  coError(CHP 1, 1, 2, 3, 4);
                  CV(netWorkList)[i].bootState = stNotInUse;
                }
              }
            }
          }
          break;
        case waitingForConfigurationTime:
        /*
        *   Expected configuration time is found at 0x1F87 : nodeId.
          */
          if (CV(netWorkList)[i].configurationTimeRes || !CV(netWorkList)[i].configurationTimeExpected) {
            if (CV(netWorkList)[i].configurationTimeExpected) {
              if (CV(netWorkList)[i].configurationTimeRes == CV(netWorkList)[i].configurationTimeExpected) {
              /*
              *   Configuration time is OK.
                */
                contNormBoot = true;
              } else {
              /*
              *   Faulty configuration time!
                */
                CV(netWorkList)[i].nodeBootRes = J; /* Automatic configuration download failed */
                CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
                contNormBoot = false;
              }
            } else {
            /*
            *   We are not expecting the configuration time, continue normal boot!
              */
              contNormBoot = true;
            }
            if (contNormBoot) {
            /*
            *   If we have a configured heart beat time for the specific node
            *   we will wait for the reception of a heartbeat.
              */
              uint16  hbTime;
              /*
              *   This is a workaround for not having to read all the entries
              *   of 0x1016 (DS301) which is a very stupid layout of an object.
              */
              hbTime = readHeartbeatTime(CHP nodeId);
              if (hbTime) {
                CV(netWorkList)[i].bootState = waitingForHeatbeat;
              } else {
                if (CV(NMTStartup) & CO_APP_STARTS_SLAVE_NODES) {
                  // We are not allowed to start the node. Next step is to
                  // jump to the "is mandatory" step.
                  CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
                } else {
                  // The stack should start the node.
                  CV(netWorkList)[i].bootState = sendNMTstartNode;
                }
              }
            }
          }
          break;
        case waitingForHeatbeat:
          isHeartReceived = isHeartbeatReceived(CHP nodeId);
          if (isHeartReceived) {
          /*
          *   If the heartbeat is received we should look at object
          *   0x1f80:3. If that bit is set then we are NOT allowed to
          *   start ANY node, that is handeled by the application.
            */
            if (CV(NMTStartup) & CO_APP_STARTS_SLAVE_NODES) {
            /*
            *  We are not allowed to start the node. Next step is to
            *  jump to the "is mandatory" step.
              */
              CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
            } else {
            /*
            *   The stack should start the node.
              */
              CV(netWorkList)[i].bootState = sendNMTstartNode;
            }
          } else {
            isHeartbeatTimedOut = isHeartbeatTimeouted(CHP nodeId);
            /*
            *   If heartbeat has elapsed we should end this boot with error-code K
            */
            if (isHeartbeatTimedOut) {
              CV(netWorkList)[i].nodeBootRes = K;
              CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
            }
          }
          break;
        case sendNMTstartNode:
        /*
        *   This stage simply sends the start node for the specific node.
          */
          coStackRes = comNMT(CHP nodeId, coNMT_StartRemoteNode);
          if (coStackRes == coRes_OK) {
            CV(netWorkList)[i].bootState = checkIfNodeIsMandatory;
          } else {
            coError(CHP 1, 1, 2, 3, 4);
            CV(netWorkList)[i].bootState = stNotInUse;
          }
          break;
        case checkIfNodeIsMandatory:
          bootErrCode = CV(netWorkList)[i].nodeBootRes;

          if ((!(CV(netWorkList)[i].slaveAssignment & CO_NODE_IS_MANDATORY_SLAVE)) && 
              (CV(netWorkList)[i].oneSecDelayTimeStamp == 0)) {
            // Regardless of whether the boot is successful or not for non-mandatory slaves, 
            // we remove the node from the boot list to enable the master to continue
            RV(networkListIdxBootNode)--;
          }

          if (bootErrCode == noErrorReported) {
          /*
          *   If there has been no problems with the boot exit this boot.
            */
            if (CV(netWorkList)[i].slaveAssignment & CO_NODE_IS_MANDATORY_SLAVE) {
              // Optional slaves are accounted for previously
              RV(networkListIdxBootNode)--;
            }

            CV(netWorkList)[i].bootState = stNotInUse;
          } else if (bootErrCode == B) {
            KfHalTimer bootStartTimeStamp = RV(bootStartTimeStamp);
            KfHalTimer now = kfhalReadTimer();
            /*
            *   Err code B means that node has not been introduced to the nextwork. We will look
            *   until the "NMTBootTime" has elapsed if it is mandatory otherwize until the end of time
            */
            if (RV(intNMTBootTime) && ((now - bootStartTimeStamp) > RV(intNMTBootTime))) {
              if (CV(netWorkList)[i].slaveAssignment & CO_NODE_IS_MANDATORY_SLAVE) {
              /*
              *   The boot time for mandatory nodes has elapsed. This means
              *   that we will have to abort this boot.
                */
              RV(isInstanceBooting) = false; /* Halt Boot */
              appNotifyBootError(CHP nodeId, B);
            }
              else {
                uint32 nodeResponse = 0; /* qqq: Tbd */
                if (appIsNonMandatoryBootErrOk(CHP nodeId, bootErrCode, nodeResponse)) {
                  // Application told us to ignore this problem and continue boot!
                  CV(netWorkList)[i].oneSecDelayTimeStamp = kfhalReadTimer();
                  CV(netWorkList)[i].bootState = waitingForOneSecDelay;
                } else {
                  RV(isInstanceBooting) = false; /* Halt Boot */
                  appNotifyBootError(CHP nodeId, bootErrCode);
                } 
              }
            } else {
            /*
            *  Wait one second (in state waitingForOneSecDelay) and then
            *  try to boot this node again.
            *  If "CV(intNMTBootTime)" is zero we will continue
              */
              CV(netWorkList)[i].oneSecDelayTimeStamp = kfhalReadTimer();
              CV(netWorkList)[i].bootState = waitingForOneSecDelay;
            }
          } else {
          /*
          *   A node with an error code C--> has been found.
          *   Report this to the application
            */
            appNotifyBootError(CHP nodeId, bootErrCode);
          }

          // Clear the boot error
          CV(netWorkList)[i].nodeBootRes = noErrorReported;

          break;
        case waitingForOneSecDelay:
          now = kfhalReadTimer();
          if (now-CV(netWorkList)[i].oneSecDelayTimeStamp > kfhalMsToTicks(1000)) {
          /*
          *  The asyncronus 1 sec delay  has elapsed, try connect to the
          *  node again.
            */
            CV(netWorkList)[i].bootState = stNodeBootInit;
          }
          break;
        default:
          coError(CHP 1, 1, 2, 3, 4);
          break;
        }
      }
    }
  }

  if (RV(networkListIdxBootNode) == 0) {
    // All nodes are booted
    if ((CV(NMTStartup) & CO_ASK_APP_BEFORE_OPERATIONAL_MODE) || 1) {  /* qqq: "1" (not allowed to happen in Sandvik impl.) */
      isOkEnterOperationalMode = appBootFinishedIsOkEnterOperational(CHP_S);
      if (isOkEnterOperationalMode) {
        /* CoStackResult coSetState(CH coOpState opState); */
        coStackRes = coSetState(CHP coOS_Operational);
        if (coStackRes != coRes_OK) {
          /* Unexpected response from coSetState(..)  */
          coError(CHP 1, 1, 2, 3, 4);
          // TODO: How do we handle this???
          return;
        }
      }
    } else {
      /* qqq: Should never happen for Sandvik!!!! */
      coStackRes = coSetState(CHP coOS_Operational);
      if (coStackRes != coRes_OK) {
        /* Unexpected response from coSetState(..)  */
        coError(CHP 1, 1, 2, 3, 4);
        // TODO: How do we handle this???
        return;
      }
    }

    if((CV(NMTStartup) & CO_START_REMOTE_NODE_FOR_ALL_NODES) &&  0) {  /* qqq: "0" (not allowed to happen in Sandvik impl.) */
      coStackRes = comNMT(CHP 0, coNMT_StartRemoteNode);
      if (coStackRes != coRes_OK) {
        coError(CHP 1, 1, 2, 3, 4);
        // TODO: How do we handle this???
        return;
      }
    }

    appBootFinishedSuccessfully(CHP_S);

    // Reduce network list to indicate that all mandatory slaves are booted
    RV(networkListIdxBootNode)--;
  }

  if (iNumberOfBootingSlaves == 0) {
    // All slaves have booted including the optional
    RV(isInstanceBooting) = false;
  }
}
/*
 *  This function looks for the given "nodeId" in the nodeInfo array.
 *  When the specific node id has been found the index in the
 *  nodeInfo array is returned.
 */
static int findNodeIdIdxInNodeInfo(CH uint8 nodeId) {
  int i;
  /*
   *  CV(nodeCount) contains the number of valid entries in the
   *  nodeInfo array. 
   */
  for (i = 0; i < MAX_HEARTBEAT_NODES; i++){
    /*
     *  Return the index for the specific node id.
     */
    if (CV(heartbeatNodes)[i].nodeId == nodeId) {   
      return i;
    }
  }
  return -1;
}

/*
 *  This function returns the heartbeat time for the given node id. 
 */
static uint16 readHeartbeatTime(CH uint8 nodeId) {
  int     i = -1;
  bool    isNodeIdFound = false;
  uint16  heartbeatTime = 0;

  i = findNodeIdIdxInNodeInfo(CHP nodeId);
  if (i >= 0) {
    isNodeIdFound = true;
  } else {
    isNodeIdFound = false;
  }
  if (isNodeIdFound && CV(heartbeatNodes)[i].isHeartBeatControlRunning) {
    heartbeatTime = CV(heartbeatNodes)[i].heartBeatPeriod;
  }
  return heartbeatTime;
}



/*
 *  This functions tells if the given nodeId (arg) has put a heatbeat
 *  message on the bus.
 */
static bool isHeartbeatReceived(CH uint8 nodeId) {
  int     i = -1;
  bool    isNodeIdFound = false;
  bool    isHeartbeatReceived = false;

  i = findNodeIdIdxInNodeInfo(CHP nodeId);
  if (i >= 0) {
    isNodeIdFound = true;
  } else {
    isHeartbeatReceived = false;
    isNodeIdFound = false;
  }
  if (isNodeIdFound) {
    isHeartbeatReceived = CV(heartbeatNodes)[i].alive;
  }
  return isHeartbeatReceived;
}



/*
 * This function check if the node has timed out. If the specific node
 * is not found in the list of nodes that will be controlled the
 * function returns false. 
 */
static bool isHeartbeatTimeouted(CH uint8 nodeId) {
  int     i = -1;
  bool    isNodeIdFound = false;
  bool    heartbeatTimeout = false;

  i = findNodeIdIdxInNodeInfo(CHP nodeId);
  if (i >= 0) {
    isNodeIdFound = true;
  } else {
    heartbeatTimeout = false;
    isNodeIdFound = false;
  }
  if (isNodeIdFound) {
    heartbeatTimeout = CV(heartbeatNodes)[i].alive;
  }
  return heartbeatTimeout;
}

#endif /* DS302_BOOT */


#if HEARTBEAT_CONSUMER
/*
*   This function handles all the object writes to objIdx 0x1016.
*   This object is used for configuring the node's heartbeat control on
*   the network, the specific object is described in the DS301 p.9-77.
*   writeConsumrHeartbeatTime
*/
static coErrorCode writeConsumerHeartbeatTime(CH uint8 subIdx, uint32 val) {
  coErrorCode     res = CO_ERR_GENERAL;

  /*
  *   Since we do have a number of slots for setting up node guarding we can
  *   not just take any slot meaning. the upper limit is given by maxSubIdx.
  *   (CV(nodeCount) carries number of configurations written and "+1" is
  *   because subIdx 0 does not carry any settings.
  */
  if (subIdx == 0) {
    res = CO_ERR_OBJECT_READ_ONLY;
  } else if (subIdx <= MAX_HEARTBEAT_NODES) {
	int j;
	int i = subIdx-1;
    uint16 heartBeatTime = (uint16)val & 0x0000ffff;
    uint8 remoteNodeId = (uint8)((val & 0x00ff0000) >> 16);
	/* Check if node id is already configured in another slot, it can only be configured once */
	for( j = 0; j < MAX_HEARTBEAT_NODES; j++){
		if( heartBeatTime != 0 && CV(heartbeatNodes)[j].nodeId == remoteNodeId && i != j){
			return CO_ERR_GENERAL_PARAMETER_INCOMPATIBILITY;
		}
	}
	/* Write slot */
	if (remoteNodeId <= 127) {
	  CV(heartbeatNodes)[i].nodeId = remoteNodeId;
	  CV(heartbeatNodes)[i].heartBeatPeriod = heartBeatTime;
      CV(heartbeatNodes)[i].heartBeatTimeout = kfhalMsToTicks(heartBeatTime);
      CV(heartbeatNodes)[i].isHeartBeatControlRunning = false;
	  CV(heartbeatNodes)[i].state = coOS_Unknown;
	  updateUsedCobIds(CHP_S);
      res = CO_OK;
	} else {
      res = CO_ERR_GENERAL;
    }
  } else {
    /*
     * Given subIdx is too high
     */
    res = CO_ERR_INVALID_SUBINDEX;
  }
  return res;
}

/*
 *  readConsumerHeartbeatTime
 *    
 *	Reads from object dictionary index 0x1016 goes here
 */
static uint32 readConsumerHeartbeatTime(CH uint8 subIdx, coErrorCode *err) 
{
  uint32 res = 0;
  uint8 maxSubIdx;
  coErrorCode localerr;

  maxSubIdx = MAX_GUARDED_NODES;
  localerr = CO_ERR_GENERAL;
  
  if (subIdx == 0) {
    res = MAX_GUARDED_NODES;
    localerr = CO_OK;
  } else if (subIdx <= maxSubIdx) {
    int i = subIdx-1;
    uint16 heartBeatTime;
    uint8 remoteNodeId;
    heartBeatTime = CV(heartbeatNodes)[i].heartBeatPeriod;
    remoteNodeId = CV(heartbeatNodes)[i].nodeId;
    res = ((uint32)remoteNodeId << 16) | heartBeatTime;
    localerr = CO_OK;
  } else {
    /* 
    * SubIdx is either negative (unsiged!) or too high 
    */
    localerr = CO_ERR_INVALID_SUBINDEX;
  }
  *err = localerr;
  return res;
}
#endif /* HEARTBEAT_CONSUMER */

#if IDENTITY_OBJECT
/*
*   This function is used for communicating with object index 0x1018. The
*   structure of this object is described by DS301 p. 9.78
*/
static uint32 readIdentityObject(CH uint16 objIdx, uint8 subIdx, coErrorCode *err) {
  uint32 res = 0;
  if (objIdx == CO_OBJDICT_IDENTITY) {
    *err = CO_OK;
    switch (subIdx) {
      case CO_IDENTITY_NUM_ENTRIES:
        res = 4;
        break;
      case CO_IDENTITY_VENDOR_ID_SUBIDX:
        res = CV(vendorId);
        break;
      case CO_IDENTITY_PROD_CODE_SUBIDX:
        res = CV(productCode);
        break;
      case CO_IDENTITY_REV_NUMBR_SUBIDX:
        res = CV(revisionNumber);
        break;
      case CO_IDENTITY_SER_NUM_SUBIDX:
        res = CV(serialNumber);
        break;
      default:
        *err = CO_ERR_INVALID_SUBINDEX;
        break;
    }
  } else {
    *err = CO_ERR_GENERAL;
  }
  return res;
}

/*
*   This function is used for communicating with object index 0x1018. The
*   structure of this object is described by DS301 p. 9.78.
*   All these values are write only in the specification but since the objects
*   needs to be configured at some point it is possible to override this
*   limitation also.
*/
static coErrorCode writeIdentityObject(CH uint16 objIdx, uint8 subIdx, uint32 val, bool overrideReadOnly) {
  coErrorCode res = CO_ERR_GENERAL;
  if (objIdx == CO_OBJDICT_IDENTITY) {
    res = CO_OK;
    if (overrideReadOnly) {
      switch (subIdx) {
        case CO_IDENTITY_NUM_ENTRIES:
          res = CO_ERR_OBJECT_READ_ONLY; /* Number of entries can never be overrided. */
          break;
        case CO_IDENTITY_VENDOR_ID_SUBIDX:
          CV(vendorId) = val;
          res = CO_OK;
          break;
        case CO_IDENTITY_PROD_CODE_SUBIDX:
          CV(productCode) = val;
          res = CO_OK;
          break;
        case CO_IDENTITY_REV_NUMBR_SUBIDX:
          CV(revisionNumber) = val;
          res = CO_OK;
          break;
        case CO_IDENTITY_SER_NUM_SUBIDX:
          CV(serialNumber) = val;
          res = CO_OK;
          break;
        default:
          res = CO_ERR_INVALID_SUBINDEX;
          break;
      }
    } else {
      /*
      *  All entries of this object is read only.
      */
      res = CO_ERR_OBJECT_READ_ONLY;
    }
  } else {
    res = CO_ERR_GENERAL;
  }
  return res;
}
#if 0
/*
*   This function accesses and configures the identity object. The reason why
*   the identity object is not being accessed thru the 'normal' coWriteObject(...)
*   is becuase it is read only. This function overrides this read only
*   attribute and should be used during the init of the node.
*/
CoStackResult coConfigureIdentityObject(CH uint32 vendorId, uint32 prodCode, uint32 revNum, uint32 serialNum) {
  coErrorCode     res = CO_ERR_GENERAL;
  CoStackResult   stackRes = coRes_CAN_ERR;
  bool            isOK = true;
  /*
  *   Override the write only limitation and write to CO_OBJDICT_IDENTITY (0x1018)
  *
  *   Writing Vendor Id:
  */
  res = writeIdentityObject(CHP CO_OBJDICT_IDENTITY, CO_IDENTITY_VENDOR_ID_SUBIDX, vendorId, true);
  if (res != CO_OK) {
    stackRes = coRes_ERR_CONFIGURE_VENDOR_ID;
    isOK = false;
  }
  /*
  *   Writing product code.
  */
  if(isOK) {
    res = writeIdentityObject(CHP CO_OBJDICT_IDENTITY, CO_IDENTITY_PROD_CODE_SUBIDX, prodCode, true);
    if (res != CO_OK) {
      stackRes = coRes_ERR_CONFIGURE_PRODUCT_CODE;
      isOK = false;
    }
  }
  /*
  *   Writing revision number.
  */
  if(isOK) {
    res = writeIdentityObject(CHP CO_OBJDICT_IDENTITY, CO_IDENTITY_REV_NUMBR_SUBIDX, revNum, true);
    if (res != CO_OK) {
      stackRes = coRes_ERR_CONFIGURE_REVISION_NUMBER;
      isOK = false;
    }
  }
  /*
  *   Writing serial number.
  */
  if(isOK) {
    res = writeIdentityObject(CHP CO_OBJDICT_IDENTITY, CO_IDENTITY_SER_NUM_SUBIDX, serialNum, true);
    if (res != CO_OK) {
      stackRes = coRes_ERR_CONFIGURE_SERIAL_NUMBER;
      isOK = false;
    }
  }
  /*
  *   If everything is OK fix correct return code for that.
  */
  if (isOK) {
    stackRes = coRes_OK;
  }
  return stackRes;
}
#endif /* #if 0 */

#endif
//#endif /* DS302_BOOT */

#if MULTIPLEXED_PDO
/*
*   This function returns the dispather list index for the given
*   dispatcher list number.
*/
static int findDispListNum(CH uint8 dispListNum, bool createIfNotFound) {
  int i = 0;
  int res = -1;
  bool dispListNumFound = false;
  bool emptySlotFound = false;
  coDispListObjT *dispList = CV(dispList);

  while(i < MAX_NUMBER_OF_DISP_LIST_SUBIDXES && !dispListNumFound && !emptySlotFound) {
    if (dispList[i].number == dispListNum) {
      dispListNumFound = true;
    } else if (dispList[i].number == 0) {
      emptySlotFound = true;
    } else {
      i++;
    }
  }
  if (dispListNumFound) {
    res = i;
  } else if (emptySlotFound && createIfNotFound) {
    dispList[i].number = dispListNum;
    res = i;
  }
  return res;
}

/*
*   This function finds the dispatcher list number for a given
*   object index. If the dispatcherlist that corresponds to the
*   given object index dispatcherlist is being created.
*/
static int findDispList(CH uint16 objIdx, bool createIfNotFound) {
  uint8 dispLstNum;
  int index = -1;

  if (objIdx >= CO_OBJDICT_DISPATCH_LST && objIdx < CO_OBJDICT_MAN_SPEC_PROF_AREA) {
    dispLstNum = (uint8) objIdx - CO_OBJDICT_DISPATCH_LST + 1;
    index = findDispListNum(CHP dispLstNum, createIfNotFound);
  }
  return index;
}

/*
*   This function looks for the index in the subindex array for the given
*   dispatcher list index. If the given subindex is not found it is being
*   created if the flag "createIfNotFound" is set.
*/
static int findDispListSubIdx(CH int dispListIdx, uint8 subIdx, bool createIfNotFound) {
  dispListSubIdxData *objP = NULL;
  bool isSubIdxFound = false;
  bool isFreeSubSlotFound = false;
  int res = -1;
  int i = 0;

  if (dispListIdx >= 0 && dispListIdx < MAX_DISP_LIST_ENTRIES) {
    objP = CV(dispList)[dispListIdx].subIdxData;
    /*
    *   Parse the configured subindexes and see if the one we are
    *   looking for is found.
    */
    while (i < MAX_NUMBER_OF_DISP_LIST_SUBIDXES && !isSubIdxFound && !isFreeSubSlotFound) {
      if (objP[i].subIdx == subIdx) {
        isSubIdxFound = true;
      } else if (objP[i].subIdx == 0) {
        isFreeSubSlotFound = true;
      } else {
        i++;
      }
    }
  }
  if (isSubIdxFound) {
    res = i;
  } else if (isFreeSubSlotFound && createIfNotFound) {
    objP[i].subIdx = subIdx;
    res = i;
  }
  return res;
}

/*
*   This buffer parse the given buffer and checks for that any of the 
*   slots is not equal to zero.
*   If all the slots of buf is equal to zero function return true, else false.
*/
static bool isBufValNotZero(uint8 *buf, uint8 bufLen) {
  int i = 0;
  while (i < bufLen) {
    if (buf[i]) {
      return true;
    }
    i++;
  }
  return false;
}

/*
*   This function shift all the values of the list one step. to delete one entry.
*/
static coErrorCode deleteDispListSubListIdx (CH int dispListIdx, int dispListSubDataIdx) {
  int i;
  coErrorCode res;
  if (dispListIdx < MAX_DISP_LIST_ENTRIES &&  dispListSubDataIdx < MAX_NUMBER_OF_DISP_LIST_SUBIDXES) {
    i = dispListSubDataIdx;
    while ( i < (MAX_NUMBER_OF_DISP_LIST_SUBIDXES-1) ) {
      CV(dispList)[dispListIdx].subIdxData[i] = CV(dispList)[dispListIdx].subIdxData[i+1];
      i++;
    } 
    CV(dispList)[dispListIdx].subIdxData[MAX_NUMBER_OF_DISP_LIST_SUBIDXES-1].subIdx = 0;
    res = CO_OK;
  } else {
    res = CO_ERR_GENERAL;
  }
  return res;
}

/*
*   This function implements the writes to the dispatcher list. Since this stack must
*   be useable in embedded systems it is not implemted as a uint64 write, instead 
*   the communication to the objects is handled via a uint8 buffer. 
*/
static coErrorCode writeDispatcherList(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len) {
  int dLstIdx = -1;
  int siDataIdx = -1;
  dispListDataT dispListVars;
  CoStackResult coRes;
  coErrorCode res;
  bool isBufVal = false;


  isBufVal = isBufValNotZero(buf, len);
  if (subIdx) {
    coRes = extrDispListVarsFrBuf(&dispListVars, buf, len);
    if (coRes == coRes) {
      if (isBufVal) {
        dLstIdx = findDispList(CHP objIdx, true); /* Create if not found. */
      } else {
        dLstIdx = findDispList(CHP objIdx, false); /* Do not create if not found. */
      }
      if (IS_POSITIVE(dLstIdx)) {
        if (isBufVal) {
          siDataIdx = findDispListSubIdx(CHP dLstIdx, subIdx, true);
        } else {
          siDataIdx = findDispListSubIdx(CHP dLstIdx, subIdx, false);
        }
        if (IS_POSITIVE(siDataIdx)) {
          if (isBufVal) {
            CV(dispList)[dLstIdx].subIdxData[siDataIdx].vars = dispListVars;
            res = CO_OK;
          } else {
            /*
            *  Application has written zero to a subIdx that existed. We must 
            *  therefore delete this entry from the dynamic list. The reason for
            *  this is because zero is considerd to be end of list - in that 
            *  way we are very efficient at runtime, hopefully :-)
            */
            res = deleteDispListSubListIdx (CHP dLstIdx, siDataIdx);
          }
        } else {
          /*
          *  We end up here if we try to write to a SUBINDEX that could not be found
          *  or created OR if we try to write zero to a slot that did not exist,
          *  which of course is OK (and that is what we therefore should return)
          */
          if (isBufVal) {
            res = CO_ERR_OUT_OF_MEMORY;
          } else {
            res = CO_OK;
          }
        }
      } else {
        /*
        *  We end up here if we try to write to a OBJECT INDEX that could not be found
        *  or created OR if we try to write zero to a slot that did not exist,
        *  which of course is OK (and that is what we therefore should return)
        */
        if (isBufVal) {
          res = CO_ERR_OUT_OF_MEMORY;
        } else {
          //printf("b");
          res = CO_OK;
        }
      }
    } else {
      res = CO_ERR_GENERAL;
    }
  } else {
    res = CO_ERR_OBJECT_READ_ONLY; /* subidx == 0 */
  }
  return res;
}

/*
*   This function implements the reads to the dispatcher list. Since this stack must
*   be useable in embedded systems it is not implemted as a uint64 reads, instead 
*   the communication to the objects is handled via a uint8 buffer. 
*/
#if MULTIPLEXED_PDO
static coErrorCode readDispatcherList(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len) {
  int dLstIdx = -1;
  int siDataIdx = -1;
  coErrorCode res;
  dispListDataT dispListVars;
  dispListSubIdxData *disLstSubDatP;

  dLstIdx = findDispList(CHP objIdx, true);
  if (IS_POSITIVE(dLstIdx)) {
    siDataIdx = findDispListSubIdx(CHP dLstIdx, subIdx, true);
    if (IS_POSITIVE(siDataIdx)) {
      if (subIdx == 0) {
        uint8 maxSubIdx = 0;
        int j;
        /*
        *   Find the highest configured subidex...
        */
        disLstSubDatP = CV(dispList)[dLstIdx].subIdxData;
        maxSubIdx = 0;        
        for(j=0; j <= siDataIdx; j++) {
          if (maxSubIdx < disLstSubDatP[j].subIdx) {
            maxSubIdx = disLstSubDatP[j].subIdx;
          }
        }
        val2buf(maxSubIdx, buf, len);
        res = CO_OK;
      } else {
        /*
        *   Specific subindex has been found.
        */
        disLstSubDatP = &(CV(dispList)[dLstIdx].subIdxData[siDataIdx]);
        dispListVars = disLstSubDatP->vars;
        packDispListVarsToBuf(dispListVars, buf, len);
        res = CO_OK;
      }
    } else {
      res = CO_ERR_OUT_OF_MEMORY;
    }
  } else {
    res = CO_ERR_OUT_OF_MEMORY;
  }
  return res;
}
#endif

/* 
*   This function implemets the scanner object writes, meaning write to 
*   objects from 0x1fa0 -> 0x1fcf. The object scanner list is described in
*   DS301 p.11-10.
*/
static coErrorCode writeScannerList(CH uint16 objIdx, uint8 subIdx, uint32 val) {
  coErrorCode res = CO_ERR_GENERAL;
  int i = -1;
  int ii = -1;

  if (objIdx >= CO_OBJDICT_SCANNER_LIST && objIdx < CO_OBJDICT_DISPATCH_LST) {
    if (subIdx) {
      if (val) {
        i = findScanListObjMemIdx(CHP objIdx, true); /* create if not found! */
      } else {
        i = findScanListObjMemIdx(CHP objIdx, false); /* do not create if not found! */
      }
      if (IS_POSITIVE(i)) {
        if (val) {
          ii = findScanListSubMemIdx(CHP i, subIdx, true); /* create if not found! */
        } else {
          ii = findScanListSubMemIdx(CHP i, subIdx, false); /* do not create if not found! */
        }
        if (IS_POSITIVE(ii)) {
          if (val) {
            coObjScanListDataEntrT *oP;
            oP = &CV(scanList)[i].subIdxData[ii];
            oP->vars.blockSize =  (uint8) ((val & SCAN_LST_BLK_SIZE_BM) >> 24);
            oP->vars.confObjIdx = (uint16)((val & SCAN_LST_OBJ_IDX_BM) >> 8);
            oP->vars.confSubIdx = (uint8)(val & SCAN_LST_SUB_IDX_MB);
            res = CO_OK;
          } else {
            /*
            *   If we write zero to a slot that has been configured we should delete it
            *   from our dynamic list.
            */
            if (deleteScanListSubMemIdx(CHP i, subIdx)) {
              res = CO_OK;
            } else {
              res = CO_ERR_GENERAL; /* We could not delete a entry that was found! */
            }
          }
        } else {
          /*
          *   We end up here if the mem index for the given subIdx 
          *   was never found or could be created. 
          *   If val is zero, this is OK because then we have written 
          *   0 to a OD entry that did not exit. If val carries a
          *   val not equal to zero, the only reason for this should 
          *   be that we ran out of free mem slots.
          */
          if (val) {
            res = CO_ERR_OUT_OF_MEMORY;
          } else {
            /*
            *   We end up here if we tried to write zero to slot that has never 
            *   been configured (and therefor it was not found). This is 
            *   ofcourse perfectly OK.
            */
            res = CO_OK; 
          }
        } 
      } else {
        if (val) {
          res = CO_ERR_OUT_OF_MEMORY;
        } else {
          res = CO_OK;  /* We tried to write zero to a slot that did not exist, NO harm done! */
        }
      }
    } else {
      /*
      *   SubIdx = 0 is read only, tell application this.
      */
      res = CO_ERR_OBJECT_READ_ONLY;
    }
  } else {
    res = CO_ERR_OBJECT_DOES_NOT_EXISTS;  /* Object is out of range for fn "writeScannerList". */
  }
  return res;
}


/* 
*   This function implemets the scanner object reads, meaning write to 
*   objects from 0x1fa0 -> 0x1fcf. The object scanner list is described in
*   DS301 p.11-10.
*/
static uint32 readScannerList(CH uint16 objIdx, uint8 subIdx, coErrorCode *coErr) {
  bool isObjFound = false;
  uint32 res = 0;
  int i = -1;
  int ii = -1;

  i = findScanListObjMemIdx(CHP objIdx, false);
  if (IS_POSITIVE(i)) {
    ii = findScanListSubMemIdx(CHP i, subIdx, false);
    if (IS_POSITIVE(ii)) {
      coObjScanListDataEntrT *oP = &CV(scanList)[i].subIdxData[ii];;
      /*
       *  Object is found. Build the 32 bit val by "or" the diffrent values.
       *  If subIdx 0 is requested the largest configured subIdx is returned. 
       */
      if (subIdx == 0) {
        int j;
        oP = &CV(scanList)[i].subIdxData[0];
        res = 0;
        for(j=0; j <= ii; j++) {
          if (res < oP->subIdx) {
            res = oP->subIdx;
          }
          oP++;
        }
        isObjFound = true;
      } else {
        oP = &CV(scanList)[i].subIdxData[ii];
        res = (oP->vars.blockSize << 24) | (oP->vars.confObjIdx << 8) | oP->vars.confSubIdx;
        *coErr = CO_OK; 
        isObjFound = true;
      }
    }
  }
  if (!isObjFound) {
    if (objIdx >= CO_OBJDICT_SCANNER_LIST || objIdx < CO_OBJDICT_DISPATCH_LST) {
      /* 
      *  The given object index have never been configured and
      *  therefore it does exsist. But the object is in range
      *  and therefore we return 0 and simultate that it was found
      *  but it was not configured.
      */
      *coErr = CO_OK;
      res = 0;
    } else {
      /*
      * The object was out of the operational limits of readScannerList().
      */
      *coErr = CO_ERR_OBJECT_DOES_NOT_EXISTS;
      res = 0;
    }
  }
  return res;
}

/*
*   This function deletes the given scanlist index at given
*   subindex. When done the function packs the list to save 
*   memory.
*/
static bool deleteScanListSubMemIdx(CH int scListIdx, uint8 subIdx) {
  coScannerListObjT *scanLstP;
  int i;
  bool res = false;

  if (scListIdx >= 0 && scListIdx < MAX_NUMBER_OF_SCAN_LISTS) {
    scanLstP = &(CV(scanList)[scListIdx]);
    i = findScanListSubMemIdx(CHP scListIdx, subIdx, false);
    if (i >= 0) {
      while (i < (MAX_NUMBER_OF_SCAN_LIST_SUBIDXES-1)) {
        scanLstP->subIdxData[i] = scanLstP->subIdxData[i+1];
        i++;
      }
      scanLstP->subIdxData[(MAX_NUMBER_OF_SCAN_LIST_SUBIDXES-1)].subIdx = 0;
      res = true;
    } 
  } 
  return res;
} 

/*
*   This function finds the subindex connected to the given scannerlist memory
*   index (scListIdx) (x in the following equation; 
*
*   CV(scanList)[scListIdx].subIdxData[x].
*
*   If the subindex does not exist and the flag "creaIfNotFnd" is true the subindex
*   entry is created.
*/
static int findScanListSubMemIdx(CH int scListIdx, uint8 subIdx, bool creaIfNotFnd) {
  int i = 0;
  int res = -1;
  bool reqSubIdxFnd = false;
  bool freeMemSlotFnd = false;
  /*
  *   Verify that "scListIdx" is in range to avoid crash if
  *   missuse of function is done.
  */
  if (scListIdx >= 0 && scListIdx < MAX_NUMBER_OF_SCAN_LISTS) {
    coScannerListObjT *scanLstP = &(CV(scanList)[scListIdx]);
    while (i < MAX_NUMBER_OF_SCAN_LIST_SUBIDXES && !reqSubIdxFnd && !freeMemSlotFnd) {
      if (scanLstP->subIdxData[i].subIdx == subIdx) {
        reqSubIdxFnd = true;
      } else if (scanLstP->subIdxData[i].subIdx == 0) {
        freeMemSlotFnd = true;
      } else {
        i++;
      }
    }
    if (reqSubIdxFnd) {
      res = i;
    } else if (freeMemSlotFnd && creaIfNotFnd) {
      scanLstP->subIdxData[i].subIdx = subIdx;
      res = i;
    } else {
      res = -1;
    }
  } else {
    coError(CHP 2,2,2,2,2);
    res = -1;
  }
  return res;
}

/*
*   This function looks for the given scanListNo and 
*   returns the index from the "CV(scanList)" when it
*   has been found.
*/
static int findScanListNumMemIdx(CH uint8 scanListNo, bool creaIfNotFnd) {
  bool reqScanListNumFound = false;
  bool emptySlotFound = false;
  uint8 i=0;
  int res = -1;
  /*
  *   Parse the list (until a free slot has been found (==0)) 
  *   and see if the given scanList could be found. 
  *   If it is found return the corrseponding value of i.
  */
  while ((i < MAX_NUMBER_OF_SCAN_LISTS) && !reqScanListNumFound && !emptySlotFound) {
    if (CV(scanList)[i].number == scanListNo) {
      res = i;
      reqScanListNumFound = true;
    } else if (CV(scanList)[i].number == 0) {
      emptySlotFound = true;  
    } else {
      i++;
    }
  }
  if (reqScanListNumFound) {
    res = i;
  } else { 
    if (emptySlotFound && creaIfNotFnd) {
      CV(scanList)[i].number = scanListNo;
      res = i;
    } else {
      res = -1;
    }
  } 
  return res;
}

/*
*   This function looks for the given objIdx in the range of
*   0x1fa0->0x1fcf (this is where will find the object scanner
*   lists).   
*/
static int findScanListObjMemIdx(CH uint16 objIdx, bool creaIfNotFnd) {
  int i;
  uint8 scanListNo = (uint8)(objIdx - CO_OBJDICT_SCANNER_LIST + 1);
  i = findScanListNumMemIdx(CHP scanListNo, creaIfNotFnd);
  return i;
}

/*
*   This function is used for extracting the data from a received and detemined 
*   to be a SAM MPDO.
*/
static CoStackResult extractDataFromSAMPDO(coSamPdoFrameDataT *resP, uint8 *buf, uint8 dlc) // May produce a warning depending on stack configuration
{
  CoStackResult res = coRes_ERR_MESSAGE_DLC;
  if (dlc > 4) {
    uint8 dataFieldLenMPDO;
    /*
    *   The dlc is equal to 8 which is neccary to be able to 
    *   extract address, index and subindex.
    */
    resP->nodeId = buf[0];
    resP->objIdx = (uint16) buf2val(&buf[1], 2);
    resP->subIdx = buf[3];
    /*
    *   Get the data field of the received CAN message. I think the
    *   multiplexed PDOs always have a dlc = 8, but let's be compatible
    *   with dlc 5-8.
    */
    dataFieldLenMPDO = dlc-4;
    if (dataFieldLenMPDO) {
      resP->data = buf2val(buf+4, dataFieldLenMPDO);
    } else {
      res = coRes_ERR_NO_VALID_BUF_DATA;
    }  
  } else {
    res = coRes_ERR_MESSAGE_DLC;
  }
  return res;
}

/*
*   This function is extracts the buffer data and sorts the variables into a 
*   coMPDOFrameDataT structure.
*/
static CoStackResult extractDataFromMPDO (coMPDOFrameDataT *mpdoP, uint8 *buf, uint8 dlc) {
  CoStackResult res = coRes_CAN_ERR;
  if (dlc > 4) {
    if (buf[0] & DESTINATION_ADDRESSING_MODE_FLG) {
      mpdoP->type = destAddrMode;
    } else {
      mpdoP->type = srcAddrMode;
    }
    mpdoP->addr = buf[0] & MULTIPLEXED_PDO_NODE_ID_BM;
    mpdoP->objIdx = (uint16)buf2val(&buf[1], 2);
    mpdoP->subIdx = buf[3];
    mpdoP->dataField = buf2val(buf+4, (dlc-4));
    res = coRes_OK;
  } else {
    res = coRes_ERR_MESSAGE_DLC;
  }
  return res;
}

/*
*   isSrcObjFoundInDispatcherList handles the dispatcher list which is the 
*   >>cross reference<< between the remote source object dictionary and the
*   object dictionary of us, the consumer.
*   If source object could be converted function return true and the local 
*   object index is written into the , else function
*   return false! 
*/
static bool isSrcObjFoundInDispatcherList(CH int dispLstIdx, uint16 remObjIdx, uint8 remSubIdx, uint8 remNodeId, uint16 *locObjIdx, uint8 *locSubIdx) {
  int i = 0;
  bool isObjFndInDispList = false;
  coDispListObjT *dispListP = NULL;
  dispListDataT *varP = NULL;

  if (IS_POSITIVE(dispLstIdx) && dispLstIdx < MAX_DISP_LIST_ENTRIES) {
    dispListP = &CV(dispList)[dispLstIdx];
    /*
    *   Parse out local dispatcher list and look for the given object.
    */
    while (i < MAX_NUMBER_OF_DISP_LIST_SUBIDXES && !isObjFndInDispList) {
      varP = &dispListP->subIdxData[i].vars;
      if (varP->sendObjIdx == remObjIdx) {
        /*
        *   see CANopen DS301, p.11-9: In the dispacher list the application
        *   does not neccarly need to configure all the subIdxes, it is 
        *   also possible to configure blocks (ranges) of subIdxes.
        */
        uint8 minSubIdx = varP->sendSubIdx;
        uint8 maxSubIdx = varP->sendSubIdx + varP->blockSize;
        if (remSubIdx >= minSubIdx && remSubIdx <= maxSubIdx) {
          uint8 offset = remSubIdx - minSubIdx;
          *locObjIdx = varP->localObjIdx;
          *locSubIdx = varP->localSubIdx + offset; /* qqq: The spec is LAME, correct? */
          isObjFndInDispList = true;
        }
      } else {
        i++;
      }
    }
  }
  return isObjFndInDispList;
}

/*
*   This function determines if the received Multiplexed PDO is in the  
*   sorce addressing mode or if it is a destination addressing mode.
*
*   Destination Address Mode (DAM)
*   ==============================
*   The addr and the m field of the MPDO refers to the consumer. This allows access to
*   the consumerÕs Object Dictionary in an SDO-like manner. With  addr = 0 , it allows 
*   multicasting and broadcasting, to write into the Object Dictionaries of more than 
*   one node simultaneously, without having a PDO for each single object.
* 
*
*   Source Address Mode (SAM)
*   =========================
*   The addr and the m field of the MPDO refers to the producer. Only one producer 
*   MPDO of this type is allowed for each node.
*   Transmission type has to be 254 or 255.
*   The producer uses an Object Scanner List in order to know, which objects are to send.
*   The consumer uses an Object Dispatcher List as a cross reference.
* 
*/
static CoStackResult handleMPDO (CH uint8 *buf, uint8 dlc) {
  coMPDOFrameDataT mpdo;
  CoStackResult res = coRes_CAN_ERR;

  res = extractDataFromMPDO(&mpdo, buf, dlc);
  if (res == coRes_OK) {
    if (mpdo.type == destAddrMode) {
     /* 
      *  DAM PDO: We should only write to our own OD if the destination nodeId is equal to
      *  our own ID or if the destination is equal to zero (0 = broadcast).
      *  If we are unsuccessful in our write the application callback function 
      *  appIndicateFaltyWriteReceivedDAMPDO is called.
      */
      if (mpdo.addr == 0 || RV(nodeId) == mpdo.addr) {
        coErrorCode res;
        res = coWriteObject(CHP mpdo.objIdx, mpdo.subIdx, mpdo.dataField);
        if (res != CO_OK) {
          (void)appIndicateFaltyWriteReceivedDAMPDO(CHP mpdo.objIdx, mpdo.subIdx, mpdo.dataField);
        }
      }
    } else if (mpdo.type == srcAddrMode) {
      uint16 locObjIdx;
      uint8 locSubIdx;
     /* 
      *  SAM PDO: The addr and the m field of the MPDO refers to the producer. Transmission type 
      *  has to be 254 or 255. The producer uses an Object Scanner List in order to know, which 
      *  objects are to send.The consumer uses an Object Dispatcher List as a cross reference.
      */
      if (isSamPdoInDispatcherList(CHP mpdo, &locObjIdx, &locSubIdx)) {
        coErrorCode err;
        /*
        *   The SAM PDO was found in the dispatcher list, write the given val to
        *   our local object dictionary.
        */
        err = coWriteObject(CHP locObjIdx, locSubIdx, mpdo.dataField);
        if (err) {
          appIndicateFaltyWriteRecSAMMPDO(CHP locObjIdx, locSubIdx, mpdo.dataField, err);         
        }
      }
    }
  }
  return res;
}


/*
*   This function packs all the variables in the dispListDataT-variable structure into
*   the given buffer (buf).
*/
static CoStackResult packDispListVarsToBuf(dispListDataT disLstData, uint8 *buf, uint8 len) {
  CoStackResult res = coRes_ERR_PACK_DISPLIST_TO_BUF_LEN;
  if (len >= 8) {
    val2buf(disLstData.blockSize, &buf[7], 1); /* qqq. LSB */
    val2buf(disLstData.localObjIdx, &buf[5], 2);
    val2buf(disLstData.localSubIdx, &buf[4], 1);
    val2buf(disLstData.sendObjIdx, &buf[2], 2);
    val2buf(disLstData.sendSubIdx, &buf[1], 1);
    val2buf(disLstData.sendNodeId, &buf[0], 1); /* qqq. MSB */ 
    res = coRes_OK;
  } 
  return res;
}

/* 
*   This function extacts all the variabels in a dispListDataT stucture from a 
*   given buffer.   
*/
static CoStackResult extrDispListVarsFrBuf(dispListDataT *resP, uint8 *buf, uint8 len) {
  CoStackResult res;
  /* 
  *   This function extracts the entries of a buffer and interpret it
  *   as if it was a Object dispatching list object (which is a uint64!)
  *
  *   We only accept len to be equal to 8.
  */
  if (len == 8) {
    resP->blockSize = (uint8)buf2val(&buf[7], 1);
    resP->localObjIdx = (uint16)buf2val(&buf[5], 2);
    resP->localSubIdx = (uint8)buf2val(&buf[4], 1);
    resP->sendObjIdx = (uint16)buf2val(&buf[2], 2);
    resP->sendSubIdx = (uint8)buf2val(&buf[1], 1);
    resP->sendNodeId = (uint8)buf2val(&buf[0], 1);
    res = coRes_OK;
  } else {
    res = coRes_ERR_BUFFER_LEN_INVALID;
  }
  return res;
}

/*
*   This function parse the dispatecher list and look if the given SAM MPDO (coMPDOFrameDataT) exists.
*   If it exists the resulting responding local object dictionary entries is written into 
*   the locObjIdxP and locSubIdxP pointeres and fuction returns true.
*/
static bool isSamPdoInDispatcherList(CH coMPDOFrameDataT samPdo, uint16 *locObjIdxP, uint8 *locSubIdxP) {
  coDispListObjT *dspLstP;
  bool isObjFnd = false;
  int i = 0;
  /*
  *   Look for configured dispatcher lists. A dispatcher list is considered to be valid if 
  *   is has a configured "number" (coDispListObjT.number != 0 is considered to be valid).
  *   If object given by the SAM-PDO is found in the dispatcher list isSrcObjFoundInDispatcherList()
  *   returns true and local obj dictionary entries are written to locObjIdx and locSubIdx.
  */
  dspLstP = CV(dispList);
  while (i < MAX_DISP_LIST_ENTRIES && !isObjFnd) {
    if (dspLstP[i].number) {
      uint16 objIdx = samPdo.objIdx;
      uint8 subIdx = samPdo.subIdx;
      uint8 addr = samPdo.addr;
      if (isSrcObjFoundInDispatcherList(CHP i, objIdx, subIdx, addr, locObjIdxP, locSubIdxP)) {
        isObjFnd = true;  
      }
    }
    i++;
  }
  return isObjFnd;
}

/*
*
*   This function checks if the given object is found in the scanner list.
*   If found, function returns true, else false.
*
*/
static bool isObjInScannerList(CH uint16 objIdx, uint8 subIdx) {
  coScannerListObjT *objScLstP = CV(scanList);
  coObjScanListDataVars *varP;
  bool isObjFound = false;
  int i = 0;
  int j = 0;

  while (i < MAX_NUMBER_OF_SCAN_LISTS && !isObjFound) {
    if (objScLstP[i].number) {
      j = 0;
      while (j < MAX_NUMBER_OF_SCAN_LIST_SUBIDXES && !isObjFound) {
        varP = &objScLstP[j].subIdxData->vars;
        if (varP->confObjIdx == objIdx) {
          if (subIdx >= varP->confSubIdx && subIdx <= (varP->confSubIdx + varP->blockSize)) {
            isObjFound = true;
          }
        }
        j++;
      }
    }
    i++;
  }
  return isObjFound;
}

/*
*   This function is used to parse the configured PDOs and look for the PDO
*   that is configured to the the SAM PDO (if any). If successful - the 
*   function will return the PDO's index (PDO[i]). If not SAM PDO is found 
*   the function returns -1!
*/
static int findSAMPDO(CH uint8 dir) {
  int i;
  for (i = 0; i < PDO_MAX_COUNT; i++) {
    if (CV(PDO)[i].dir == dir && checkBM(CV(PDO)[i].status, PDO_STATUS_SAM_PDO)) {
      return i;
    }
  }
  return -1; // -1 means "not found".
}

/*
*
*   This function check if the PDO, at index "i", is OK to put in queue.
*
*/
static CoStackResult checkTxStatusPDO(CH int i) {
  CoStackResult res = coRes_OK;
  coPDO *pdo = &CV(PDO)[i];
#if DYNAMIC_PDO_MAPPING
  if (checkBM(pdo->status, PDO_STATUS_SUB_DISABLED)) {
    res = coRes_ERR_PDO_DISABLED;
  }
#endif
  if (res == coRes_OK) {
    if (checkBM(pdo->status, PDO_STATUS_INHIBIT_ACTIVE)) {  /* Is inhibit active? */
      if (checkBM(pdo->status, PDO_STATUS_QUEUED)) {
        res = coRes_ERR_TPDO_INHIBIT_TIME_NOT_ELAPSED;
      }
      if (res == coRes_OK) {
        KfHalTimer elapsedTime;
        elapsedTime = kfhalReadTimer() - pdo->lastMsgTime;
        if (elapsedTime < pdo->inhibitTime) {
          res = coRes_ERR_TPDO_INHIBIT_TIME_NOT_ELAPSED; 
        }
      }
    }
  }
  return res;
}

/*
*
* This function updates the flag for the PDO at index "i".
*
*/
static CoStackResult setTxStatusPDO(CH int i)   // May produce a warning depending on stack configuration
{
  CoStackResult res = coRes_OK;
  coPDO *pdo = &CV(PDO)[i];
  setBM(pdo->status, PDO_STATUS_QUEUED);
  if (pdo->inhibitTime) {
    setBM(pdo->status, PDO_STATUS_INHIBIT_ACTIVE);
  }
  return res;
}

/*
*
*   This function looks for the PDO that is configured to be Source Addess Mode 
*   multiplexed PDO and tx the SAM MPDO with the given object, subindex and val.
*
*/
static CoStackResult transmitSAMPDO (CH uint16 locObjIdx, uint8 locSubIdx, uint32 val) {
  CoStackResult res = coRes_ERR_NO_SAM_TPDO_FOUND;
  uint32 tag; 
  int i = findSAMPDO(CHP PDO_TX);
  if (IS_POSITIVE(i)) {
    coPDO *pdo = &CV(PDO)[i];
    res = checkTxStatusPDO(CHP i);
    if (res == coRes_OK) {
      coMPDOFrameDataT mpdo;
      uint8 buf[8];
      /*
      *   Copy the SAM MPDO data.
      */
      mpdo.type = srcAddrMode;
      mpdo.objIdx = locObjIdx;
      mpdo.subIdx = locSubIdx;
      mpdo.dataField = val;
      mpdo.addr = RV(nodeId);
      res = packMPDOToBuf(mpdo, buf, sizeof(buf));
      if (res == coRes_OK) {
		tag = ((uint32)i << 16) + pdo->pdoNo;
        res = getCanHalError(coWriteMessage(CHP pdo->COBID, buf, 8, tag));
      }
    }
  }
  return res;
}
#endif  


#if RESTORE_PARAMS


/*
*   writeRestoreParamObj implement object 0x1010 writes, DS301, p.9-70.
*/

/* We write an invalid value to the nv-ram which causes default to be loaded on the next reset communication */
static coErrorCode writeRestoreParamObj(CH uint8 subIdx, uint32 val) { /* DS301, p.9-70, 0x1010. */
  bool isApplBufOk = false;
  uint8 *applBufP = NULL;
  uint16 badData = 0xEEEE; /* We store this into nv-ram to invalidate saved params */

  if (subIdx == 0) {
    return CO_ERR_OBJECT_READ_ONLY;
  }

  if( subIdx > CO_RESTORE_MANUF_PARAMS_MIN){
	return CO_ERR_INVALID_SUBINDEX;
  }

  /* Check signature 0x64616F6C == 'LOAD' */
  if( val != 0x64616F6C){
	return CO_ERR_DATA_CAN_NOT_BE_STORED;	
  }

  if( subIdx == CO_RESTORE_ALL_PARAMS){
	if( appSaveParameters(CHP CO_STORE_COMM_PARAMS, (uint8 *)&badData, sizeof( badData)) == false){
		return CO_ERR_ABORT_SDO_TRANSFER;
	}
	if( appSaveParameters(CHP CO_STORE_APPL_PARAMS, (uint8 *)&badData, sizeof( badData)) == false){
		return CO_ERR_ABORT_SDO_TRANSFER;
	}
	if( appSaveParameters(CHP CO_STORE_MANUF_PARAMS_MIN, (uint8 *)&badData, sizeof( badData)) == false){
		return CO_ERR_ABORT_SDO_TRANSFER;
	}
  }

  if( subIdx == CO_RESTORE_COMM_PARAMS){
	if( appSaveParameters(CHP CO_STORE_COMM_PARAMS, (uint8 *)&badData, sizeof( badData)) == false){
		return CO_ERR_ABORT_SDO_TRANSFER;
	}
  }

  if( subIdx == CO_RESTORE_APPL_PARAMS){
	if( appSaveParameters(CHP CO_STORE_APPL_PARAMS, (uint8 *)&badData, sizeof( badData)) == false){
		return CO_ERR_ABORT_SDO_TRANSFER;
	}
  }

  if( subIdx == CO_RESTORE_MANUF_PARAMS_MIN){
	if( appSaveParameters(CHP CO_STORE_MANUF_PARAMS_MIN, (uint8 *)&badData, sizeof( badData)) == false){
		return CO_ERR_ABORT_SDO_TRANSFER;
	}
  }

  return CO_OK;
}

/*
*   readRestoreParamObj implement object 0x1010 reads, DS301, p.9-70.
*/
static uint32 readRestoreParamObj(CH uint8 subIdx, coErrorCode *err) { /* DS301, p.9-70, 0x1010. */
  uint32 val;
  *err = CO_ERR_GENERAL;
  switch (subIdx) {
    case 0:
      val = 4; /* largest subindex supported */
      *err = CO_OK;
      break;
    case CO_RESTORE_ALL_PARAMS:
    case CO_RESTORE_COMM_PARAMS:
	case CO_RESTORE_APPL_PARAMS:
	case CO_RESTORE_MANUF_PARAMS_MIN:
      val = CO_DEVICE_RESTORES_PARAMS;
      *err = CO_OK;
      break;
    default:
      val = 0;
      *err = CO_ERR_INVALID_SUBINDEX;
      break;
  }
  return val;
}


#endif /* #if RESTORE_PARAMS */



#if STORE_PARAMS

/*
*   This function should return true if the application could successfully store
*   all the communication params. 0x1000-0x1FFF
*/
static bool saveCommParams(CH_S) {
    CoComObjects *chP;
#if CO_MULTIPLE_INSTANCES
    chP = &coComObjects[RV(instanceIdx)];
#else 
    chP = &coComObjects[0];
#endif
	CV(magicNumber) = 0x0D;
	return appSaveParameters(CHP CO_STORE_COMM_PARAMS, (uint8 *)chP, sizeof( coComObjects[0]));
}

/*
*   This function should return true if the application could successfully store
*   all the communication params.
*/
static bool loadCommParams(CH_S) {
    CoComObjects *chP;
#if CO_MULTIPLE_INSTANCES
    chP = &coComObjects[RV(instanceIdx)];
#else 
    chP = &coComObjects[0];
#endif
	if( appLoadParameters(CHP CO_STORE_COMM_PARAMS, (uint8 *)chP, sizeof( coComObjects[0])) != true){
		return false;
	}

	if( CV(magicNumber) != 0x0D){
		return false;
	}

	return true;
}

/*
*    Stores paramters from the "device profile" and "manufacturer" area of the OD. The range of object indexes
*    to store must be specified (usually the manufacturer and device profile range is saved separately)
*/
uint8 paramBuf[STOREPARAM_BUF_SIZE];

static bool saveODParams(CH uint8 area, uint16 lowIndex, uint16 highIndex) {
  const coObjDictEntry *ode;
  CoObjLength pos = 0;

  ode = RV(ODapp);
  if (ode == NULL) {
	  return true;    // There's nothing to store
  }

  paramBuf[pos++] = 0x0D;    /* Magic number for verification */
  paramBuf[pos++] = area;

  /* Iterate through the user object dictionary and save the values to the temporary buffer */
  while (ode->objIdx) {
	  // Calculate size of object (it could be an array)
	  CoObjLength objSize = ode->objDesc->size;
	  if( ode->subIdxCount > 0){
	      objSize *= ode->subIdxCount;
	  }

	  // Skip entries outside of requested range
	  if( ode->objIdx < lowIndex || ode->objIdx > highIndex){
	    ode++;
		continue;
	  }

	  // If object does not fit into RAM buffer we fail
	  if( pos + objSize > STOREPARAM_BUF_SIZE){
		  return false;  /* Buffer size too small -> Increase size of STOREPARAM_BUF_SIZE */
	  }

	  // Copy object into RAM buffer and go on to the next one
	  memcpy( paramBuf + pos, ode->objPtr, objSize);
	  pos += objSize;
	  ode++;
  }

  return appSaveParameters(CHP area, paramBuf, pos);
}

/*
*    Loads paramters from NVRAM into the "manufacturer" area of the OD. Currently that's everything that's not
*    hardcoded into the stack. 0x2000-0x9FFF
*/
static bool loadODParams(CH uint8 area, uint16 lowIndex, uint16 highIndex) {
  const coObjDictEntry *ode;
  CoObjLength size = 0;
  CoObjLength pos = 0;

  ode = RV(ODapp);
  if (ode == NULL) {
	  return true;    // There's nothing to load
  }

  size += 2;  // The "OD" magic number at he beginning of the data

  /* Calculate bytes required */
  while (ode->objIdx) {
	  CoObjLength objSize = ode->objDesc->size;
	  if( ode->subIdxCount > 0){
	      objSize *= ode->subIdxCount;
	  }
	  if( ode->objIdx < lowIndex || ode->objIdx > highIndex){
		ode++;
		continue;
	  }
	  if( size + objSize > STOREPARAM_BUF_SIZE){
		  return false;  /* Buffer size too small -> Increase size of STOREPARAM_BUF_SIZE */
	  }
	  size += objSize;
	  ode++;
  }

  if( appLoadParameters(CHP area, paramBuf, size) != true){
	return false;
  }

  pos = 0;
  if( paramBuf[pos++] != 0x0D || paramBuf[pos++] != area){
	return false;
  }

  ode = RV(ODapp);
  while (ode->objIdx) {
	  CoObjLength objSize = ode->objDesc->size;
	  if( ode->subIdxCount > 0){
	      objSize *= ode->subIdxCount;
	  }

	  // Skip entries outside of requested reange
	  if( ode->objIdx < lowIndex || ode->objIdx > highIndex){
	    ode++;
		continue;
	  }

	  if( pos + objSize > STOREPARAM_BUF_SIZE){
		  return false;  /* Buffer size too small -> Increase size of STOREPARAM_BUF_SIZE */
	  }
	  memcpy( ode->objPtr, paramBuf + pos, objSize);
	  pos += objSize;
	  ode++;
  }

  return true;
}

/*
*   writeStoreParamObj implement object 0x1010 writes, DS301, p.9-70.
*/
static coErrorCode writeStoreParamObj(CH uint8 subIdx, uint32 val) { /* DS301, p.9-70, 0x1010. */

  // The number of entries cannot be set
  if (subIdx == 0) {
    return CO_ERR_OBJECT_READ_ONLY;
  }

  if (val != 0x65766173) {  // 0x65766173 = 'SAVE' in ASCII, see DS301, p.9-70.
    return CO_ERR_DATA_CAN_NOT_BE_STORED;
  }

  switch (subIdx) {
  case CO_STORE_ALL_PARAMS:
    if (!saveCommParams(CHP_S)) {
      return CO_ERR_DATA_CAN_NOT_BE_STORED;
    }
	if( !saveODParams(CHP CO_STORE_APPL_PARAMS, 0x6000, 0x9FFF)){
      return CO_ERR_DATA_CAN_NOT_BE_STORED;	  
	}
	if( !saveODParams(CHP CO_STORE_MANUF_PARAMS_MIN, 0x2000, 0x5FFF)){
      return CO_ERR_DATA_CAN_NOT_BE_STORED;	  
	}
    break;
  case CO_STORE_COMM_PARAMS:
    if (!saveCommParams(CHP_S)) {
      return CO_ERR_DATA_CAN_NOT_BE_STORED;
    }
    break;
  case CO_STORE_APPL_PARAMS:
	if( !saveODParams(CHP CO_STORE_APPL_PARAMS, 0x6000, 0x9FFF)){
      return CO_ERR_DATA_CAN_NOT_BE_STORED;	  
	}
	break;
  case CO_STORE_MANUF_PARAMS_MIN:
	if( !saveODParams(CHP CO_STORE_MANUF_PARAMS_MIN, 0x2000, 0x5FFF)){
      return CO_ERR_DATA_CAN_NOT_BE_STORED;	  
	}
	break;
  default:
    return CO_ERR_INVALID_SUBINDEX;
  }

  return CO_OK;
}

/*
*   readStoreParamObj implement object 0x1010 reads, DS301, p.9-70.
*/
static uint32 readStoreParamObj(CH uint8 subIdx, coErrorCode *err) { /* DS301, p.9-70, 0x1010. */
  uint32 res;
  *err = CO_ERR_GENERAL;
  switch (subIdx) {
    case 0:
      res = 4; /* largest subindex supported */
      *err = CO_OK;
      break;
    case CO_STORE_ALL_PARAMS:
    case CO_STORE_COMM_PARAMS:
    case CO_STORE_APPL_PARAMS:
    case CO_STORE_MANUF_PARAMS_MIN:
      res = CO_DEVICE_STORES_PARAMS_ON_COMMAND;
      *err = CO_OK;
      break;
    default:
      res = 0;
      *err = CO_ERR_INVALID_SUBINDEX;
      break;
  }
  return res;
}
#endif



#if ERROR_BEHAVIOR
/*
*  writeErrorBehaviorObj implements object 0x1029 writes, DS301, page 125.
*/
static coErrorCode writeErrorBehaviorObj(CH uint8 subIdx, uint8 val){
  // The number of entries cannot be set
  if(subIdx == 0) {
    return CO_ERR_OBJECT_READ_ONLY;
  }
  switch( subIdx){
	  case CO_COMM_ERR:
		  CV(CommunicationError) = val;
		  break;
	  default:
		  return CO_ERR_INVALID_SUBINDEX;
  }  
  return CO_OK;
}
/*
*	readErrorBehaviorObj implements object 0x1029 reads, DS301, page 125.
*/
static uint8 readErrorBehaviorObj(CH uint8 subIdx, coErrorCode *err){
  uint8 val = 0;
  switch (subIdx) {
     case 0:
        val = 1; /* Highest sub-index supported */
        break;
     case CO_COMM_ERR:
        val = CV(CommunicationError);
        break;
     default:
		 val = 0;
        *err = CO_ERR_INVALID_SUBINDEX;
        break;
  } 
  return val;

}
#endif


#if TIME_H_IS_AVAILABLE
/*
*   This function updates object 1020 - Config Date. It should be called at the
*   time when a configuration is being stored in the non-voilatile memory.   
*/
static bool updateConfigDate(CH_S) {
  bool res = false;
  struct tm timeStamp1984;
  time_t secElapsedFromJan1970ToNow;
  time_t secElapsedFromJan1970ToJan1984;
  time_t secElapsedFromJan1984ToNow;
  uint32 daysElapsedFromJan1984ToToday;
  /* 
  *   See CANopen DS301 p. 11-3. All timestamps are from Jan 01, 1984.
  */
  timeStamp1984.tm_year = 1984-1900; /* hohohoho */
  timeStamp1984.tm_mon = 1;
  timeStamp1984.tm_mday = 1;
  timeStamp1984.tm_hour = 0;
  timeStamp1984.tm_min = 0;
  timeStamp1984.tm_sec = 0;
  /*
  *   Figure out the no. seconds that have elapsed from Jan 1970 -> Jan 1984.
  */
  secElapsedFromJan1970ToJan1984 = mktime(&timeStamp1984);
  if (IS_POSITIVE(secElapsedFromJan1970ToJan1984)) {
    /*
    *   Figure out the no. seconds that have elapsed from Jan 1970 -> Now.
    */
    secElapsedFromJan1970ToNow = time(NULL);
    if (IS_POSITIVE(secElapsedFromJan1970ToNow)) {
      /*
      *   Figure out the no. seconds that have elapsed from now -> Jan 1984.
      */
      secElapsedFromJan1984ToNow = secElapsedFromJan1970ToNow - secElapsedFromJan1970ToJan1984;
      if (IS_POSITIVE(secElapsedFromJan1984ToNow)) {
        /*
        *   Figure out the no. days from Jan 1984 -> Today.
        */
        daysElapsedFromJan1984ToToday = secElapsedFromJan1984ToNow / 86400;
        /*
        *   Update the variable held by stack.
        */
        CV(verCfgDate) = daysElapsedFromJan1984ToToday;
        res = true;
      }
    }
  }
  return res;
}

/*
*   This function updates object 1020 - Config Time. It should be called at the
*   time when a configuration is being stored in the non-voilatile memory.   
*/
static bool updateConfigTime(CH_S) {
  bool res = false;
  time_t timeStampNow;
  struct tm* timeInParamFormatP;
  uint32 elapsedMsAfterMidnight;
  /*
  *   Get current time.
  */
  time(&timeStampNow);
  timeInParamFormatP = localtime (&timeStampNow); /* localtime is from time.h */
  /*
  *   Convert current time to ms after midnight, CANopen DS301, p.11-3.
  */
  if (timeInParamFormatP) {
    elapsedMsAfterMidnight = (uint32)timeInParamFormatP->tm_hour * 3600 * 1000;
    elapsedMsAfterMidnight += (uint32)timeInParamFormatP->tm_min * 60 * 1000;
    elapsedMsAfterMidnight += (uint32)timeInParamFormatP->tm_sec * 1000;
    /*
    *   Update the local variable held by stack.
    */  
    CV(verCfgTime) = elapsedMsAfterMidnight;
    res = true;
  }
  return res;
}



#endif


/* Layer Setting Services (LSS) according to CiA 305 */
#if LSS_SLAVE

uint32 lssVendorId;
uint32 lssProductCode;
uint32 lssRevisionNumber;
uint32 lssSerialNumber;
uint8 switchStateSelectiveMsg = 0; /* Where in the message sequence are we 1,2,3,4 */


// Called from reset node and reset communication
static void lssSetState(CH uint8 state){
	RV(lssState) = state;
	if( RV(lssState) == LSSinit){
		
		RV(lssState) = LSSwait;
		appIndicateLssState(CHP RV(lssState));
	}
}

// Return 1 to indicate we processed the message and no-one else needs to
// Return 0 to indicate this message was not for us and someone else needs to take a look
static int lssProcessMessage(CH uint32 cobId, uint8 *buf, uint8 dlc){

	if( RV(lssState) == LSSwait){
		if( cobId == 2021 && buf[0] == 4 && buf[1] == 1)
        {
			RV(lssState) = LSSconf;
			appIndicateLssState(CHP RV(lssState));
			return 1;
        }
		/* Messages for switch state selective */
		if( cobId == 2021 && buf[0] == 0x40)
        {
			lssVendorId = buf2val( buf + 1, 4);
			switchStateSelectiveMsg = 1;
			return 1;
        }
		if( cobId == 2021 && buf[0] == 0x41 && switchStateSelectiveMsg == 1)
        {
			lssProductCode = buf2val( buf + 1, 4);
			switchStateSelectiveMsg++;
			return 1;
        }
		if( cobId == 2021 && buf[0] == 0x42 && switchStateSelectiveMsg == 2)
        {
			lssRevisionNumber = buf2val( buf + 1, 4);
			switchStateSelectiveMsg++;
			return 1;
        }
		if( cobId == 2021 && buf[0] == 0x43 && switchStateSelectiveMsg == 3)
        {
			lssSerialNumber = buf2val( buf + 1, 4);
			switchStateSelectiveMsg = 0;
			if( lssVendorId == CV(vendorId) 
				&& lssProductCode == CV(productCode)
				&& lssRevisionNumber == CV(revisionNumber)
				&& lssSerialNumber == CV(serialNumber))
			{
				lssSend(CHP 0x44, 0x0, 0x0);
				RV(lssState) = LSSconf;
				appIndicateLssState(CHP RV(lssState));
			}
			return 1;
        }
	}
	if( RV(lssState) == LSSconf){
	    /* Set NodeId */
		if( cobId == 2021 && buf[0] == 0x11 && buf[1] != 0x0)    // Set Node ID
        {
			if( lssSetNodeId(CHP buf[1]) != 0){
				lssSend(CHP 0x11, 0x1, 0x0);
			} else {
				lssSend(CHP 0x11, 0x0, 0x0);
			}
        }
		/* Set Bitrate */
        if( cobId == 2021 && buf[0] == 0x13)
        {
			if( buf[1] == 0 && buf[2] < 9 && buf[2] != 5){
				LSSV(bitRate) = buf[2];
				lssSend(CHP 0x13, 0x0, 0x0); /* Success */				
			} else {
				lssSend(CHP 0x13, 0x01, 0x00); /* Bitrate not supported */
			}
		}
		/* Switch To New Bitrate */
        if( cobId == 2021 && buf[0] == 0x15)
        {                         
			KfHalTimer startTime = kfhalReadTimer();
			uint32 delay = kfhalMsToTicks( buf[1]);
			
			while( kfhalReadTimer() - startTime < delay)
			{
				/* Wait */
			}

			canSetBaudrate( RV(canChan), LSSV(bitRate));

			startTime = kfhalReadTimer();
			while( kfhalReadTimer() - startTime < delay)
			{
				/* Wait */
			}
        }                
        if( cobId == 2021 && buf[0] == 0x17)    // Store all params
        {
			if( lssStoreParams(CHP_S) == true){
				lssSend(CHP 0x17, 0x0, 0x0);
			} else {
				lssSend(CHP 0x17, 0x2, 0x0);
			}
        }                                            
        if( cobId == 2021 && buf[0] == 4 && buf[1] == 0)
        {
			RV(lssState) = LSSwait;
			appIndicateLssState(CHP RV(lssState));
        }
	}

	return 0; /* Message not processed */
}


static int lssSetNodeId(CH uint8 nodeId){

	if( nodeId < 1 || nodeId > 0x7F )
    {
		return -1;
	}
	
	LSSV(nodeId) = nodeId;     
	
	return 0;
}


static void lssSend(CH uint8 errorCode, uint8 specError, uint8 lssError){
	uint8 buf[8];
	int res;

	buf[0] = errorCode;
	buf[1] = specError;       
	buf[2] = lssError;
	buf[3] = 0;
	buf[4] = 0;
	buf[5] = 0;
	buf[6] = 0;
	buf[7] = 0;

	res = coWriteMessage(CHP 2020, buf, sizeof(buf), 0);
}

/*
*   This function should return true if the application could successfully store
*   all the LSS parameters. Node Id and bitrate.
*/
static bool lssStoreParams(CH_S) {
  bool isApplBufOk = false;
  bool isLssParamsSuccStrd = false;
  uint8 *applBufP = NULL;
  uint16 coLssParamsSize = 0;
  /*
  *   Figure out the size of the "CoInternals" structure (see canopen.c).
  *   and ask the application for the buffer that is required.
  */
  coLssParamsSize = sizeof(CoLssData);
  isApplBufOk = appSaveLssParams(CHP coLssParamsSize, &applBufP);
  if (isApplBufOk && applBufP != NULL) {
    CoLssData *chP;
#if CO_MULTIPLE_INSTANCES
    uint8 chIdx = RV(instanceIdx);
    chP = &coLssData[chIdx];
#else 
    chP = &coLssData[0];
#endif
    /*
    *   Copy the structure to the buffer held by application.
    */
    memcpy((uint8*)applBufP, (uint8*)chP, coLssParamsSize);
  }
  /*
  *   Notify application that the data has been written to the 
  *   non-violatile memory.
  */
  isApplBufOk = appSaveLssParams(CHP 0, NULL);
  if (isApplBufOk) {
    isLssParamsSuccStrd = true;
  }
  return isLssParamsSuccStrd;
}

static bool lssLoadParams(CH_S) {
  bool isLssParamsRestoreOK = false;
  bool isApplBufOK = false;
  uint16 applBufSize = 0;
  uint8 *applBufP = NULL;
  /*
  *   Ask the application if it has any object that we can restore. 
  *   What we want is a pointer to the buffer/non violatile mem that
  *   we gave the app a while back.
  */ 
  isApplBufOK = appLoadLssParams(CHP &applBufSize, &applBufP);
  if (isApplBufOK && applBufP != NULL && applBufSize == sizeof(CoLssData)) {
    CoLssData *chP;
   /*
    *   We do have a valid area, copy this structure to our co
    */
#if CO_MULTIPLE_INSTANCES
    uint8 chIdx = RV(instanceIdx);
    chP = &coLssData[chIdx];
#else
    chP = &coLssData[0];
#endif
    /*
    *   Copy the structure to our CoInternals structure.
    */
    memcpy((uint8*)chP, (uint8*)applBufP, applBufSize);
    isLssParamsRestoreOK = true;
  }
  return isLssParamsRestoreOK;
}

#endif /* LSS_SLAVE */

CanId usedCobIds[MAX_COB_IDS];

static void updateUsedCobIds( CH_S){
	int num = 0;
	int i;
	
	if( num < MAX_COB_IDS){
		usedCobIds[num++] = 0x000; /* NMT */
	}

	if( num < MAX_COB_IDS){
		usedCobIds[num++] = 0x7E5; /* LSS Master -> Slave */
	}

	if( num < MAX_COB_IDS){
		usedCobIds[num++] = 0x600 + RV(nodeId); /* Default SDO Client */
	}

#if EMERGENCY_MASTER || EMERGENCY_CONSUMER_OBJECT
    for( i = 0; i < MAX_EMCY_GUARDED_NODES; i++){
		if( CV(emcyNodes)[i].COBID != 0 && num < MAX_COB_IDS){
			usedCobIds[num++] = CV(emcyNodes)[i].COBID; /* EMCY */	
		}
	}
#endif

	/* Nodeguarding RTR */
#if NODE_GUARDING
	if( num < MAX_COB_IDS){
		usedCobIds[num++] = ((0x700 + RV(nodeId)) | CANHAL_ID_RTR); 
	}
#endif

	if( num < MAX_COB_IDS){
		usedCobIds[num++] = CV(syncCOBID); /* SYNC */	
	}

	/* RPDOs */
	for (i = 0; i < PDO_MAX_COUNT; i++){
    	if (CV(PDO)[i].dir == PDO_RX && num < MAX_COB_IDS) {
			usedCobIds[num++] = CV(PDO)[i].COBID; /* RPDO */		
		}
  	}

	/* TPDOs with RTR */
	for( i = 0; i < PDO_MAX_COUNT; i++){
    	if( CV(PDO)[i].dir == PDO_TX
			&& (CV(PDO)[i].COBID & (cobRTRdisallowed + cobPDOinvalid)) == 0
			&& num < MAX_COB_IDS)
		{
			usedCobIds[num++] = (CV(PDO)[i].COBID | CANHAL_ID_RTR); /* TPDO RTR*/		
		}
  	}

	/* Heartbeat consumer nodes */
	for( i = 0; i < MAX_HEARTBEAT_NODES; i++){
		if( CV(heartbeatNodes)[i].nodeId > 0 && num < MAX_COB_IDS){
	    	usedCobIds[num++] = CV(heartbeatNodes)[i].nodeId + 0x700; /* NMT Error Control */
		}
	}
		
	canSetUsedCanIds( RV(canChan), usedCobIds, num);
	canUpdateFilters( RV(canChan));
}


// Codewarrior throws a warning if we #if when not even defined
#ifdef MODULE_TEST
#if MODULE_TEST
unsigned char TEST_coCanCallback(CanCbType cbType, const CANMessage *msg){
 return coCanCallback( RV(canChan), cbType, msg);
}
#endif
#endif
