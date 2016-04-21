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
** Configuration settings of the KVASER CANopen stack.
**
** This file should normally be adapted to each project.
**
*/

#include "config.h"

#ifndef __COCONFIG_H
#define __COCONFIG_H

// #define CANOPEN_MASTER          0 Not used in stack anymore
#define EMERGENCY_MASTER        0
#define CLIENT_SDO_SUPPORT      1

/* Init this number of client SDO with default SDO channel for node id equal to
 * SDO number. Must be less than SDO_MAX_COUNT - 1 and only use in an SDO
 * manager CiA 302-5 */

#ifdef LIB_CANOPEN_MASTER

 #define NODEGUARD_MASTER        1  /* Currently the implementation is not complete, use the heartbeat consumer protocol */
 #define SDO_MANAGER_DEFAULT_CHANNELS 8 
 #define BOOTUP_DS302      1
 #define BOOTUP_MAX_COUNT  1

#else

 #define SDO_MANAGER_DEFAULT_CHANNELS 0

#endif

#define CO_DIRECT_PDO_MAPPING   0
#define CO_MULTIPLE_INSTANCES   1
#define CO_INST_MAX             2


/* PDO Configurable Options */
#define USE_INHIBIT_TIMES			 1
#define USE_TPDO_EVENT_TIMER         1
#define USE_RPDO_TIMEOUT_MONITORING  0  /* Set RPDO timeout in subindex 4 event timer */

/*	Implementation of dynamic mapping of the PDOs 
	By disabling Dynamic pdo mapping the stack will be static mapped...
*/

#define DYNAMIC_PDO_MAPPING               1
#define DYNAMIC_PDO_MAPPING_MAP_ITEMS    20 /* 19 */ /*Cost is 4 bytes*/
#define MAX_DYNAMIC_MAP_ENTRIES 20 /* 19 */

#define CANOPEN_DEVICE_TYPE   0x000001a2        /* Object Index 1000, device type */
#define IDENTITY_OBJECT         1  /* Object Index 1018 */
#define OD_MFG_STRINGS			0  /* Object Index 0x1008-0x100A */

#define EMERGENCY_PRODUCER_HISTORY 1
#define EMERGENCY_CONSUMER_OBJECT   0   /* Object Index 0x1028 */

#define ERROR_BEHAVIOR          1  /* Object Index 1029 */

#define MULTIPLEXED_PDO                     0
#define MAX_DISP_LIST_ENTRIES               0

/* Scanner List implementation */
#define MAX_NUMBER_OF_SCAN_LIST_SUBIDXES   0
#define MAX_NUMBER_OF_SCAN_LISTS           0

/* Dispatcher List implementation */
#define MAX_NUMBER_OF_DISP_LIST_SUBIDXES   1


/* CiA-301 v4.2.0, chapter 7.5.2.13 Object 1010h: Store parameters */
#define STORE_PARAMS                        0
#define STOREPARAM_BUF_SIZE                 100   /* Size of RAM buffer used when storing application and manufacturer specific parameters */


/* CiA-301 v4.2.0, chapter 7.5.2.14 Object 1011h: Restore default parameters */
#define RESTORE_PARAMS                      0

#define TIME_H_IS_AVAILABLE                 0 /* This flag currently has no other effect than increaseing the size of the binary, don't use */

/* Local SDO buffer size for segmented transfers */
#define SIZE_LOCAL_SDO_BUF                  8

#define CO_SDO_TIMEOUT                      kfhalMsToTicks(500)
#define MAX_GUARDED_NODES                   1 /* Max no. nodeguard nodes. */
#define MAX_HEARTBEAT_NODES                 1 /* Max no. heartbeat nodes. */
#define MAX_EMCY_GUARDED_NODES              1 /* Max no. emcy identifiers that will be guarded. */
#define MAX_EMCY_PRODUCER_HISTORY_RECORDED  1 /* Max no. emcy history. */

/* SDO and PDO max numbers */

#ifdef LIB_CANOPEN_MASTER
#define SDO_MAX_COUNT (SDO_MANAGER_DEFAULT_CHANNELS + 1)
#else
#define SDO_MAX_COUNT 1
#endif

#define PDO_MAX_COUNT 6 /* Up to 1024 PDOs can exists (512 receive, 512 transmit) */

/* Implementation of the boot up (DS302). */

/* Use subindex 0xFF to indicate data type of object - CiA 301 v4.2.0 - 7.4.7.2 */
#define SUBINDEX_FF_STRUCTURE 0
#define NODE_GUARDING 1 /* Support the obsolete node guarding protocol (server/node side). */
#define LIFE_GUARDING 0 /* Life guarding */
#define HEARTBEAT_CONSUMER 1 /* Support for hearbeat monitoring according to DS301 */
#define LSS_SLAVE 0 /* Support for Layer Setting Services CiA 305) */
#define SYNC_WINDOW_LENGTH 1  /* Support for object id 0x1007 - Synchronous Window Lenght */

/* Max COB IDs 
 * Tells the stack how many COB IDs it will need to listen to. This define
 * is used to reserve memory for the COB ID list sent down to the HAL layer
 *
 * If the driver does not use filters this define should be set to 0
 * to save RAM.
 *
 * Uses 2 x MAX_COB_IDS bytes of RAM if USE_EXTENDED_CAN is 0
 * Uses 4 x MAX_COB_IDS bytes of RAM if USE_EXTENDED_CAN is 1
 */ 
#define MAX_COB_IDS 20 /* Max number of COB IDs to listen to */

// Error checking, prevents invalid configurations

#if RESTORE_PARAMS && !STORE_PARAMS
	#error You must enable STORE_PARAMS before enabling RESTORE_PARAMS
#endif

#if STORE_PARAMS && STOREPARAM_BUF_SIZE < 2
	#ERROR The store parameters buffer must be at least 2 bytes in size
#endif

#if SDO_MANAGER_DEFAULT_CHANNELS > SDO_MAX_COUNT - 1
#error Cannot allocate more client SDO channels than SDO_MAX_COUNT - 1 (default server)
#endif

#if CO_INST_MAX > 1 && !CO_MULTIPLE_INSTANCES
  #error CO_INST_MAX > 1 and CO_MULTIPLE_INSTANCES == 0!
#endif

#endif /* __COCONFIG_H */

