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
** CANopen protocol declarations
*/
#ifndef __CANOPEN_H
#define __CANOPEN_H

#include "kfCommon.h"
#include "kfCanHal.h"
#include "kfHal.h"
#include "coConfig.h"

#define CONV_FROM_BYTES(v) v*8
#define BITSBYTES(n) (n ? 1+(n-1)/8 : 0)


/* SDO Transfer comiling parameters */
#define CO_SDO_BUF_SIZE (0)
#define NO_SIZE ((CoObjLength)-1)

#if CO_MULTIPLE_INSTANCES
  #define CV(v) ((CoRuntimeData*)ch)->comObjects->v /* Use CV(n) to access object dictionary variable n */
  #define LSSV(v) ((CoRuntimeData*)ch)->lssData->v /* Use RV(n) to access runtime variable n */
  #define RV(v) ((CoRuntimeData*)ch)->v /* Use RV(n) to access runtime variable n */
  #define CHP ch, /* Pass CHP as the first parameter when a function with "CH " first in the parameter list is called */
  #define CHP_S ch
/*  #define CO_INST_MAX 4  */
#else
  #define CV(v) coComObjects[0].v
  #define LSSV(v) coLssData[0].v
  #define RV(v) coRuntime[0].v
  #define CHP
  #define CHP_S
/*  #define CO_INST_MAX 1  */
#endif

/* Implementation of the Dyanmic Mapping of PDOs */

#if DYNAMIC_PDO_MAPPING
  #define PDO_MAP CV(dynPdoMap)
#else
  #define PDO_MAP RV(pdoMap)
#endif

/* --- configuration values --- */

/* Emergency Implementation */
//#define CO_OUT_OF_MEM             1
//#define CO_EMCY_NODE_NOT_FOUND    2
//#define CO_EMCY_NODE_OUT_OF_RANGE 3

/* Arguments for appSDOReadResponse() function so that
 * appSDOReadResponseExpedited() don't have to be implemented */


/* Define extra bits in the COB id
* The values ending with a 'P' are used when getting values via the CAN bus as
* the COBIDs always are 32-bit then.
*/
#if USE_EXTENDED_CAN
 #define cobPDOinvalid 0x80000000 /* Marks that the PDO is invalid */
 #define cobRTRdisallowed 0x40000000
 #define cobExtended 0x20000000L
 #define cobIdMask 0x3FFFFFFFL
 /* Used when the value checked always is 32 bits */
 #define cobPDOinvalidP cobPDOinvalid
 #define cobRTRdisallowedP cobRTRdisallowed
 #define cobExtendedP cobExtended
 #define cobIdMaskP cobIdMask
#else
 #define cobPDOinvalid 0x8000 /* Marks that the PDO is invalid */
 #define cobRTRdisallowed 0x4000
 #define cobExtended 0x2000L
 #define cobIdMask 0x3FFFL 
/* Used when the value checked always is 32 bits */
 #define cobPDOinvalidP 0x80000000
 #define cobRTRdisallowedP 0x40000000
 #define cobExtendedP 0x20000000L
 #define cobIdMaskP 0x3FFFFFFFL
#endif
typedef CanId cobIdT;
typedef uint32 CoObjLength;

/* Object dictionary indices */
#define CO_OBJDICT_SDO_SERVER               0x1200   /* First object index */
#define CO_OBJDICT_SDO_SERVER_LEN 0x80
#define CO_OBJDICT_SDO_CLIENT               0x1280
#define CO_OBJDICT_SDO_CLIENT_LEN 0x80
#define CO_OBJDICT_PDO_RX_COMM              0x1400
#define CO_OBJDICT_PDO_RX_COMM_LEN 0x200
#define CO_OBJDICT_PDO_RX_MAP               0x1600
#define CO_OBJDICT_PDO_RX_MAP_LEN 0x200
#define CO_OBJDICT_PDO_TX_COMM              0x1800
#define CO_OBJDICT_PDO_TX_COMM_LEN 0x200
#define CO_OBJDICT_PDO_TX_MAP               0x1a00
#define CO_OBJDICT_PDO_TX_MAP_LEN 0x200
#define CO_OBJDICT_PDO_TX_MAP_END           0x1bff

#define CO_OBJDICT_PREDEFINED_ERR_FIELD     0x1003 /* Emegency Produced errors */ 
#define CO_OBJDICT_SYNC_COBID               0x1005
#define CO_OBJDICT_SYNC_PERIOD              0x1006
#define CO_OBJDICT_SYNC_WINDOW_LEN          0x1007
#define CO_OBJDICT_MANUF_DEVICE_NAME        0x1008
#define CO_OBJDICT_MANUF_HW_VERSION         0x1009
#define CO_OBJDICT_MANUF_SW_VERSION         0x100A
#define CO_OBJDICT_PRODUCER_HEARTBEAT_TIME  0x1017
#define CO_OBJDICT_GUARD_TIME               0x100c
#define CO_OBJDICT_LIFETIME_FACTOR          0x100d
#define CO_OBJDICT_TIMESTAMP_COBID			0x1012
#define CO_OBJDICT_EMCY_COBID               0x1014
#define CO_OBJDICT_EMCY_INHIBIT_TIME        0x1015
#define CO_OBJDICT_CONSUMER_HEARTBEAT_TIME  0x1016
#define CO_OBJDICT_DEVICE_TYPE              0x1000
#define CO_OBJDICT_ERROR_REGISTER           0x1001
#define CO_OBJDICT_IDENTITY                 0x1018  /* Object Type 0x023 */
  #define CO_IDENTITY_NUM_ENTRIES             0x00
  #define CO_IDENTITY_VENDOR_ID_SUBIDX        0x01
  #define CO_IDENTITY_PROD_CODE_SUBIDX        0x02
  #define CO_IDENTITY_REV_NUMBR_SUBIDX        0x03
  #define CO_IDENTITY_SER_NUM_SUBIDX          0x04
#define CO_OBJDICT_EMCY_CONSUMER            0x1028
  #define CO_EMCY_CONSUMER_INACTIVE     0x80000000  
#define CO_OBJDICT_ERR_BEHAVIOR             0x1029
  #define CO_COMM_ERR						   0x1
#define CO_OBJDICT_STORE_PARAMS             0x1010
  #define CO_STORE_ALL_PARAMS                  0x1
  #define CO_STORE_COMM_PARAMS                 0x2
  #define CO_STORE_APPL_PARAMS                 0x3
  #define CO_STORE_MANUF_PARAMS_MIN            0x4
  #define CO_STORE_MANUF_PARAMS_MAX           0x7F
  #define CO_DEVICE_STORES_PARAMS_AUTOMATICLY    2
  #define CO_DEVICE_STORES_PARAMS_ON_COMMAND     1
#define CO_OBJDICT_RESTORE_PARAMS           0x1011
  #define CO_RESTORE_ALL_PARAMS                0x1
  #define CO_RESTORE_COMM_PARAMS               0x2
  #define CO_RESTORE_APPL_PARAMS               0x3
  #define CO_RESTORE_MANUF_PARAMS_MIN           0x4
  #define CO_RESTORE_MANUF_PARAMS_MAX          0x7F
  #define CO_DEVICE_RESTORES_PARAMS			     1

/* MPDO implementation */
#define CO_OBJDICT_SCANNER_LIST             0x1fa0
#define CO_OBJDICT_DISPATCH_LST             0x1fd0


