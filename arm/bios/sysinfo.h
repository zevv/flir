#ifndef bios_sysinfo_h
#define bios_sysinfo_h

#define SYSINFO_PROP_LEN 16

struct sysinfo_version {
	uint8_t maj;
	uint8_t min;
	uint16_t rev;
	uint8_t dev;
};

enum sysinfo_prop {
	SYSINFO_NAME,
	SYSINFO_LOCATION,
	SYSINFO_CONTACT,
};

void sysinfo_init(void);
rv sysinfo_set(enum sysinfo_prop s, const char *val);
rv sysinfo_get(enum sysinfo_prop s, char *val, size_t maxlen);
rv sysinfo_get_version(struct sysinfo_version *ver);

#endif

