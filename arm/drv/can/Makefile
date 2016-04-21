
SRC-$(DRV_CAN_LINUX) += drv/can/can-linux.c
SRC-$(DRV_CAN_STM32) += drv/can/can-stm32.c
SRC-$(DRV_CAN_DUMMY) += drv/can/can-dummy.c
SRC-$(DRV_CAN_SOCKETCAND) += drv/can/can-socketcand.c

SRC-$(DRV_CAN_IXXAT) += drv/can/can-ixxat.c
CFLAGS-$(DRV_CAN_IXXAT) += -Iarch/win32/ixxat/inc/
LIBS-$(DRV_CAN_IXXAT) += -Larch/win32/ixxat/dll -lvcinpl
LDFLAGS-$(DRV_CAN_IXXAT) += -Wl,--enable-stdcall-fixup