#define CO_OBJDICT_MAN_SPEC_PROF_AREA       0x2000



  /* DS 301, p.11-3 */
  #define CO_OBJDICT_VER_CONFIG                       0x1020
    #define CO_CONF_DATE_SUBIDX                         0x01
    #define CO_CONF_TIME_SUBIDX                         0x02
  #define CO_OBJDICT_VER_APPL_SW                      0x1f52
    #define CO_VER_APPL_SW_DATE_SUBIDX                  0x01
    #define CO_VER_APPL_SW_TIME_SUBIDX                  0x02
  #define CO_OBJDICT_NMT_EXPECTED_SW_DATE             0x1f53 /* No. days since January 1, 1984. */
  #define CO_OBJDICT_NMT_EXPECTED_SW_TIME             0x1f54 /* No. milliseconds after midnight (00:00). */
  #define CO_OBJDICT_NMT_STARTUP                      0x1f80
    /* Flags in obj CO_OBJDICT_NMT_STARTUP (0x1f80) */
    #define CO_NODE_IS_NMT_MASTER                       0x01    /* Is the current node the NMT master? */
    #define CO_START_REMOTE_NODE_FOR_ALL_NODES          0x02    /* If set all nodes will be started
                                                                * with the NMT command "start remote node".
                                                                * If flag is not set explicit start of assigned slaves
                                                                * will be performed. */
    #define CO_ASK_APP_BEFORE_OPERATIONAL_MODE          0x04    /* if flag is set stack will ask app if jump into
                                                                 * operational mode is OK. */
    #define CO_APP_STARTS_SLAVE_NODES                   0x08    /* If this bit is set the application will start
                                                                 * the nodes else stack will start nodes. */
    #define CO_SLAVE_AUTOSTART                          0x08    /* If this is the only bit set in the whole register it
																 * means this is a slave that should autostart (same bit as 
																 * above) */
  #define CO_OBJDICT_NMT_NETWORK_LIST                 0x1f81  /* SubIdx 1-127 represents corresponding node. */
    #define CO_NODE_IS_SLAVE                            0x01
    #define CO_ON_ERR_INFORM_APP_AND_START_ERR_CNTRL    0x02
    #define CO_ON_ERR_DO_NOT_START_NETWORK              0x04
    #define CO_NODE_IS_MANDATORY_SLAVE                  0x08
    #define CO_MASTER_KEEP_ALIVE                        0x10
    #define CO_MASTER_MUST_CHECK_SW_VERSION             0x20
    #define CO_MASTER_ALLOWED_TO_DOWNLOAD_NEW_SW        0x40
  #define CO_OBJDICT_NMT_STARTUP_EXPECT_DEVICETYPE    0x1f84
  #define CO_OBJDICT_NMT_STARTUP_EXPECT_VENDOR_ID     0x1f85
  #define CO_OBJDICT_NMT_STARTUP_EXPECT_PRODUCT_CODE  0x1f86  
  #define CO_OBJDICT_NMT_STARTUP_EXPECT_REV_NUMBER    0x1f87
  #define CO_OBJDICT_NMT_STARTUP_EXPECT_SERIAL_NUMBER 0x1f88
  #define CO_OBJDICT_NMT_BOOTTIME                     0x1f89

/* Object Dictionary Object Definitions */
#define CO_ODOBJ_NULL           0
#define CO_ODOBJ_DOMAIN         2
#define CO_ODOBJ_DEFTYPE        5
#define CO_ODOBJ_DEFSTRUCT      6
#define CO_ODOBJ_VAR            7
#define CO_ODOBJ_ARRAY          8
#define CO_ODOBJ_RECORD         9

/* Some object dictionary data types */
#define CO_ODTYPE_BOOLEAN 0x001
#define CO_ODTYPE_INT8 0x0002
#define CO_ODTYPE_INT16 0x0003
#define CO_ODTYPE_INT32 0x0004
#define CO_ODTYPE_UINT8 0x0005
#define CO_ODTYPE_UINT16 0x0006
#define CO_ODTYPE_UINT32 0x0007
#define CO_ODTYPE_REAL32 0x0008
#define CO_ODTYPE_VISIBLE_STRING 0x0009
#define CO_ODTYPE_OCTET_STRING 0x000A
#define CO_ODTYPE_UNICODE_STRING 0x000B
#define CO_ODTYPE_TIME_OF_DAY 0x000C
#define CO_ODTYPE_TIME_DIFFERENCE 0x000D
/* 0x000E is reserved */
#define CO_ODTYPE_DOMAIN 0x000F
#define CO_ODTYPE_INT24 0x0010
#define CO_ODTYPE_REAL64 0x0011
#define CO_ODTYPE_INT40 0x0012
#define CO_ODTYPE_INT48 0x0013
#define CO_ODTYPE_INT56 0x0014
#define CO_ODTYPE_INT64 0x0015
#define CO_ODTYPE_UINT24 0x0016
/* 0x0017 is reserved */
#define CO_ODTYPE_UINT40 0x0018
#define CO_ODTYPE_UINT48 0x0019
#define CO_ODTYPE_UINT56 0x001A
#define CO_ODTYPE_UINT64 0x001B
#define CO_ODTYPE_PDO_COMM_PAR 0x0020
#define CO_ODTYPE_PDO_MAPPING 0x0021
#define CO_ODTYPE_SDO_PAR 0x0022
#define CO_ODTYPE_IDENTITY 0x0023

/* Constructs the value of coObjDictEntry.structure */
#define CO_ODDEF(obj, type) ((type << 8)+obj)

/* Get the object code or data type from a structure entry */
#define CO_ODOBJCODE(structure) (structure & 0xFF)
#define CO_ODTYPE(structure) (structure >> 8)


/* Statusflags of the status entry in the PDO[] table */
#define PDO_STATUS_INHIBIT_ACTIVE     0x01    /* bit 0 */
#define PDO_STATUS_QUEUED             0x02    /* bit 1*/
#define PDO_STATUS_SUB_DISABLED       0x04    /* bit 2 */
#define PDO_STATUS_DAM_PDO            0x08    /* bit 3, Destination Address Mode (DAM) */
#define PDO_STATUS_SAM_PDO            0x10    /* bit 4, Source Address Mode (SAM) */
#define PDO_STATUS_MPDO               0x18    /* bit 3&4, For convinence! */
#define PDO_STATUS_COBID_DISABLED	  0x20	  /* bit 5 if cobid defines disabled */	 // Added by PN
#define PDO_STATUS_RPDO_MONITORED	  0x40	  /* bit 6 if RPDO timout monitoring active (Set to 1 when RPDO received, set to 0 when timout occures) */


/*
*   For the PDO mapping the following subIdx'es are predefined.
*/

#define SUB_0_DAMPDO    255
#define SUB_0_SAMPDO    254




/* Macros used to construct and read the PDO map vector.
*  The length can not exceed 64 bits (0x40), so we use high values of the length
*  for special purposes.
*  The first entry for each PDO has objIdx set to the pdo number (0..511),
*  len is set to CO_PDOMAP_RX or CO_PDOMAP_TX.
*  PDO numbering starts at 0, PDO1 is numbered 0.
*  A map object consists of 32 bits: (objIdx:16, subIdx:8, len:8).
*/
typedef struct {
  uint16 objIdx;
  uint8 subIdx;
  uint8 len;
} coPdoMapItem;

/*
*  Structure for DS302 Bootup routine.
*/

typedef enum {  noErrorReported = 0,
                A, /* The slave no longer exists in the Network list */
                B, /* No response on access to Actual Device Type (object 1000h) received */
                C, /* Actual Device Type (object 1000h) of the slave node did
                    * not match with the expected DeviceTypeIdentification in object 1F84h */
                D, /* Actual Vendor ID (object 1018h) of the slave node did not match */
                   /* with the expected Vendor ID in object 1F85h */
                E, /* Slave node did not respond with its state during
                    * Check node state -process. Slave is a heartbeat producer */
                F, /* Slave node did not respond with its state during Check
                    * node state -process. Slave is a Node Guard slave (NMT slave) */
                G, /* It was requested to verify the application software version, but the
                    * expected version date and time values were not configured in
                    * objects 1F53h and 1F54h respectively */
                H, /* Actual application software version Date or Time (object 1F52h)
                    * did not match with the expected date and time values in objects
                    * 1F53h and 1F54h respectively. Automatic software update was not allowed */
                I, /* Actual application software version Date or Time (object 1027h) did
                    * not match with the expected date and time values in objects 1F53h
                    * and 1F54h respectively and automatic software update failed */
                J, /* Automatic configuration download failed */
                K, /* The slave node did not send its heartbeat message during Start
                    * Error Control Service although it was reported to be a heartbeat
                    * producer (Note! This error situation is illustrated in Figure 10
                    * in chapter 5.3) */
                L, /* Slave was initially operational. (CANopen manager may resume
                    * operation with other nodes) */
                M, /* Actual ProductCode (object 1018h) of the slave node did not
                    * match with the expected Product Code in object 1F86H */
                N, /* Actual RevisionNumber (object 1018h) of the slave node did
                    * not match with the expected RevisionNumber in object 1F87H */
                O } bootResultStates; /* Actual SerialNumber (object 1018h) of the slave node did not
                    * match with the expected SerialNumber in object 1F88H */

