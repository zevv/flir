#include "lpc_types.h"
#include "error.h"
#include "usbd_rom_api.h"

#ifndef __APP_USB_CFG_H_
#define __APP_USB_CFG_H_


/* HID In/Out Endpoint Address */
#define HID_EP_IN       0x81
#define HID_EP_OUT      0x01

/* The following manifest constants are used to define this memory area to be used
   by USBD ROM stack.
   */
#define USB_STACK_MEM_BASE      0x20004000
#define USB_STACK_MEM_SIZE      0x0800

/* Manifest constants used by USBD ROM stack. These values SHOULD NOT BE CHANGED
   for advance features which require usage of USB_CORE_CTRL_T structure.
   Since these are the values used for compiling USB stack.
   */
#define USB_MAX_IF_NUM          8  
#define USB_MAX_EP_NUM          5  
#define USB_MAX_PACKET0         64  
#define USB_FS_MAX_BULK_PACKET  64  
#define USB_HS_MAX_BULK_PACKET  512  
#define USB_DFU_XFER_SIZE       2048 
/* USB descriptor arrays defined *_desc.c file */
extern const uint8_t USB_DeviceDescriptor[];
extern uint8_t USB_HsConfigDescriptor[];
extern uint8_t USB_FsConfigDescriptor[];
extern const uint8_t USB_StringDescriptor[];
extern const uint8_t USB_DeviceQualifier[];

extern USB_INTERFACE_DESCRIPTOR *find_IntfDesc(const uint8_t *pDesc, uint32_t intfClass);

#endif

