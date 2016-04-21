
/*
 * Block based EEPROM storage. 
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>

#if 0
#define debug printd
#else
#define debug(...)
#endif

#include "bios/eeprom.h"
#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/cmd.h"

#include "lib/config.h"
#include "lib/atoi.h"
#include "lib/crc16.h"

#define MAGIC 0x32425053 /* SPB1 */

#define ID_LEN_HOLE 0x00u
#define ID_LEN_TERMINATOR 0xFFu

struct config_header {
	uint32_t magic;
	eeprom_addr_t addr_start;
	eeprom_addr_t addr_end;
	uint32_t reserved;
} __attribute__ (( __packed__ ));

struct block_header {
	uint8_t id_len;
	uint16_t data_len;
	uint16_t crc;
} __attribute__ (( __packed__ ));

struct config {
	struct dev_eeprom *dev;
	eeprom_addr_t addr_start;
	eeprom_addr_t addr_end;
};

static rv config_erase(void);
static rv config_test(void);

/* There is only one global configuration store */

static struct config config;


rv config_init(struct dev_eeprom *dev, eeprom_addr_t addr_start, eeprom_addr_t addr_end)
{
	rv r;

	config.dev = dev;
	config.addr_start = addr_start;
	config.addr_end = addr_end;

	/* Verify configuration header */

	struct config_header ch;
	r = eeprom_read(config.dev, config.addr_start, &ch, sizeof(ch));

	if(r == RV_OK) {
		if((ch.magic != MAGIC) || (ch.addr_start != addr_start) || (ch.addr_end != addr_end)) {
			hexdump(&ch, sizeof(ch), 0);
			printd("config header invalid, erasing\n");
			r = config_erase();
		}
	}

	return r;
}



static rv config_erase(void)
{
	assert(config.dev);

	rv r;
	eeprom_addr_t addr = config.addr_start;

	debug("Config erase\n");

	/* Write config store header */

	struct config_header ch;
	memset(&ch, 0, sizeof(ch));
	ch.magic = MAGIC;
	ch.addr_start = config.addr_start;
	ch.addr_end = config.addr_end;
	r = eeprom_write(config.dev, addr, &ch, sizeof(ch));

	/* Fill the rest of the eeprom with 0xff's */

	uint8_t ffs[16];
	memset(ffs, 0xff, sizeof(ffs));

	addr += sizeof(struct config_header);

	while((r == RV_OK) && (addr < config.addr_end)) {
		size_t len = config.addr_end - addr;
		if(len > sizeof(ffs)) len = sizeof(ffs);
		r = eeprom_write(config.dev, addr, &ffs, len);
		addr += len;
	}

	return r;

}


static rv config_find(const char *id, struct block_header *bh_out, eeprom_addr_t *addr, eeprom_addr_t *addr_next)
{
	assert(config.dev);
	
	debug("config_find '%s' %08x\n", id, *addr);

	struct block_header bh;
	rv r = RV_OK;
	uint8_t found = 0;

	if(*addr == 0) *addr = config.addr_start + sizeof(struct config_header);

	while((!found) && (r == RV_OK)) {

		r = eeprom_read(config.dev, *addr, &bh, sizeof(bh));
		debug("read %08x id_len=%d data_len=%d\n", *addr, bh.id_len, bh.data_len);

		if(r == RV_OK) {

			if(bh.id_len == ID_LEN_HOLE || bh.id_len == ID_LEN_TERMINATOR) {

				r = RV_ENOENT;

			} else {

				/* Read id string */

				char id2[bh.id_len];
				r = eeprom_read(config.dev, *addr + sizeof(bh), id2, bh.id_len);

				if(r == RV_OK) {

					if(addr_next) {
						*addr_next = *addr + sizeof(bh) + bh.id_len + bh.data_len;
					}

					if(id == NULL) {

						/* Wildcard match */
						memcpy(bh_out, &bh, sizeof(bh));
						found = 1;

					} else if((strlen(id) == bh.id_len) && (memcmp(id, id2, bh.id_len)) == 0) {

						memcpy(bh_out, &bh, sizeof(bh));
						found = 1;

					} else {
						*addr += sizeof(bh) + bh.id_len + bh.data_len;
					}
				}
			}
		}
	}
	
	debug("config_find '%s' %08x: %s\n", id, *addr, rv_str(r));

	return r;
}


static rv config_list(void)
{
	assert(config.dev);

	eeprom_addr_t addr = 0;
	eeprom_addr_t addr_next = 0;
	rv r = RV_OK;

	printd("list:\n");

	while(r == RV_OK) {
		struct block_header bh;
		r = config_find(NULL, &bh, &addr, &addr_next);

		if(r == RV_OK) {
			char id[bh.id_len + 1];
			r = eeprom_read(config.dev, addr + sizeof(bh), id, bh.id_len);
			if(r == RV_OK) {
				id[bh.id_len] = '\0';
				printd("%08x %04x '%s'\n", addr + sizeof(bh) + bh.id_len, bh.data_len, id);
			}
			addr = addr_next;
		}
	}

	return RV_OK;
}