typedef enum {  stNotInUse = 0,
                stNodeBootInit,
                waitingForDeviceType,
                waitingForVendorId,   /* State where we can expect a response or not. */
                waitingForProductCode,
                waitingForRevNmber,
                waitingForSerialNmbr,
                waitingForSoftwareDate,
                waitingForSoftwareTime,
                waitingForConfigurationDate,
                waitingForConfigurationTime,
                waitingForHeatbeat,
                sendNMTstartNode,
                checkIfNodeIsMandatory,
                bootNextNodeState,
                isEnterOperationalStateOk,
                waitingForOneSecDelay,
                isOkStartAllNodes,
                stAnalyzeRes,
                bootIsFinished
              } bootStateMachine;


typedef struct {
  KfHalTimer        oneSecDelayTimeStamp;
  bootResultStates  nodeBootRes;
  uint32            slaveAssignment; /* DS302, p-16. */
  uint8             otherNodeId; /* Corresponding node id. */
  KfHalTimer        timeStampTx;
  bool              responseFailed;
  bootStateMachine  bootState; /* Boolean for keeping track if Device Type read has been requested. */
  /* Device Type (0x1000) */
  uint32            deviceTypeRes;
  uint32            deviceTypeExpected;
  bool              isDeviceTypeResErrCode;
  /* Vendor Id found on 0x1018 : 1 in remote */
  uint32            vendorIdRes;  /* If I can find the time I will implement read from these so that Sandvik knows what went wrong. */
  uint32            vendorIdExpected;
  bool              isVendorIdResErrCode;
  /* Product Code found on 0x1018 : 2 in remote */
  uint32            productCodeRes; /* If I can find the time... */
  uint32            productCodeExpected;
  bool              isProductCodeResErrCode;
  /* Revision number found on 0x1018 : 3 in remote */
  uint32            revisionNumberRes;  /* If I can find the time... */
  uint32            revisionNumberExpected;
  bool              isRevisionNumberResErrCode;
  /* Serial number found on 0x1018 : 4 in remote */
  uint32            serialNumberRes;  /* If I can find the time... */
  uint32            serialNumberExpected;
  bool              isSerialNumberResErrCode;
  /* Software version date found on 0x1f52 : ? in remote */
  uint32            softwareDateRes;  /* If I can find the time... */
  uint32            softwareDateExpected;  /* 0x1f53. */
  bool              isSoftwareDateResErrCode;
  /* Software version date found on 0x1f52 : ? in remote */
  uint32            softwareTimeRes;  /* If I can find the time... */
  uint32            softwareTimeExpected; /* 0x1f54. */
  bool              isSoftwareTimeResErrCode;
  /* Configuration date found on 0x1f52 : ? in remote */
  uint32            configurationDateRes;  /* If I can find the time... */
  uint32            configurationDateExpected; /* 0x1f26. */
  bool              isConfigurationDateResErrCode;
  /* Configuration time found on 0x1f52 : ? in remote */
  uint32            configurationTimeRes;
  uint32            configurationTimeExpected; /* 0x1f27. */
  bool              isConfigurationTimeResErrCode;
} coNMTNetworkList;

/* Flags for the  coNMTNetworkList -> mode variable. */


#define CO_PDOMAP_RX            0xfd /* pdoNo in objIdx */
#define CO_PDOMAP_TX            0xfe
#define CO_PDOMAP_END           0xff
#if DYNAMIC_PDO_MAPPING
  #define CO_PDOMAP_FREE_MEM    0xfd
#endif

#define CO_PDOMAP(m) (m->len <= 64) /* Given a pointer to a coPdoMapItem, returns true if it contains a map (and isn't a command) */
#define CO_PDO_MAP_NEW_TX(pdoNo)            {pdoNo, 0, CO_PDOMAP_TX}
#define CO_PDO_MAP_NEW_RX(pdoNo)            {pdoNo, 0, CO_PDOMAP_RX}
#define CO_PDO_MAP_OBJ(objIdx, subIdx, len) {objIdx, subIdx, len}
#define CO_PDO_MAP_END                      {0, 0, CO_PDOMAP_END}
#if DYNAMIC_PDO_MAPPING
  #define CO_PDO_MAP_FREE_MEM               {0, 0, CO_PDOMAP_FREE_MEM}
#endif

#define IS_POSITIVE(idx) (idx >= 0)  /* Idx is invalid if negative */

/* The operational states */
typedef enum {coOS_Initialising = 0,
              coOS_Disconnected = 1, /* Optional */
              coOS_Connecting = 2,   /* Optional */
              coOS_Preparing = 3,    /* Optional */
              coOS_Stopped = 4, /* Might also be called coOS_Prepared */
              coOS_Operational = 5,
              coOS_PreOperational = 127, /* State pre-operational is numberd 3 in canproto.pdf */
              coOS_Unknown = 0xff} coOpState; /* State unknown is only used internally */

/* NMT commands */
typedef enum {coNMT_StartRemoteNode = 0x01,  /* {coOS_PreOperational} -> coOS_Operational */
              coNMT_StopRemoteNode = 0x02,   /* {coOS_PreOperational, coOS_Prepared} -> coOS_Prepared */
              coNMT_EnterPreOperationalState = 0x80, /* {coOS_Operational, coOS_Prepared} -> coOS_PreOperational */
              coNMT_ResetNode = 0x81, /* {}  -> coOS_Reset */
              coNMT_ResetCommunication = 0x82} coNMTfunction;


/* Heartbeat messages are transmitted witha COB-ID derived from the nodeId. */
#define CO_HEARTBEAT_COBID_BASE 0x700  /* The COB-ID of a heart beat message (or the answer to a NMT Guarding Message) is CO_HEARTBEAT_COBID_BASE+nodeId */
#define CO_HEARTBEAT_COBID(n) (CO_HEARTBEAT_COBID_BASE+n)

/* PDO Transmission types */
#define CO_PDO_TT_SYNC_ACYCLIC      0
#define CO_PDO_TT_SYNC_CYCLIC_MIN   1 /* 1..240 is the number of SYNC objects between two PDO transmissions */
#define CO_PDO_TT_SYNC_CYCLIC_MAX 240
#define CO_PDO_TT_SYNC_RTR        252
#define CO_PDO_TT_ASYNC_RTR       253
#define CO_PDO_TT_ASYNC_MANUF     254
#define CO_PDO_TT_ASYNC_PROF      255
#define CO_PDO_TT_DEFAULT CO_PDO_TT_ASYNC_PROF /* Device Profile dependent */

/* Errorcodes for dynamic mapped PDOs */
#define DYNAMIC_MAP_ERROR 10

#define PDO_UNUSED    0 /* Pdo is un-used */
#define PDO_RX        1 /* RX pdo. Static mapped. */
#define PDO_TX        2 /* TX pdo. Static mapped. */
#define MPDO_TX       3 /* TX MPDO. */
#define MPDO_RX       4 /* RX MPDO. */
#define PDO_MAP_NONE ((uint16)-1)

/* Flags used by PDO */

/* Sync PDO realated flags: */

#define SYNC_COBID_GENERATE 0x40000000 /* Bit 30 of sync COBID says if node shall
                                        * generate COBID or not. */

#define SYNC_RX_PDO_EVENT   0x04 /* bit 3 */
#define SYNC_TX_PDO_EVENT   0x08 /* bit 4 */
#define SYNC_TX_PDO_RTR     0x10 /* bit 5 */


typedef struct {
  cobIdT COBID; /* cobPDOinvalid for unused records. */
  uint8 dir; /* PDO_UNUSED, PDO_RX or PDO_TX for a free, a receive or a transmit PDO (no other values are allowed). */
  uint8 transmissionType; /* See CO_PDO_TT_... */
  KfHalTimer inhibitTime; /* The minimum time between two consecutive PDO transmissions, unit 100 us; 0 if no min time */
  KfHalTimer eventTimer; /* PDO is transmitted periodically, unit 1 ms; 0 if no periodic transmissions */
  KfHalTimer lastMsgTime; /* Time of last transmission; For TPDO used together with inhibittTimer and eventTimer. Time of last Rx used for RPDO timeout monitoring */
  uint16 pdoNo; /* Index in Object Dictionary relative to CO_OBJDICT_PDO_RX/TX_COMM-1 (1..512) */
  uint16 mapIdx; /* Points into the pdoMap-vector, or PDO_MAP_NONE if no objects are mapped */
  uint8 dataLen; /* The total nmbr of bits in the PDO mapping. */
  uint8 status;  /* status flags for the PDO in the PDO[]-table */
                 /* bit 0 - inhibit-time active */
                 /* bit 1 - PDO in internal PDO queue */
                 /* bit 3 - PDO received, waiting for SYNC (SYNC_PDO_RECEIVED) */
                 /* bit 4 - PDO is disabled if set */
  uint8 cmBuf[8]; /* In case of temporarly storage of PDO */
  uint8 cmDlc;
  uint8 pdoSyncCntr; /* counts no. received Sync objects received for this PDO */
#if CO_DIRECT_PDO_MAPPING
  uint8 dmFlag; /* If non-zero, the following vectors should be used for the mapping. */
  uint8 dmNotifyCount;       /* ? */
  uint8 *dmDataPtr[8];       /* Worst case is 8 separate objects. */
  uint16 dmNotifyObjIdx[8];  /* Object Index */
  uint8 dmNotifySubIdx[8];   /* Sub Index */
  uint16 dmNotifyGroup[8];   /* notification group */
#endif /* CO_DIRECT_PDO_MAPPING */
} coPDO;
CompilerAssert((sizeof(coPDO) & 1) == 0);

