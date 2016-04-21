
#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/arch.h"
#include "bios/cmd.h"


static rv on_cmd_arch(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char c = argv[0][0];

		if(c == 'r') {
			arch_reboot();
			r = RV_OK;
		}

		if(c == 'u') {
			uint32_t t = arch_get_ticks() / 1000;

			printd("%dd, %02d:%02d:%02d\n", 
					(t / (24*60*60)),
					(t / (   60*60)) % 24,
					(t / (      60)) % 60,
					(t             ) % 60);
			r = RV_OK;
		}
	}

	return r;
}

CMD_REGISTER(arch, on_cmd_arch, "r[eboot] | u[ptime]");


/*
 * End
 */