rv config_read(const char* id, void *data, size_t data_len)
{
	assert(config.dev);

	eeprom_addr_t addr = 0;
	struct block_header bh;
	rv r = config_find(id, &bh, &addr, NULL);
	
	if(r == RV_OK) {

		if(bh.data_len == data_len) {
			r = eeprom_read(config.dev, addr + sizeof(bh) + bh.id_len, data, data_len);

			uint16_t crc1 = bh.crc;
			uint16_t crc2 = CRC16_INIT;

			bh.crc = 0;
			crc2 = crc_update_block(crc2, &bh, sizeof(bh));
			crc2 = crc_update_block(crc2, data, data_len);

			if(crc1 != crc2) {
				r = RV_EIO;
			}
		} else {
			r = RV_EINVAL;
		}
	} else {
		memset(data, 0, data_len);
	}

	return r;
}


rv config_write(const char *id, void *data, size_t data_len)
{
	assert(config.dev);

	eeprom_addr_t addr = config.addr_start;
	struct block_header bh;
	rv r;

	/* Check if block with given ID is already found. If not so, config_find() will
	 * set addr to the first available block */

	r = config_find(id, &bh, &addr, NULL);

	if(r == RV_OK) {
		if(bh.data_len != data_len) {
			r = RV_EINVAL;
		}
	}

	if(r == RV_OK || r == RV_ENOENT) {

		bh.id_len = strlen(id);
		bh.data_len = data_len;
		bh.crc = 0;
		
		if(addr + sizeof(bh) + bh.id_len + data_len < config.addr_end) {
		
			uint16_t crc = CRC16_INIT;
			crc = crc_update_block(crc, &bh, sizeof(bh));
			crc = crc_update_block(crc, data, data_len);
			bh.crc = crc;

			r = eeprom_write(config.dev, addr, &bh, sizeof(bh));
			addr += sizeof(bh);

			if(r == RV_OK) {
				r = eeprom_write(config.dev, addr, id, strlen(id));
				addr += strlen(id);
			}
			if(r == RV_OK) {
				r = eeprom_write(config.dev, addr, data, data_len);
			}
		} else {
			r = RV_ENOSPC;
		}
	}

	return r;
}


rv config_del(const char *id)
{
	assert(config.dev);

	return RV_OK;
}


#ifdef ENABLE_TESTS

static rv config_test(void)
{
	assert(config.dev);
	char buf[16];

#define TEST(r2, exp) { rv r = exp; printd("%4d: %-4s (%s)\n", __LINE__, (r == r2) ? "ok" : "FAIL", rv_str(r)); }

	/* Erase config */

	TEST(RV_OK,     config_erase());

	/* Read non-existing value */

	TEST(RV_ENOENT, config_read("nop", &buf, 10));

	/* Write, overwrite and read new keys */

	TEST(RV_OK,     config_write("aap", "vlieger123", 10));
	TEST(RV_OK,     config_write("aap", "apehaar123", 10));
	TEST(RV_OK,     config_read("aap", &buf, 10));
	TEST(RV_OK,     config_write("noot", "flabbertje", 10));
	TEST(RV_OK,     config_write("noot", "donnderkop", 10));
	TEST(RV_OK,     config_read("noot", &buf, 10));
	TEST(RV_OK,     config_read("aap", &buf, 10));
	
	/* Read more non-existing value */

	TEST(RV_ENOENT, config_read("aapa", &buf, 10));
	TEST(RV_ENOENT, config_read("anoot", &buf, 10));

	/* Try to rewrite and read key with wrong length */

	TEST(RV_EINVAL, config_write("aap", "hallodaar", 9));
	TEST(RV_EINVAL, config_read("aap", &buf, 9));

	/* Write some more to test EEPROM page handling */

	TEST(RV_OK,     config_write("longertest", "longertestdata", 14));
	TEST(RV_OK,     config_read("longertest", &buf, 14));

#if 0
	/* Write until full */

	rv r = RV_OK;
	uint16_t i = 0;
	for(i=0;r==RV_OK;i++) {
		char id[5] = { '0' + ((i/1000) % 10), '0' + ((i/100) % 10), '0' + ((i/10) % 10), '0' + (i % 10), '\0' };
		r = config_write(id, "ABCDEFG", 7);
		if(r != RV_OK) break;
	}
	TEST(RV_ENOSPC, config_write("vol", "noroom!!", 8));
#endif

	return RV_OK;
}

#else
static rv config_test(void) { return RV_EIMPL; }
#endif


static rv on_cmd_config(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1) {
		
		char cmd = argv[0][0];

		if(cmd == 't') {
			r = config_test();
		}
		
		if(cmd == 'l') {
			r = config_list();
		}

		if(strcmp(argv[0], "erase") == 0) {
			r = config_erase();
		}

	}

	return r;
}

CMD_REGISTER(config, on_cmd_config, "erase | l[ist]");


/*
 * End
 */