/* PDO abort codes */

#define TPDO_INHIBIT_TIME_NOT_ELAPSED 1
#define TPDO_ERR_PDONO_NOT_IN_OD 2
#define TPDO_ERR_NOT_OPERATIONAL 3
#define TPDO_ERR_NO_MAP_FOUND 4
#define TPDO_ERR_APP_ABORT 5
#define TPDO_ERR_OBJECT_NOT_FOUND 6
#define TPDO_ERR_CAN 7

/* Status of SDOs */

#define SDO_UNUSED 0
#define SDO_SERVER 1
#define SDO_CLIENT 2

/* ======= LOGIC MASKS FOR CANopen MESSAGES ======= */

#define XCS_MASK                  0xE0 /* CCS/SCS */
#define TOGGLE_MASK               0x10 /* Toggle bit */
#define INIT_N_MASK               0x0C /* No. invalid bytes of init frame */
#define EXPEDITED_MASK            0x02 /* Expedited flag */
#define SIZE_MASK                 0x01 /* Size flag */
#define SEG_N_MASK                0x0E /* No. invalid bytes of segment frame */
#define SEG_N_MASK_SHIFT          0x01 /* no of shifts */
#define C_MASK                    0x01 /* c-flag */
#define INITIATE_N_MASK           0x0C /* Mask for initiate n */
#define INITIATE_N_MASK_SHIFT     0x02 /* no of shifts */

/* Statusflags of status entry in the SDO[] table */
#define SDO_STATUS_AVAILABLE                   0
#define SDO_STATUS_WAIT_RESPONSE_EXPEDITED_DL  1
#define SDO_STATUS_WAIT_RESPONSE_SEGMENTED_DL  2
#define SDO_STATUS_WAIT_RESPONSE_EXPEDITED_UL  3
#define SDO_STATUS_WAIT_RESPONSE_SEGMENTED_UL  4
#define SDO_STATUS_DOES_NOT_EXIST              5


/* ======= STATUS FLAGS ======= */

/* SDO configured as CLIENT */

#define CLI_EXPEDITED_DL_REQUESTED 0x0001 /* Only expect confirm frame - done! */
#define CLI_SEG_DL_REQUESTED       0x0002 /* Expect confirm frame - allowance to start transfer */
#define CLI_SEG_DL_INITIATED       0x0004 /* Confirm frame received - transfer started */

#define CLI_EXPEDITED_UL_REQUESTED 0x0008 /* Only expect confirm frame - done! */
#define CLI_INITIATE_UL_REQUESTED  0x0020 /* Client asks for upload to itself */
#define CLI_SEG_UL_INITIATED       0x0040 /* Expedited transfer was not enough, segmented */
                                        /* transfer has been initiated */
#define CLI_SEG_DL_DONE_INDICATED  0x0080 /* Set when all we expect after last SEG_DL_REQUEST */
                                        /* is a acknowledge. */

/* SDO configured as SERVER */

#define SRV_SEG_DL_INITIATED       0x0001 /* Confim DL-req frame sent - transfer started */
#define SRV_SEG_UL_INITIATED       0x0002 /* Confim UL-req frame sent - transfer started */

/* CLIENT / SERVER */

#define SEG_TRANSFER_TOGGLE        0x0010 /* For toggle control, both UL and DL */
#define SEG_BUFFER_INITIATED       0x0100 /* Flag to indicate buffer initiated */

/* ======= CCS / SCS ======= */

/* CCS (Client Command Specifiers) */

#define CLI_REQUEST_INIT_TRANSF_FROM_CLI_TO_SRV   0x01 /* Initiate SDO download request */
#define CLI_REQUEST_INIT_TRANSF_FROM_SRV_TO_CLI   0x02 /* Initiate SDO upload   request */
#define CLI_REQUEST_SEGM_FROM_CLI_TO_SRV          0x00 /* Download SDO segment protocol request */
#define CLI_REQUEST_SEGM_FROM_SRV_TO_CLI          0x03 /* Upload   SDO segment protocol request */

/* SCS (Server Commmand Specifiers) */

#define SRV_RESPONSE_INIT_TRANSF_FROM_CLI_TO_SRV  0x03 /* Initiate SDO download respons */
#define SRV_RESPONSE_INIT_TRANSF_FROM_SRV_TO_CLI  0x02 /* Initiate SDO download respons */
#define SRV_RESPONSE_SEGM_FROM_CLI_TO_SRV         0x01 /* Download SDO segment protocol respons */
#define SRV_RESPONSE_SEGM_FROM_SRV_TO_CLI         0x00 /* Upload   SDO segment protocol respons */

/* Abortcode */

#define XCS_ABORTCODE              0x04 /* Abort code */

#define XSC_SHIFT                  0x05 /* No of shifts for CCS/SCS */
#define SIZE_SHIFT                 0x02 /* No of shifts for SIZE */

typedef struct { /* We are the server */
  cobIdT COBID_Rx; /* COBID_ClientToServer if we are the server */
  cobIdT COBID_Tx; /* COBID_ServerToClient if we are the server */
  uint16 objIdx; /* ObjIdx initaited seg/normal transfer */
  uint8 subIdx; /* SubIdx initiated seg/normal transfer */
  uint8 sdoNo; /* Index in Object Dictionary relative to CO_OBJDICT_SDO_RX/TX_COMM-1 (1..128) */
  KfHalTimer timer; /* Timer to handle SDO transfer protocol timeout */
  uint8 otherNodeId;
  uint8 type; /* SDO_UNUSED, SDO_SERVER or SDO_CLIENT (and nothing else) */
  uint16 status;  /* Statusflags for current SDO */
  CoObjLength objToTransLen; /* size of file that should be transmitted */
  uint8 localBuf[SIZE_LOCAL_SDO_BUF];  /* To be able to store segmented transfers CO_OD! */
  CoObjLength pos; /*Size of total sent data of this file.*/
  uint8 bufPos;
  uint8 validBufLen;
} coSDO;
CompilerAssert((sizeof(coSDO) & 1) == 0);

/* --- NMT Master NodeGuarding implementation. */
typedef struct {
  uint8 nodeId;
  bool alive; /* Set to true when the node is heard from, cleared when heartbeats times out. */
  uint8 toggle; /* The value of toggle in the last heartbeat message */
  coOpState state; /* Valid after valid is set to true */
  uint16 guardTimeMs; /* Guard time in ms */
  KfHalTimer guardTimeTicks; /* Guard time in ticks */
  uint8 lifeTimeFactor;
  KfHalTimer lastNodeGuardRecvTime; /* Last time a heartbeat message was received */
  KfHalTimer lastNodeGuardSendTime; /* Last time a guard request msg was transmitted to this node. */
  bool       isNodeguardingActive;
  uint8 padding;
} NodeguardingNodeInfo;
CompilerAssert((sizeof(NodeguardingNodeInfo) & 1) == 0);

/* --- Heartbeat Consumer implementation. */
typedef struct {
  uint8 nodeId;
  bool alive; /* Set to true when the node is heard from, cleared when heartbeats times out. */
  coOpState state; /* Valid after valid is set to true */
  uint16 heartBeatPeriod; /* In ms */
  KfHalTimer heartBeatTimeout; /* heartBeatPeriod in ticks */
  KfHalTimer lastHeartbeatTime; /* Last time a heartbeat message was received */
  bool       isHeartBeatControlRunning;
  uint8 padding;
} HeartbeatNodeInfo;
CompilerAssert((sizeof(HeartbeatNodeInfo) & 1) == 0);

