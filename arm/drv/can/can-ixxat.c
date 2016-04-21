#include <windows.h>

#include "bios/bios.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "bios/can.h"
#include "drv/can/can.h"
#include "drv/can/can-ixxat.h"

#include "arch/win32/ixxat/inc/vcinpl.h"
#include "arch/win32/mainloop.h"

static DWORD WINAPI ReceiveThread(void* Param);

static rv vci_to_rv(HRESULT rvci)
{
	rv r;

	if(rvci == VCI_OK) {
		r = RV_OK;
	} else {
		char err[256];
		vciFormatError(rvci, err, sizeof(err));
		printd("Ixxat: %s (%08x)\n", err, rvci);
		r = RV_ENODEV;
	} 

	return r;
}


static rv init(struct dev_can *dev)
{
	struct drv_can_ixxat_data *dd = dev->drv_data;

	HRESULT r;
	HANDLE hEnum;
	VCIDEVICEINFO sInfo;
	UINT16 wRxFifoSize  = 1024;
	UINT16 wRxThreshold = 1;
	UINT16 wTxFifoSize  = 128;
	UINT16 wTxThreshold = 1;

	/* 
	 * Initialization
	 */
	
	r = vciInitialize();
	if(r != VCI_OK) goto out;

	/* 
	 * List available devices and open first 
	 */

	r = vciEnumDeviceOpen(&hEnum);
	if(r != VCI_OK) goto out;

	r = vciEnumDeviceNext(hEnum, &sInfo);
	if(r != VCI_OK) goto out;

	vciEnumDeviceClose(hEnum);

	r = vciDeviceOpen(&sInfo.VciObjectId, &dd->hdevice);
	if(r != VCI_OK) goto out;

	/* 
	 * Open channel
	 */

	r = canChannelOpen(dd->hdevice, 0, FALSE, &dd->hchannel);
	if(r != VCI_OK) goto out;

	r = canChannelInitialize(dd->hchannel, 
				wRxFifoSize, wRxThreshold,
				wTxFifoSize, wTxThreshold);

	if(r != VCI_OK) goto out;

	r = canChannelActivate(dd->hchannel, TRUE);
	if(r != VCI_OK) goto out;

	/*
	 * Open controller
	 */

	r = canControlOpen(dd->hdevice, 0, &dd->hcontrol);
	if(r != VCI_OK) goto out;

	r = canControlInitialize(dd->hcontrol, CAN_OPMODE_STANDARD | CAN_OPMODE_ERRFRAME, CAN_BT0_250KB, CAN_BT1_250KB);
	if(r != VCI_OK) goto out;

	r = canControlSetAccFilter(dd->hcontrol, FALSE, CAN_ACC_CODE_ALL, CAN_ACC_MASK_ALL);
	if(r != VCI_OK) goto out;

	r = canControlStart(dd->hcontrol, TRUE);
	if(r != VCI_OK) goto out;

	/* 
	 * Create RX thread
	 */

	DWORD threadDescriptor;

	CreateThread(NULL, 0, ReceiveThread, dev, 0, &threadDescriptor);

out:
	return vci_to_rv(r);
}


static rv tx(struct dev_can *can, enum can_address_mode_t fmt, can_addr_t addr, const void *data, size_t len)
{
	struct drv_can_ixxat_data *dd = can->drv_data;

	HRESULT r;
	CANMSG  sCanMsg;

	sCanMsg.dwTime   = 0;
	sCanMsg.dwMsgId  = addr;
	memcpy(sCanMsg.abData, data, len);

	sCanMsg.uMsgInfo.Bits.dlc     = len;
	sCanMsg.uMsgInfo.Bytes.bType  = CAN_MSGTYPE_DATA;
	sCanMsg.uMsgInfo.Bytes.bFlags = CAN_MAKE_MSGFLAGS(8,0,0,0,0);
	sCanMsg.uMsgInfo.Bits.srr     = 1;

	r = canChannelSendMessage(dd->hchannel, INFINITE, &sCanMsg);

	return vci_to_rv(r);
}


static DWORD WINAPI ReceiveThread(void* Param)
{
	struct dev_can *dev = Param;
	struct drv_can_ixxat_data *dd = dev->drv_data;

	HRESULT r;
	CANMSG  sCanMsg;

	Sleep(100);

	for(;;) {

		r = canChannelReadMessage(dd->hchannel, 100, &sCanMsg);
	
		if(r == (int)VCI_E_TIMEOUT) {
			/* No problem */
		}

		else if (r == VCI_OK) {

			if(sCanMsg.uMsgInfo.Bytes.bType == CAN_MSGTYPE_DATA) {

				event_t e;
				e.type = EV_CAN;
				e.can.dev = dev;
				e.can.id = sCanMsg.dwMsgId;
				e.can.len = sCanMsg.uMsgInfo.Bits.dlc;
				memcpy(e.can.data, sCanMsg.abData, sCanMsg.uMsgInfo.Bits.dlc);
				evq_push(&e);

			} 
			
			else if (sCanMsg.uMsgInfo.Bytes.bType == CAN_MSGTYPE_INFO) {

				switch (sCanMsg.abData[0])
				{
					case CAN_INFO_START: printd("CAN started\n"); break;
					case CAN_INFO_STOP : printd("CAN stoped\n");  break;
					case CAN_INFO_RESET: printd("CAN reseted\n"); break;
				}
			}
		
			else if (sCanMsg.uMsgInfo.Bytes.bType == CAN_MSGTYPE_ERROR) {

				switch (sCanMsg.abData[0])
				{
					case CAN_ERROR_STUFF: printd("stuff error\n");   break; 
					case CAN_ERROR_FORM : printd("form error\n");    break; 
					case CAN_ERROR_ACK  : printd("ack error\n");     break;
					case CAN_ERROR_BIT  : printd("bit error\n");     break; 
					case CAN_ERROR_CRC  : printd("CRC error\n");     break; 
					case CAN_ERROR_OTHER:
					default             : printd("other error\n");   break;
				}
			}
		}
	
		else {
			vci_to_rv(r);
		}
	}

	return 0;
}


const struct drv_can drv_can_ixxat = {
	.init = init,
	.tx = tx
};



/*
 * End
 */
