
/*
 * Glue between fatfs library and bios MMC functions
 */

#include <string.h>

#include "diskio.h"
#include "lib/atoi.h"
#include "bios/bios.h"
#include "bios/mmc.h"
#include "bios/printd.h"
#include "bios/cmd.h"
#include "ff.h"

#define SECTOR_SIZE 512

DSTATUS disk_initialize( BYTE drv)
{
	return RES_OK;
}



DSTATUS disk_status( BYTE drv)
{
	return RES_OK;
}



DRESULT disk_read(BYTE drv, BYTE* buff, DWORD lba, UINT count)
{
	rv r = mmc_read(&DEFAULT_MMC_DEV, lba, buff, count * SECTOR_SIZE);
	return (r == RV_OK) ? RES_OK : RES_ERROR;
}


DRESULT disk_write(BYTE drv, const BYTE* buff, DWORD lba, UINT count)
{
	mmc_write(&DEFAULT_MMC_DEV, lba, buff, count * SECTOR_SIZE);
	return RES_OK;
}


DRESULT disk_ioctl(BYTE drv, BYTE cmd, void* buff)
{
	switch(cmd) {

		case CTRL_SYNC:
			return RES_OK;

		case GET_SECTOR_COUNT:
			*(DWORD*)buff = 15625000;
			return RES_OK;

		case GET_BLOCK_SIZE:
			*(DWORD*)buff = SECTOR_SIZE;
			return RES_OK;
	}

	printd("ioctl %d\n", cmd);
	return RES_PARERR;
}


DWORD get_fattime(void)
{
	return 0;
}


static rv fr_to_rv(FRESULT fr)
{
	rv r = RV_EIMPL;

	if(fr == FR_OK) r = RV_OK;
	if(fr == FR_DISK_ERR) r = RV_EIO;
	if(fr == FR_NOT_READY) r = RV_EBUSY;
	if(fr == FR_TIMEOUT) r = RV_ETIMEOUT;
	if(fr == FR_NO_FILE) r = RV_ENOENT;
	if(fr == FR_NO_PATH) r = RV_ENOENT;
	if(fr == FR_INVALID_NAME) r = RV_EINVAL;
	if(fr == FR_INVALID_OBJECT) r = RV_EINVAL;
	if(fr == FR_INVALID_DRIVE) r = RV_EINVAL;
	if(fr == FR_INVALID_PARAMETER) r = RV_EINVAL;

	if(r == RV_EIMPL) {
		printd("fatfs err %d\n", fr);
	}

	return r;
}


static rv on_cmd_fs(uint8_t argc, char **argv)
{
	static FATFS FatFs;
	rv r = RV_EINVAL;

	FIL fil;  
	FRESULT fr = FR_OK;

	if(argc >= 1) {

		char cmd = argv[0][0];

		if(cmd == 'l') {

			DIR dir;
			FILINFO fno;
			char *path = "";
			if(argc >= 2) {
				path = argv[1];
			}

			fr = f_opendir(&dir, path);
			if(fr == FR_OK) {
				for(;;) {
					fr = f_readdir(&dir, &fno);
					if(fr != FR_OK || fno.fname[0] == 0) break;
					printd("%-12.12s %7d %c%c%c%c%c\n", fno.fname, fno.fsize,
						(fno.fattrib & AM_DIR) ? 'D' : '-',
						(fno.fattrib & AM_RDO) ? 'R' : '-',
						(fno.fattrib & AM_HID) ? 'H' : '-',
						(fno.fattrib & AM_SYS) ? 'S' : '-',
						(fno.fattrib & AM_ARC) ? 'A' : '-');
				}
				f_closedir(&dir);
			}
			r = fr_to_rv(fr);
		}

		if(cmd == 'm') {
			fr = f_mount(&FatFs, "", 1);
			r = fr_to_rv(fr);
		}

		if(cmd == 't' && argc >= 3) {
			fr = f_open(&fil, argv[1], FA_WRITE | FA_CREATE_ALWAYS);
			size_t size = a_to_s32(argv[2]);
			if(fr == FR_OK) {
				UINT bw;
				char c = 'a';
				while(size--) {
					fr = f_write(&fil, &c, sizeof(c), &bw);
				}
			}
			if(fr == FR_OK) {
				fr = f_close(&fil);
			}
			r = fr_to_rv(fr);
		}

#ifdef LIB_FATFS_MFKFS	
		if(strcmp(argv[0], "format") == 0) {
			fr = f_mkfs("", 0, 0);
			r = fr_to_rv(fr);
		}
#endif
		
		if(cmd == 'r' && argc >= 2) {
			fr = f_unlink(argv[1]);
			r = fr_to_rv(fr);
		}

		if(cmd == 'c' && argc >= 2) {
			fr = f_open(&fil, argv[1], FA_READ);
			if(fr == FR_OK) {
				uint8_t buf[32];
				UINT br;
				for(;;) {
					fr = f_read(&fil, buf, sizeof buf, &br);
					if(br == 0 || fr != FR_OK) break;
					uint8_t *p = buf;
					while(br) {
						printd("%c", *p);
						p++;
						br--;
					}
				}
				f_close(&fil);
			}
			r = fr_to_rv(fr);
		}
	}

	return r;
}


CMD_REGISTER(fs, on_cmd_fs, "m[ount] | l[s] | c[at] <file> | r[m] <file>");


/*
 * End
 */