/* Emergency (EMCY) implementation */
typedef struct {
  cobIdT COBID;              /* COBID for node. */
  uint8 nodeId;              /* NodeId. If zero, no entry */
  uint8 padding;
} EmcyNode;
CompilerAssert((sizeof(EmcyNode) & 1) == 0);

/* 
*   Implementation of EMCY producer history list, p.9-65, DS301. 
*/
typedef struct {
  uint16  additionalInfo;
  uint16  errorCode;
  uint8   manufSpecField[5];
} coEmcyProdHistEntr;


/* Object dictionary vector.  The Object Dictionary can be accessed wither via
* function calls or by table lookup.
*/

typedef struct {
  CoObjLength defVal;
  uint8 *defValStrOnly;
  CoObjLength size; /* Size in bytes */
  uint8 attr; /* Bitmask of CO_OD_-values */
  /* Objects in a PDO with identical non-zero value will get only one read or
  * write notification. */
  uint16 notificationGroup;
} coObjDictVarDesc;
CompilerAssert((sizeof(coObjDictVarDesc) & 1) == 0);

typedef struct {
  uint16 objIdx;      /* ObjIdx */
  uint16 subIdxCount; /* 0 for simple variables, 16 bit to allign to even address */
  void *objPtr;       /* Points to storage. */
  const coObjDictVarDesc *objDesc;
  uint16 structure; /* Sub-index 0xff.*/
  uint16 dsp_dummy;
} coObjDictEntry;
CompilerAssert((sizeof(coObjDictEntry) & 1) == 0);


/* Bit values of coObjDictVarDesc.attr
 * For OD entries that are defined in the table, reading and writing of values
 * are usually transparent to the canopen stack. By setting the
 * CO_OD_READ_NOTIFY, CO_OD_WRITE_VERIFY or CO_OD_WRITE_NOTIFY bits, it is
 * possible to be notified after a read or update is made of a value should
 * further action or verification have to be done. */
#define CO_OD_ATTR_READ        1 /* Read access */
#define CO_OD_ATTR_WRITE       2 /* Write access */
#define CO_OD_ATTR_VECTOR      4 /* All subindexes (except 0 and 255) are identical (there is only one coObjDictVarDesc) */
#define CO_OD_ATTR_PDO_MAPABLE 8 /* Can be mapped into PDO */
#define CO_OD_READ_NOTIFY     16 /* Call odReadNotify() before read access */
#define CO_OD_WRITE_VERIFY    32 /* Call odWriteVerify() before write access */
#define CO_OD_WRITE_NOTIFY    64 /* Call odWriteNotify() after write access */
#define CO_OD_ATTR_STRING    128 /* The contents of this Object Dictionary entry is string and is handled like a stream of data qqq */


/* Bits in the Error Register */
#define CO_ERREG_GENERIC_ERROR 0x01
#define CO_ERREG_CURRENT       0x02
#define CO_ERREG_VOLTAGE       0x04
#define CO_ERREG_TEMPERATURE   0x08
#define CO_ERREG_COMM          0x10
#define CO_ERREG_DEV_PROF_SPEC 0x20
#define CO_ERREG_MANUF_SPEC    0x80

// Emergency error codes and masks
#define CO_EMCY_NO_ERRORS                                   0x0000
#define CO_EMCY_ERROR_RESET                                 0x0000
#define CO_EMCY_GENERIC_ERRORS                              0x1000
#define CO_EMCY_CURRENT_ERRORS                              0x2000
#define CO_EMCY_CURRENT_DEVICE_INPUT_ERRORS                 0x2100
#define CO_EMCY_CURRENT_INSIDE_DEVICE_ERRORS                0x2200
#define CO_EMCY_CURRENT_DEVICE_OUTPUT_ERRORS                0x2300
#define CO_EMCY_VOLTAGE_ERRORS                              0x3000
#define CO_EMCY_VOLTAGE_MAINS_ERRORS                        0x3100
#define CO_EMCY_VOLTAGE_INSIDE_DEVICE_ERRORS                0x3200
#define CO_EMCY_VOLTAGE_OUTPUT_ERRORS                       0x3300
#define CO_EMCY_TEMPERATURE_ERRORS                          0x4000
#define CO_EMCY_TEMPERATURE_AMBIENT_ERRORS                  0x4100
#define CO_EMCY_TEMPERATURE_DEVICE_ERRORS                   0x4200
#define CO_EMCY_HARDWARE_ERRORS                             0x5000
#define CO_EMCY_SOFTWARE_ERRORS                             0x6000
#define CO_EMCY_SOFTWARE_INTERNAL_ERRORS                    0x6100
#define CO_EMCY_SOFTWARE_USER_ERRORS                        0x6200
#define CO_EMCY_SOFTWARE_DATA_SET_ERRORS                    0x6300
#define CO_EMCY_ADDITIONAL_MODULES_ERRORS                   0x7000
#define CO_EMCY_MONITORING_ERRORS                           0x8000
#define CO_EMCY_MONITORING_COMMUNICATION_ERRORS             0x8100
#define CO_EMCY_MONITORING_COM_CAN_OVERRUN_ERROR            0x8110
#define CO_EMCY_MONITORING_COM_CAN_PASSIVE_ERROR            0x8120
#define CO_EMCY_MONITORING_COM_LIFE_GUARD_HB_ERROR             0x8130
#define CO_EMCY_MONITORING_COM_BUS_RECOVERY_ERROR           0x8140
#define CO_EMCY_MONITORING_COM_COBID_COLLISION_ERROR        0x8150
#define CO_EMCY_MONITORING_PROTOCOL_ERRORS                  0x8200
#define CO_EMCY_MONITORING_PROTOCOL_LENGTH_ERROR            0x8210
#define CO_EMCY_MONITORING_PROTOCOL_LENGTH_EXCEEDED_ERROR   0x8220
#define CO_EMCY_EXTERNAL_ERRORS                             0x9000
#define CO_EMCY_ADDITIONAL_FUNCTION_ERRORS                  0xF000
#define CO_EMCY_DEVICE_SPECIFIC_ERRORS                      0xFF00

/* SDO abort codes */
typedef uint32 coErrorCode;
#define CO_ERR_TOGGLE_BIT_NOT_ALTERED                    0x05030000 /* Toggle bit not altered */
#define CO_ERR_SDO_PROTOCOL_TIMED_OUT                    0x05040000 /* SDO protocol timed out */
#define CO_ERR_UNKNOWN_CMD                               0x05040001 /* Client/server command specifier not valid or unknown */
#define CO_ERR_UNSUPPORTED_ACCESS_TO_OBJECT              0x06010000 /* Unsupported access to an object */
#define CO_ERR_OBJECT_READ_ONLY                          0x06010002 /* Attempt to write a read-only object */
#define CO_ERR_OBJECT_WRITE_ONLY                         0x06010001 /* Attempt to read a write-only object */
#define CO_ERR_OBJECT_DOES_NOT_EXISTS                    0x06020000 /* Object does not exists in the Object Dictionary */
#define CO_ERR_OBJECT_LENGTH                             0x06070010 /* Data type does not match, length of service parameter does not match */
#define CO_ERR_SERVICE_PARAM_TOO_HIGH                    0x06070012 /* Data type does not match, length of service parameter too high */
#define CO_ERR_SERVICE_PARAM_TOO_LOW                     0x06070013 /* Data type does not match, length of service parameter too low */
#define CO_ERR_INVALID_SUBINDEX                          0x06090011 /* Sub-index does not exist */
#define CO_ERR_OBJDICT_DYN_GEN_FAILED                    0x08000023 /* Object dictionary dynamic generation fails or no object dictionary is present */
#define CO_ERR_GENERAL                                   0x08000000 /* General error */
#define CO_ERR_GENERAL_PARAMETER_INCOMPATIBILITY         0x06040043 /* General paramter internal incompabilitity */
#define CO_ERR_GENERAL_INTERNAL_INCOMPABILITY			 0x06040047	/* General internal incompatibility in the device */	// Added by PN
#define CO_ERR_NOT_MAPABLE                               0x06040041 /* Try to map un-mapable object to PDO. */
#define CO_ERR_DATA_CAN_NOT_BE_STORED                    0x08000020 /* the requested data can not be stored */
#define CO_ERR_OUT_OF_MEMORY                             0x05040005 /* Node is out of memory. */
#define CO_ERR_PDO_LENGHT                                0x06040042 /* Leght of PDO is exceeded  */
#define CO_ERR_VAL_RANGE_FOR_PARAM_EXCEEDED              0x06090030 /* Param out of range. */ 
#define CO_ERR_VALUE_TOO_HIGH              				 0x06090031 /* Value of parameter written too high (download only). */ 
#define CO_ERR_VALUE_TOO_LOW                             0x06090032 /* Value of parameter written too low (download only). */ 
#define CO_ERR_ABORT_SDO_TRANSFER                        0x06060000 /* Abort SDO transfer */

