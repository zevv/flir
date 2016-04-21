
#include <string.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/sysinfo.h"
#include "bios/cmd.h"
#include "lib/config.h"

struct sysinfo {
	char name[SYSINFO_PROP_LEN];
	char location[SYSINFO_PROP_LEN];
	char contact[SYSINFO_PROP_LEN];
};


static struct sysinfo sysinfo;


void sysinfo_init(void)
{
#ifdef LIB_CONFIG
	(void)config_read("sysinfo", &sysinfo, sizeof(sysinfo));
#endif
}


rv sysinfo_set(enum sysinfo_prop s, const char *val)
{
	char *p;
	rv r;

	switch(s) {
		case SYSINFO_NAME:
			p = sysinfo.name; 
			break;
		case SYSINFO_LOCATION: 
			p = sysinfo.location; 
			break;
		case SYSINFO_CONTACT: 
			p = sysinfo.contact; 
			break;
		default:
			p = NULL;
			break;
	}

	if(p) {
		r = RV_OK;
		strncpy(p, val, SYSINFO_PROP_LEN);
		p[31] = '\0';
#ifdef LIB_CONFIG
		config_write("sysinfo", &sysinfo, sizeof(sysinfo));
#endif
	} else {
		r = RV_EINVAL;
	}
	
	return r;
}


rv sysinfo_get_version(struct sysinfo_version *ver)
{
	ver->maj = VERSION_MAJ;
	ver->min = VERSION_MIN;
	ver->rev = VERSION_REV;
	ver->dev = VERSION_DEV;
	return RV_OK;
}


rv sysinfo_get(enum sysinfo_prop s, char *val, size_t maxlen)
{
	rv r = RV_ENOENT;
	char *p;

	val[0] = '\0';
	
	switch(s) {
		case SYSINFO_NAME:
			p = sysinfo.name; 
			break;
		case SYSINFO_LOCATION: 
			p = sysinfo.location; 
			break;
		case SYSINFO_CONTACT: 
			p = sysinfo.contact; 
			break;
		default:
			p = NULL;
			break;
	}

	if(p) {
		strncpy(val, p, maxlen);
		val[maxlen-1] = '\0';
		r = RV_OK;
	}

	return r;
}


static rv on_cmd_sysinfo(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {

		char cmd = argv[0][0];
		
		if(cmd == 'n' && argc >= 2) {
			r = sysinfo_set(SYSINFO_NAME, argv[1]);
		}
		if(cmd == 'l' && argc >= 2) {
			r = sysinfo_set(SYSINFO_LOCATION, argv[1]);
		}
		if(cmd == 'c' && argc >= 2) {
			r = sysinfo_set(SYSINFO_CONTACT, argv[1]);
		}

	} else {
		struct sysinfo_version ver;
		sysinfo_get_version(&ver);

		printd("v%d.%d (%d%s) / ", ver.maj, ver.min, ver.rev, ver.dev ? "M" : "");

		char buf[SYSINFO_PROP_LEN];
		sysinfo_get(SYSINFO_NAME, buf, sizeof(buf));
		printd("%s ", buf);
		sysinfo_get(SYSINFO_LOCATION, buf, sizeof(buf));
		printd("%s ", buf);
		sysinfo_get(SYSINFO_CONTACT, buf, sizeof(buf));
		printd("%s\n", buf);
		r = RV_OK;
	}


	return r;
}


CMD_REGISTER(sysinfo, on_cmd_sysinfo, "l[ist] | n[ame] <val> | l<ocation> <val> | c<ontact> <val>");

/*
 * End
 */