#define CO_OK                                            0x00000000

typedef uint8 coStatusSDO;
#define SDO_READY                                0
#define SDO_WAIT_RESPONSE_EXPEDITED_DL           1
#define SDO_WAIT_RESPONSE_SEGMENTED_DL           2
#define SDO_WAIT_RESPONSE_EXPEDITED_UL           3
#define SDO_WAIT_RESPONSE_SEGMENTED_UL           4
#define SDO_WAIT_REQUEST_SEGMENTED_DL            6
#define SDO_WAIT_REQUEST_SEGMENTED_UL            7
#define SDO_STATUS_NOT_AVAILABLE                 8
#define SDO_NOT_AVAILABLE                        9

/* Local stack SDO Segmented Transfer errorcodes */

#define CO_ERR_CLIENT_SDO_NOT_FOUND    0x00000001 /* requested SDO not found */

typedef enum {
  CO_SEG_CONTINUE = 0,
  CO_SEG_DONE,
  CO_SEG_UNKNOWN,
  CO_SEG_INIT_DATA
} CoSegTranAction;

/* Return codes from e.g. appNotifyPDORequest() */
typedef enum {
  coPA_Continue = 0,
  coPA_Abort,
  coPA_Done
} CoPDOaction;

typedef enum {
  coRes_OK = 0,
  /* Can hardware errors */
  coRes_CAN_ERR,                                   /* Undefined CAN error. */
  /* CANHAL errors, more exact */
  coRes_ERR_CAN_PARAM,                             /* Error in CAN HW paramters (out of bounds?) */
  coRes_ERR_CAN_NOMSG,                             /* No CAN message to read. */
  coRes_ERR_CAN_NOTFOUND,                          /* CAN hw not found. */
  coRes_ERR_CAN_TIMEOUT,                           /* CAN timeout. */
  coRes_ERR_CAN_NOTINITIALIZED,                    /* CAN hw not initialized. */
  coRes_ERR_CAN_OVERFLOW,                          /* Receive overflow. */
  coRes_ERR_CAN_HARDWARE,                          /* CAN HW error. */
  coRes_ERR_CAN_OTHER,                             /* Undefined error from device driver. */
  coRes_ERR_CAN_SHUT_DOWN,                         /* Could not properly shut down the CAN comm. */
  coRes_ERR_CAN_GO_ON_BUS,
  /* Callback */
  coRes_ERR_CAN_CALLBACK_SETUP,                    /* Could not set up the callback function */
  /* Reset communication */
  coRes_ERR_RESET_COMM,                            /* Stack could not RESET/reinitlaize due to communcation. */
  coRes_ERR_RESET_NODE_ID,                         /* Stack could not RESET/reinitlaize due to NODE ID out of range. */
  coRes_ERR_DEFAULT_PDO_MAP_SIZE,                  /* Stack could not RESET/reinitlaize due to the size of the default PDO mapping size. */
  coRes_ERR_DEFAULT_PDO_MAP_ARGUMENTS,             /* Stack could not RESET/reinitlaize due to argument of specific PDO (not mapable, readable, writeable?) */
  coRes_ERR_DEFAULT_PDO_MAP_PDO_SIZE,              /* Stack could not RESET/reinitlaize due to mappingsize of a specific PDO (it has exceeded 64bit). */
  /* transmit heartbeat err */
  coRes_ERR_GEN_HEARTBEAT,                         /* Heartbeat could not be generated due to state error of node. */
  /* transmit sync err */
  coRes_ERR_GEN_SYNC,                              /* Sync message could not be generated due to state error of node. */
  /* PDO errors */
  coRes_ERR_TPDO_NOT_OPERATIONAL,                  /* PDO could not be transmitted due to error in state (not in operational state). */
  coRes_ERR_TPDO_NO_MAP_FOUND,                     /* Mapping of specific PDO not found. */
  coRes_ERR_TPDO_APP_ABORT,                        /* PDO could be sent (aborted by application). */
  coRes_ERR_TPDO_OBJECT_NOT_FOUND,                 /* Mapped object in PDO not found. */
  coRes_ERR_TPDO_INHIBIT_TIME_NOT_ELAPSED,         /* PDO could not be sent, inhibit time has not elaped. */
  coRes_ERR_TPDO_NOT_FOUND,                        /* Transmit PDO does not exist. */
  /* SDO errors */
  coRes_ERR_UNDEFINED_SDO_NO,                      /* Specified SDO number does not exist.  */
  coRes_ERR_SDO_BUSY,                              /* Specified SDO is occupied with transfer, wait and try again. */
  /* comSetEmcyControl/comSetEmcyControlCreate */
  coRes_ERR_EMCY_CNTRL_NODE_NOT_FOUND,             /* Specified ID was not in the Emergency control list. */
  coRes_ERR_EMCY_CNTRL_ID,                         /* ID out of range. */
  coRes_ERR_EMCY_CNTRL_LIST_FULL,                  /* The list is full, can not guard an other node. */
  /* coSetNodeGuard */
  coRes_ERR_CFG_NODEGUARD,                         /* Error to configure/reconfigure the nodeguard. */
  coRes_ERR_CFG_NODEGUARD_ID_NOT_FOUND,            /* The specified ID not found. */
  coRes_ERR_CFG_NODEGUARD_LIST_FULL,               /* Can not add an other node to the list, it's full. */
  /* coProcessMessage */
  coRes_ERR_DLC_REQUEST_PDO,                       /* The dlc was wrong in the PDO request frame */
  coRes_ERR_TX_PDO_RECEIVED_IS_RX,                 /* A PDO, configured as TX has been received! Design error in the network. */
  coRes_OK_UNPROCESSED,                            /* A can message has been received but not processed by the stack. */
  /* comWriteObject */
  coRes_ERR_PROTOCOL,                              /* Errorcode sent by remote node */
  coRes_ERR_PROTOCOL_TIMEOUT,
  coRes_ERR_DLC_OUT_OF_RANGE,                      /* DLC parameter out of range. */
  coRes_UNKNOW_FRAME,                               /* An unindentifable frames has been received. */
  /* coSetNodeId */
  coRes_ERR_NODE_ID_OUT_OF_RANGE,
  /* coSetState */
  coRes_ERR_INVALID_STATE,
  /* Dynamic mapping */
  coRes_ERR_DYNAMIC_MAP_RESET,                     /* If the requested nodestate could not be set */
  coRes_ERR_OBJ_OUT_OF_RANGE,                      /* stack tried to map obejcts outside the specific range. */
  coRes_ERR_NO_MAP_AVAILABLE,                      /* No PDO-map could be found */
  coRes_OUT_OF_MEMORY,                             /* Out of memory */
  coRes_ERR_CREATE_PDO,
  coRes_ERR_OBJ_NOT_MAPABLE,
  coRes_ERR_SUB_IDX_OUT_OF_RANGE,
  coRes_ERR_CANOPEN,
  coRes_ERR_PDO_DISABLED,
  /* connectClientSdoToNodeId */
  coRes_ERR_CONFIGURE_CLIENT_SDO,
  /* resetNodeByNode */
  coRes_ERR_NODE_BY_NODE_RST_GENERAL,
  /* bootSlavesNodeByNode */
  coRes_ERR_CFG_CLIENT_SDO,
  coRes_ERR_NODEID_NOT_FOUND_IN_BOOTLIST,
  /* bootNextNode */
  coRes_ERR_BOOT_NEXT_NODE,
  coRes_ERR_NO_NODES_FOUND,
  /* coMasterBootDS302 */
  coRes_ERR_NO_NODES_TO_BOOT,
  /* analyzeResponseDS302 */
  coRes_REBOOT_FAILING_NODES,
  /* coConfigureIdentityObject */
  coRes_ERR_CONFIGURE_VENDOR_ID,
  coRes_ERR_CONFIGURE_PRODUCT_CODE,
  coRes_ERR_CONFIGURE_REVISION_NUMBER,
  coRes_ERR_CONFIGURE_SERIAL_NUMBER,
  /* readObjDispListParamsFromBuf */
  coRes_ERR_BUFFER_LEN_INVALID,
  /* extractDataFromMPDO */
  coRes_ERR_NO_VALID_BUF_DATA,
  coRes_ERR_MESSAGE_DLC,
  /* handleMPDO */
  coRes_ERR_INVALID_NODE_ID_SAM_PDO,
  coRes_ERR_HANDLE_MPDO,
  /* coTransmitDAMPDOEx & coTransmitDAMPDO */
  coRes_ERR_TPDO_IS_NOT_DAM_PDO,
  coRes_ERR_NO_MAPPED_OBJ_DAM_PDO,
  /* packMPDOToBuf */
  coRes_ERR_PACK_MPDO_BUF_LEN,
  coRes_ERR_MPDO_TYPE_UNKNOWN,
  /* transmitSAMPDO */
  coRes_ERR_NO_SAM_TPDO_FOUND,
  /* packDispListVarsToBuf */
  coRes_ERR_PACK_DISPLIST_TO_BUF_LEN

} CoStackResult;


/* --- Function prototypes */
#ifdef __cplusplus
extern "C" {
#endif

#if CO_MULTIPLE_INSTANCES
  typedef void* CoHandle;
  #define CH CoHandle ch,
  #define CH_S CoHandle ch
#else
  #define CH
  #define CH_S void // For functions with no additional parameters
#endif

/*
*   Function calls added by PLENWARE.
*/
coErrorCode coGetBitCount(CH unsigned long* bits);
coErrorCode coGetErrorCount(CH uint16 *txErr, uint16 *rxErr, uint16 *ovErr);
void coPoll(void);

/* Implementation of multiple instances of the stack */

void coInitLibrary(void);


#if CO_MULTIPLE_INSTANCES
  CoHandle
#else
  CoStackResult
#endif
        coInit(CanChannel chan, CanBitRate btr, uint8 nodeId, const coObjDictEntry *ODappI, const coPdoMapItem *pdoMapI);

/*CoStackResult coInit(CanChannel chan, CanBitRate btr, const coObjDictEntry *ODappI, const coPdoMapItem *pdoMapI);*/
CoStackResult coExit(CH_S);
void coMain(int seconds);
coErrorCode coWriteObjectP(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len);
CoStackResult coProcessMessage(CH CanId cmId, uint8 *cmBuf, uint8 cmDlc);
CoStackResult coTransmitPDO(CH uint16 pdoNo);
CoStackResult coRequestPDO(CH uint16 pdoNo);
uint32 coODReadSetError( coErrorCode err, coErrorCode *perr);  /* Same for all stacks. */
uint8 coObjLen( uint8 subIdx, uint8 arrLen, uint8 elemSize);   /* Same for all stacks. */
CoObjLength coGetObjectLen(CH uint16 objIdx, uint8 subIdx, coErrorCode *err);
uint8 coGetObjectAttr(CH uint16 objIdx, uint8 subIdx);
uint32 coReadObject(CH uint16 objIdx, uint8 subIdx, coErrorCode *err);
coErrorCode coReadObjectP(CH uint16 objIdx, uint8 subIdx, uint8 *buf, uint8 len);
coErrorCode coReadObjectEx(CH uint16 objIdx, uint8 subIdx, uint8 *buf, CoObjLength pos, uint8 *(valid), uint8 len);
CanHalCanStatus coWriteMessage(CH cobIdT id, const uint8 *msg, uint8 dlc, uint32 tag);

coErrorCode coWriteObject(CH uint16 objIdx, uint8 subIdx, uint32 val);
void coPeriodicService(CH_S);
coOpState coGetNodeState(CH uint8 mNodeId );
CoStackResult coSetState(CH coOpState opState);  /* qqq dokument */
coOpState coGetState(CH_S);
void coEmergencyError(CH uint16 errCode);
void coEmergencyErrorEx(CH uint16 errCode, uint16 additionalInfo, uint8 b1, uint8 b2, uint8 b3, uint8 b4, uint8 b5);
void coEMCYclearError(CH uint8 err);
CoStackResult getCanHalError(CanHalCanStatus stat);

// Codewarrior throws a warning if we #if when not even defined
#ifdef MODULE_TEST
#if MODULE_TEST
unsigned char TEST_coCanCallback(CanCbType cbType, const CANMessage *msg);
#endif
#endif


#if IDENTITY_OBJECT
//CoStackResult coConfigureIdentityObject(CH uint32 vendorId, uint32 prodCode, uint32 revNum, uint32 serialNum);
#endif


#if BOOTUP_DS302
CoStackResult coMasterBootDS302(CH_S);
#endif

#if NODEGUARD_MASTER
CoStackResult coSetNodeGuard(CH uint8 nId, uint16 heartBeatPeriod, uint8 lifeTimeFactor);
#endif

#if MULTIPLEXED_PDO
void appIndicateFaltyWriteReceivedDAMPDO(CH uint16 objIdx, uint8 subIdx, uint32 dataField);
#endif

CoStackResult coResetNode(CH_S);
CoStackResult coResetCommunication(CH_S);
uint8 coGetNodeIdSDO(CH uint8 sdoNo, uint8 type);
uint8 coGetSdoNo(CH uint8 otherNodeId, uint8 type);
coStatusSDO coReadStatusSDO(CH uint8 sdoNo, uint8 sdoType );
uint8 coGetNodeId(CH_S);
CoStackResult coSetNodeId(CH uint8 nodeId);

void coVal2Buf(uint32 val, uint8 *buf, uint8 len);
uint32 coBuf2Val(uint8 *buf, uint8 len);


/* CLIENT functions. */

/* -> Stack (available in the stack) */
#if CLIENT_SDO_SUPPORT
CoStackResult coReadObjectSDO(CH uint8 sdoNo, uint16 objIdx, uint8 subIdx);
CoStackResult coWriteObjectSDO(CH uint8 sdoNo, uint16 objIdx, uint8 subIdx, CoObjLength objLen); 
CoStackResult coWriteObjectExpSDO(CH uint8 sdoNo, uint16 objIdx, uint8 subIdx, uint32 val, uint8 objLen);
CoStackResult coAbortTransferSDO(CH uint8 sdoNo, uint16 objIdx, uint8 subIdx, coErrorCode err);
#endif

/* <- Stack (callbacks, must be implemented by the application) */
coErrorCode appReadObjectResponseSDO(CH uint8 sdoNo, uint16 objIdx, uint8 subIdx, uint8 *bufPtr, CoObjLength pos, uint8 valid, CoSegTranAction flag );
coErrorCode appWriteObjectResponseSDO(CH uint8 sdoNo,uint16 objIdx, uint8 subIdx, uint8 *bufPtr, CoObjLength pos, uint8 *(valid), uint8 sizeOfBuffer);

/* Callbacks from the stack! (May be used for server stuff) */
coErrorCode appWriteObject(CH uint16 objIdx, uint8 subIdx, uint8 *bufPtr, CoObjLength pos, uint8 valid, CoSegTranAction flag );
coErrorCode appReadObject(CH uint16 objIdx, uint8 subIdx, uint8 *bufPtr, CoObjLength pos, uint8 *(valid), uint8 sizeOfBuffer );
uint8 appGetObjectAttr(CH uint16 objIdx, uint8 subIdx);
CoObjLength appGetObjectLen(CH uint16 objIdx, uint8 subIdx, coErrorCode *err);
coErrorCode appODReadNotify(CH uint16 objIdx, uint8 subIdx, uint16 notificationGroup);
coErrorCode appODWriteNotify(CH uint16 objIdx, uint8 subIdx, uint16 notificationGroup);
coErrorCode appODWriteVerify(CH uint16 objIdx, uint8 subIdx, uint32 val);

uint8 appGetBufSizeSDO(CH uint8 sdoNo );
void appTransferErrSDO(CH uint8 sdoNo, uint16 objIdx, uint8 subIdx, coErrorCode errCode );

/* --- Prototypes for functions the application must implement in a file which
 *     includes this file (canopen.h) */

/* --- PDO stuff --- */
CoPDOaction appIndicatePDOReception(CH uint16 pdoNo, uint8 *buf, uint8 dlc);

/*CoPDOaction appIndicatePDORequest(CH uint16 pdoNo, uint8 *buf, uint8 dlc);*/
CoPDOaction appIndicatePDORequest(CH uint16 pdoNo);
void appNotifyPDOReception(CH uint16 pdoNo);

/* ---  Network Management callbacks/Stack help --- */

/*void appIndicateHeartbeat(CH uint8 mNodeId, uint8 mToggle, coOpState mState);*/
void appIndicateNodeGuardStateChange(CH uint8 nodeId, coOpState state); /* by Kalle 2005-02-11: */
void appIndicateNodeGuardFailure(CH uint8 nodeId); 
void appError(CH uint16 errType, uint8 pos, uint16 param1, uint8 param2, uint32 param3);
#if LIFE_GUARDING
void appIndicateLifeGuardingEvent(CH uint8 state);
#define LG_STATE_OCCURRED 1
#define LG_STATE_RESOLVED 2
#endif

#if SYNC_WINDOW_LENGTH
void appIndicateSyncWindowLengthExpire(CH uint16 pdoNo);
#endif

#if (USE_RPDO_TIMEOUT_MONITORING)
void appIndicateRpdoTimeout(CH uint16 pdoNo);
#endif

#if BOOTUP_DS302
void appNotifyBootError(CH uint8 nodeId, bootResultStates bootErrCode);
bool appBootFinishedIsOkEnterOperational(CH_S);  /* We must ask application if OK enter operational. */
void appBootFinishedSuccessfully(CH_S);
bool appIsNonMandatoryBootErrOk(CH uint8 nodeId, bootResultStates bootErr, uint32 nodeResponse);
#endif

#if LSS_SLAVE
bool appLoadLssParams(CH uint16 *size, uint8 **bufP);
bool appSaveLssParams(CH uint16 size, uint8 **bufP);
void appIndicateLssState(CH uint8 state); /* 0 - LSS off, Other value - LSS active */
#endif

#if STORE_PARAMS
/* area is CO_STORE_COMM_PARAMS, CO_STORE_APPL_PARAMS or CO_STORE_MANF_PARAMS_MIN */
bool appSaveParameters(CH uint8 area, const uint8 *buf, CoObjLength len);
bool appLoadParameters(CH uint8 area, uint8 *buf, CoObjLength len);
bool appSaveConfDateTime(CH uint32 confDate, uint32 confTime);
bool appLoadConfDateTime(CH uint32 *confDate, uint32 *confTime);
#endif

#if IDENTITY_OBJECT
bool appGetIdentityObject(CH uint32 *vendorId, uint32 *prodCode, uint32 *revNum, uint32 *serialNum);
#endif

#if OD_MFG_STRINGS
uint8 appGetMfgDeviceNameLen(CH_S);
bool appGetMfgDeviceName(CH uint8 *mfgDeviceName, uint8 len);
uint8 appGetMfgHwVersionLen(CH_S);
bool appGetMfgHwVersion(CH uint8 *mfgHwVersion, uint8 len);
uint8 appGetMfgSwVersionLen(CH_S);
bool appGetMfgSwVersion(CH uint8 *mfgSwVersion, uint8 len);
#endif


/* Server->Application */

/* Emergency Implementation */
void resetEmcyCntrlList(CH_S);
void handleReceivedEmcy(CH uint8 nodeId, uint8* dataPtr);
CoStackResult coSetEmcyControl(CH uint8 nId, cobIdT COBID);
CoStackResult coSetEmcyControlCreate(CH uint8 nId, cobIdT COBID);
  /* nodeId : Current node´s node Id. */
  /* mkUSHORT(buf[0], buf[1]): Emergency Error Code. */
  /* buf[3]: Error register. */
  /* buf[4]->buf[7]: Manufacurer specific error field. */

  /* the only thing to do is to call the application, what should CANopen do? */
void appEmcyError(CH uint8 nodeId, uint16 emcyErrCode, uint8 errReg, uint8* manfSpecErrField);
/* Secret link between canopen.c and canopenM.c */

uint8 coGetChannel(CH_S);

/* local PDO err */
#define LOC_CO_ERR_NODID        1
#define LOC_CO_ERR_PDO          2
#define LOC_CO_ERR_PDO_ATTR     3
#define LOC_CO_ERR_DYNAMIC_PDO  4
#define LOC_CO_ERR_SUPPORT      5
#define LOC_CO_ERR_CANHAL       6


/* local SDO err */

//#define LOC_CO_ERR_SUPPORT    4
#define SYNC_PRODUCER_FUNCTIONALITY 1  /* 0-node can not be a sync-producer, 1-node can be a sync-producer */



#if MULTIPLEXED_PDO

#define SCAN_LST_BLK_SIZE_BM        0xff000000
#define SCAN_LST_OBJ_IDX_BM         0x00ffff00
#define SCAN_LST_SUB_IDX_MB         0x000000ff


/*
*   This function is for reding the data from a received
*   multiplexed PDO.
*/

/* Application callbacks upon reception of a MPDO but the write could not be done to local OD */
void appIndicateFaltyWriteReceivedMPDO(CH uint16 objIdx, uint8 subIdx, uint32 val);

void appIndicateFaltyWriteRecSAMMPDO(CH uint16 objIdx, uint8 subIdx, uint32 val, coErrorCode err);


/*
*   This API call follows the spec meaning that the data field of this DAM MPDO is the
*   contents of the object mapped on the first mappostion.
*/
CoStackResult coTransmitDAMPDO(CH uint16 pdoNo, uint8 nodeId, uint16 objIdx, uint8 subIdx);

/*
*   This function overrides the spec (see coTransmitDAMPDO(..)) and you write the value 
*   yourself in the API call directly.
*/
CoStackResult coTransmitDAMPDOEx(CH uint16 pdoNo, uint8 nodeId, uint16 objIdx, uint8 subIdx, uint32 val);


#define DESTINATION_ADDRESSING_MODE_FLG       0x80
#define MULTIPLEXED_PDO_NODE_ID_BM            0x7f

typedef struct {
  uint8 nodeId;
  uint16 objIdx;
  uint8 subIdx;
  uint32 data;
} coSamPdoFrameDataT;


/*
*   The different Multiplexed PDO types.
*/
typedef enum { 
  unknown = 0,
  destAddrMode = 1,
  srcAddrMode = 2
} mpdoType;

/*
*   The different data to be found in a multiplex PDO. 
*/
typedef struct {
  mpdoType type;
  uint8 addr;
  uint16 objIdx;
  uint8 subIdx;
  uint32 dataField;
} coMPDOFrameDataT;


/* ... */
typedef struct {
  uint8 blockSize;
  uint16 confObjIdx;
  uint8 confSubIdx;
} coObjScanListDataVars;

/*
*   The "coObjScanListDataEntr" structure is the stucture
*   of the datafield of the objects in range 0x1fa0->0x1fcf.
*/
typedef struct {
  uint8 subIdx;
  coObjScanListDataVars vars;
} coObjScanListDataEntrT;



/*
*   The coScannerListÓbj implements the objects in the range
*   0x1fa0->0x1fcf. 
*/
typedef struct {
  uint8 number;
  coObjScanListDataEntrT subIdxData[MAX_NUMBER_OF_SCAN_LIST_SUBIDXES];
} coScannerListObjT;


/* 
*   The object disapatching list.
*   This is a 64 bit entry, but can all processors handling
*   uint64 efficiently? (qqq: talk to Mats about this)..
*   qqq: is  
*/

typedef struct {
  uint8 blockSize;
  uint16 localObjIdx;
  uint8 localSubIdx; /* The local objects dictionary is "source subidx + offset". Is "block size" dependent parameter. */
  uint16 sendObjIdx;
  uint8 sendSubIdx;
  uint8  sendNodeId;
} dispListDataT;

typedef struct {
  uint8 subIdx;
  dispListDataT vars;
} dispListSubIdxData;

typedef struct {
  uint8 number;
  dispListSubIdxData subIdxData[MAX_NUMBER_OF_DISP_LIST_SUBIDXES];
} coDispListObjT;

#endif

typedef struct {
  uint16 objIdx;
  uint8 subIdx;
  uint32 val;
} coStoreParamStruct;

// LSS States
#define LSSoff    0x00
#define LSSinit   0x01
#define LSSwait   0x10
#define LSSconf   0x55
#define LSSfinal  0xff





#ifdef __cplusplus
}
#endif




#endif /* __CANOPEN_H */


