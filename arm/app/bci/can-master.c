
/*lint -e754 identifier not referenced */

#include <string.h>

#include "bios/eeprom.h"
#include "bios/bios.h"
#include "bios/log.h"
#include "bios/evq.h"
#include "bios/led.h"
#include "bios/can.h"
#include "bios/arch.h"
#include "bios/button.h"
#include "bios/sysinfo.h"

#include "lib/config.h"
#include "lib/canopen/inc/canopen.h"
#include "lib/canopen/inc/canopenM.h"
#include "lib/canopen/hal-can.h"

#include "app/bci/can-master.h"
#include "app/bci/bci-api.h"
#include "app/bci/modules.h"
#include "app/bci/can-scanner.h"
#include "app/bci/device_state.h"

static size_t mod_read_ptr = 0;

/*
 * 0x2ff8 - Module administration
 */

struct od_module {
	uint8_t Count;
	uint8_t Index;
	uint8_t Flags;
	char Name[8]; 
	char Description[8];
	uint16_t MinManBlck0;
	uint16_t MaxManBlck0;
	uint16_t MinDevBlck0;
	uint16_t MaxDevBlck0;
	uint16_t MinManBlck1;
	uint16_t MaxManBlck1;
	uint16_t MinDevBlck1;
	uint16_t MaxDevBlck1;
	uint8_t CountVis;
	uint8_t Options;
	uint8_t UserSelect;
	uint8_t NetName; /* fixme: string */
	uint16_t MinBaud;
	uint16_t MaxBaud;
	uint16_t Revision;
};


static const coObjDictVarDesc ode_module[20] = {
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE + CO_OD_WRITE_NOTIFY },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 8, CO_OD_ATTR_READ + CO_OD_READ_NOTIFY },
	{ 0, NULL, 8, CO_OD_ATTR_READ + CO_OD_READ_NOTIFY },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
};


/*
 * 0x2ffa - Battery lay-out functionality
 */

struct od_layout {
	uint8_t cia418TotCells;
        uint8_t BattLaySeries;
        uint8_t BattLayParallel;
        char BattPassword[16];
        uint8_t BattActualCells;
};

static const coObjDictVarDesc ode_layout[5] = {
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE + CO_OD_WRITE_NOTIFY },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 16, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE + CO_OD_ATTR_STRING + CO_OD_WRITE_NOTIFY },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
};


/*
 * 0x2ffc - Board functionality
 */

struct od_board {

#define  BRD_SYS_MASTER         0x20u    /* Bit5 - Status of Master CANopen-channel */
#define  BRD_SYS_SCAN           0x40u    /* Bit6 - Status of Full scan */

	uint8_t stat;
	uint16_t inprel;
	uint8_t reldelay;
	uint8_t inpconfig;
	uint8_t ondelay;
	uint8_t setting;
	uint16_t led;

#define	 BRD_SXS_MODE_OFF       0x0000u  /* Mode OFF */
#define	 BRD_SXS_MODE_ON        0x0001u  /* Mode ON */
#define	 BRD_SXS_MODE_PRE       0x0002u  /* Mode Precharge */
#define	 BRD_SXS_MODE_BAL       0x0003u  /* Mode Balance, not shown on BattMon */
#define	 BRD_SXS_MODE_REC       0x0004u  /* Mode Recovery */

#define	 BRD_SXS_ERR_NO         (0x00u<<3u) /* No error */
#define  BRD_SXS_ERR_START      (0x01u<<3u) /* Error, default after start */
#define  BRD_SXS_ERR_SWITCH     (0x02u<<3u) /* Error, by active switch input */
#define  BRD_SXS_ERR_COMMAND    (0x03u<<3u) /* Error, via CANopen command */
#define  BRD_SXS_ERR_MISSING    (0x04u<<3u) /* Error, node(s) missing */
#define  BRD_SXS_ERR_NODEOFF    (0x05u<<3u) /* Error, node(s) relay(s) OFF */
#define  BRD_SXS_ERR_SWREJ      (0x06u<<3u) /* Error, switch rejected */
#define  BRD_SXS_ERR_UNKNOWN    (0x07u<<3u) /* Error, general / unknown source */

#define  BRD_SXS_FSINHIBIT      0x1000u  /* Bit12 - Status of Full Scan Inhibit */
#define  BRD_SXS_MASTER         0x2000u  /* Bit13 - Status of Master CANopen-channel */
#define  BRD_SXS_SCAN           0x4000u  /* Bit14 - Status of Full scan */

	uint16_t statext;
	uint8_t errinhibit;
	uint16_t relmanctrl;
	uint16_t relmanset;
} __attribute__ (( __packed__ ));

static const coObjDictVarDesc ode_board[11] = {
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
};


/*
 * 0x2ffd - Switch functionality
 */

struct od_swit {

#define SWCMD_NOP		0x00	/* No OPeration */
#define SWCMD_VERIFYSCAN  	0x01	/* Verification scan */
#define SWCMD_CHNGBITRATE  	0x02	/* Change master network bitrate */
#define SWCMD_SYSDISABLE  	0x03	/* Disable all scan, polling and admin functionality */
#define SWCMD_SYSENABLE	  	0x04	/* Enable all scan, polling and admin functionality */
#define SWCMD_PWRUPRESET	0x05    /* Full power-up reset (Password 0xAA55 needed) */
#define SWCMD_FULLSCAN  	0x06	/* Full scan */
#define SWCMD_POLLFULL  	0x10    /* Full polling */
#define SWCMD_POLLLOWP  	0x11    /* Low-priority polling */
#define SWCMD_POLLSTND		0x12	/* Standard polling */
#define SWCMD_EQUAL		0x13	/* Equalization */
#define SWCMD_RSTBATTHARD	0x20	/* Reset connected batteries */
#define SWCMD_CHNGNODEID	0x21	/* Change node-Id of a node in the master network */
#define SWCMD_RSTBATTSOFT	0x22	/* Reset connected batteries and reset relays */
#define SWCMD_LSSSAVE		0x30    /* Save master NodeId & Bitrate */
#define SWCMD_MTRRESET		0x31	/* Perform master reset */
#define SWCMD_PRNADMIN		0x40    /* Print admin via UART */
#define SWCMD_UARTCANEN 	0x41	/* Enable UARTprinting via CAN */
#define SWCMD_UARTCANDIS	0x42	/* Disable UARTprinting via CAN */
#define SWCMD_RESETPWRMGT	0x50	/* Reset MAS Power management */
#define	SWCMD_NEWMCOMMAND	0x7F	/* Special (internal) command to set new command */
#define SWCMD_BRDOFF 		0x80	/* Set board OFF */
#define SWCMD_BRDON 		0x81	/* Set board to ON */
#define SWCMD_BRDPRE 		0x82	/* Set board to Precharge */
#define SWCMD_BRDBAL 		0x83	/* Set board to Balance */
#define SWCMD_BRDERROR 		0x84	/* Set board to Error */
#define SWCMD_BRDRESET 		0x8F	/* Reset board */
#define	SWCMD_MASPOLLVOLT	0x90	/* Start polling ADC Voltage channels */
#define	SWCMD_XXXXX1		0x91	/* Switch CANmaster Power Control ON - Not longer supported */
#define	SWCMD_XXXXX2		0x92	/* Switch CANmaster Power Control OFF - Not longer supported*/
#define	SWCMD_MASBATTPWRON	0x93	/* Switch Battery Power Supply Control ON */
#define	SWCMD_MASBATTPWROFF	0x94	/* Switch Battery Power Supply Control OFF */
#define SWCMD_DEFEESETTINGS 	0xA0	/* Reset EE(parts) to default settings (Password 0xAA55 and action byte #3 needed) */
#define SWCMD_ERASELOG		0xA1	/* Erase Event Log (Password 0xAA55) */

	uint8_t   cmd;
	uint16_t  poll;
	uint16_t  lowprio;
	uint8_t   mnode;
	uint8_t   midx;
	uint32_t  stat;
	uint8_t   equalint;
	uint8_t   custcmd;
	uint16_t  missing;
	uint16_t  backon;
	uint32_t  opercmd;
	uint8_t   bmchannel;
	uint8_t   fsinhibit;
	uint8_t   snode;
	uint8_t   sidx;
	uint16_t  pollcycle;
	uint32_t  bootrev;
} __attribute__ (( __packed__ ));

static const coObjDictVarDesc ode_swit[17] = {
	{ 0, NULL, 1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE + CO_OD_WRITE_NOTIFY },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE + CO_OD_WRITE_NOTIFY },
	{ 0, NULL, 4, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE + CO_OD_WRITE_NOTIFY },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 4, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 4, CO_OD_ATTR_READ },
};


/* 0x2ffe: Battery info */
 
struct od_swbatt {
	uint8_t node;
	uint32_t serial;
	uint32_t version;
	uint8_t reserved0;
	uint8_t flags;
	uint16_t errors;
	uint16_t lasterror;
	int32_t current;
	uint16_t voltage;
	uint16_t temp;
	uint16_t cycles;
	uint8_t chargestate;
	uint16_t ahcap;
	uint8_t statflags;

#define	BATMIN_FG_WIPE		0x0000			/* wipe all */
#define	BATMIN_FG_FOUND		0x0001			/* CANopen Node found */
#define	BATMIN_FG_ONLINE    	0x0002			/* Node on-line status */
#define	BATMIN_FG_BATT		0x0004			/* Node is Super-B Battery */
#define BATMIN_FG_CHECK		0x0008			/* Check node after on-line status changed */
#define	BATMIN_FG_SCAN		0x0010			/* Node occupied by full scan */
#define	BATMIN_FG_CHANGED	0x0020			/* Node on-line status changed */
#define	BATMIN_FG_MISSING	0x0080			/* Node is missing */
#define	BATMIN_FG_BACKON	0x0100			/* Node is back on-line */
#define	BATMIN_FG_BOOTLOAD	0x0200			/* Node is bootloader */
#define	BATMIN_FG_DISABLED	0x8000			/* Node is Disabled */

	uint16_t adminflags;
} __attribute__ (( __packed__ ));

static const coObjDictVarDesc ode_swbatt[15] = {
	{ 0, NULL, 1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE + CO_OD_WRITE_NOTIFY },
	{ 0, NULL, 4, CO_OD_ATTR_READ },
	{ 0, NULL, 4, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 4, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
};


/*
 * CiA 418 index 6020 
 */

struct od_i6020 {
	uint8_t battery_type;
	uint16_t ah_capacity;
	uint16_t I_charge_max;
	uint16_t cell_count;
} __attribute__ (( __packed__ ));

static const coObjDictVarDesc ode_i6020[4] = {
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
	{ 0, NULL, 2, CO_OD_ATTR_READ },
};


/*
 * 0x2fff: bridge
 */

struct od_bridge {
	uint8_t node;	
	uint8_t error;	
	uint8_t timeout;	
	uint8_t logging;	
} __attribute__ (( __packed__ ));

static const coObjDictVarDesc ode_bridge[4] = {
	{ 0, NULL, 1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE + CO_OD_WRITE_NOTIFY },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
	{ 0, NULL, 1, CO_OD_ATTR_READ },
};


/*
 * Local object database
 */

struct od_ivars {
	uint8_t status;
	uint8_t program_control;
	uint32_t U_batt;
	uint32_t I_batt;
	uint32_t I_req;
	uint16_t errors;
	uint16_t cycle_count;
	uint16_t T_batt;
	uint8_t soc;
	uint16_t cmd;

	uint32_t overview[4];

	struct od_module module;
	struct od_layout layout;
	struct od_board board;
	struct od_swit swit;
	struct od_swbatt swbatt;
	struct od_i6020 i6020;
	struct od_bridge bridge;
} __attribute__ (( __packed__ ));



/*
 * Local object database
 */

static struct od_ivars ov;

const coPdoMapItem PDOmap[] = { 
        CO_PDO_MAP_END
};

static const coObjDictVarDesc
   ode_8_r        = { 0,  NULL,   1, CO_OD_ATTR_READ, 0 },
   ode_8_rw       = { 0,  NULL,   1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE, 0 },
   ode_8v_rw      = { 0,  NULL,   1, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE + CO_OD_ATTR_VECTOR, 0 },
   ode_16_r       = { 0,  NULL,   2, CO_OD_ATTR_READ, 0 },
   ode_16_rw      = { 0,  NULL,   2, CO_OD_ATTR_READ + CO_OD_ATTR_WRITE + CO_OD_WRITE_NOTIFY, 0 },
   ode_32_r       = { 0,  NULL,   4, CO_OD_ATTR_READ, 0 },
   ode_32v_r      = { 0,  NULL,   4, CO_OD_ATTR_READ + CO_OD_ATTR_VECTOR, 0 };

/*lint -e9027 -e9053 */

const coObjDictEntry ODapp[] = {
	{ 0x1f51,    1, &ov.program_control, &ode_8_rw,    CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_INT8  )  },

	{ 0x2001,    0, &ov.errors,          &ode_16_r,    CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_INT16 )  },
	{ 0x2010,    0, &ov.I_batt,          &ode_32_r,    CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_INT32 )  },
	{ 0x2014,    0, &ov.cycle_count,     &ode_16_r,    CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_UINT16)  },

	{ 0x2ff8,   20, &ov.module,          ode_module,   CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_UINT32)  },
	{ 0x2ff9,    4,  ov.overview,        &ode_32v_r,   CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_UINT32)  },
	{ 0x2ffa,    5, &ov.layout,          ode_layout,   CO_ODDEF(CO_ODOBJ_RECORD, CO_ODTYPE_UINT32)  },
	{ 0x2ffc,   11, &ov.board,           ode_board,    CO_ODDEF(CO_ODOBJ_RECORD, CO_ODTYPE_UINT32)  },
	{ 0x2ffd,   17, &ov.swit,            ode_swit,     CO_ODDEF(CO_ODOBJ_RECORD, CO_ODTYPE_UINT32)  },
	{ 0x2ffe,   14, &ov.swbatt,          ode_swbatt,   CO_ODDEF(CO_ODOBJ_RECORD, CO_ODTYPE_UINT32)  },
	{ 0x2fff,    4, &ov.bridge,          ode_bridge,   CO_ODDEF(CO_ODOBJ_RECORD, CO_ODTYPE_UINT32)  },
	
	{ 0x5000,    0, &ov.cmd,             &ode_16_rw,   CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_UINT16)  },
	
	{ 0x6000,    0, &ov.status,          &ode_8_r,     CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_INT8  )  },

	{ 0x6020,    4, &ov.i6020,           ode_i6020,    CO_ODDEF(CO_ODOBJ_RECORD, CO_ODTYPE_UINT32)  },
	{ 0x6070,    0, &ov.I_req,           &ode_32_r,    CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_INT32 )  },

	{ 0x6010,    0, &ov.T_batt,          &ode_16_r,    CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_UINT16)  },
	{ 0x6060,    0, &ov.U_batt,          &ode_32_r,    CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_UINT32)  },
	{ 0x6081,    0, &ov.soc,             &ode_8_r,     CO_ODDEF(CO_ODOBJ_VAR,    CO_ODTYPE_UINT8 )  },

	{      0,    0, NULL,                NULL,         0                                            },
};



static void update_ov(void);
static void on_ev_tick_10hz(event_t *ev, void *data);


/*
 * Public functions
 */

rv can_master_init(CoHandle ch)
{
	uint8_t i;
	for(i=0; i<32; i++) {
		const struct module *mod;
		rv r = module_get_info(i, &mod);
		if(r == RV_OK) {
			ov.module.Count = i;
			ov.module.CountVis = i;
		}
	}

	ov.module.Count = 3;
	ov.module.CountVis = 8;
	ov.module.UserSelect = 2;

	update_ov();

	evq_register(EV_TICK_10HZ, on_ev_tick_10hz, ch);
	return RV_OK;

}


/*
 * Accumulate the properties of all batteries into our own object
 * dictionary
 */

static void update_ov(void)
{
	rv r;

	ov.program_control = 1;
	ov.status = 1;
	ov.errors = 0;

	ov.swit.mnode = 1;
	
	enum bci_state state;
	(void)bci_get_state(&state);

	ov.board.stat = BRD_SYS_MASTER;
	ov.board.statext = BRD_SXS_MASTER;

	if(state == BCI_STATE_SCANNING) {
		ov.board.stat |= BRD_SYS_SCAN;
		ov.board.statext |= BRD_SXS_SCAN;
	}

	/* Device relay state */

	{
		enum device_state ds;
		(void)device_state_get(&ds);
		switch(ds) {
			case DEVICE_STATE_INIT:
			case DEVICE_STATE_OFF:
				ov.board.stat |= BRD_SXS_MODE_OFF;
				ov.board.statext |= BRD_SXS_MODE_OFF;
				break;
			case DEVICE_STATE_ON1:
			case DEVICE_STATE_ON2:
			case DEVICE_STATE_ON:
				ov.board.stat |= BRD_SXS_MODE_ON;
				ov.board.statext |= BRD_SXS_MODE_ON;
				break;
			case DEVICE_STATE_PRE1:
			case DEVICE_STATE_PRE:
				ov.board.stat |= BRD_SXS_MODE_PRE;
				ov.board.statext |= BRD_SXS_MODE_PRE;
				break;
			default:
				/* misra */
				break;
		}
	}

	/* Switch status */

	{
		enum can_scanner_state st;
		uint8_t perc_done;
		(void)can_scanner_get_state(&st, &perc_done);

		if(st == CAN_SCANNER_STATE_SCAN) {
			ov.swit.stat = SWCMD_FULLSCAN | (perc_done << 8);
		} else {
			ov.swit.stat = 0;
		}
	}

	/* Get list of available batteries */

	{

		memset(ov.overview, 0, sizeof(ov.overview));
		uint8_t count = 0;

		enum batt_state state_list[BATT_ID_MAX+1u];
		r = bci_get_batt_state(state_list, BATT_ID_MAX);
		if(r == RV_OK) {
			uint8_t i;
			for(i=0; i<=BATT_ID_MAX; i++) {
				if(state_list[i] == BATT_STATE_AVAILABLE) {
					uint8_t d1 = i / 32u;
					uint32_t d2 = (uint32_t)1u << (i % 32u);
					ov.overview[d1] |= d2;
					count ++;
				}
			}
		}

		ov.i6020.cell_count = count;
		ov.layout.cia418TotCells = count;
		ov.layout.BattLaySeries = count;
		ov.layout.BattLayParallel = 1;
		ov.layout.BattActualCells = count;

	}

	/* Update parameters for selected battery */

	if(ov.swbatt.node != 0u) {
		struct batt_data data;
		r = bci_get_batt_data(ov.swbatt.node, &data);
		if(r == RV_OK) {
			ov.swbatt.serial = 1234;
			ov.swbatt.version = 1231;
			ov.swbatt.flags = 0x81; /* 0x80 = online | 0x40 = bootloader | 0x01 = relay */
			ov.swbatt.errors = 0;
			ov.swbatt.lasterror = 0;
			ov.swbatt.current = data.I * 1000.0;
			ov.swbatt.voltage = data.U * 1024.0;
			ov.swbatt.temp = data.T_batt * 10.0;
			ov.swbatt.cycles = data.cycle_count;
			ov.swbatt.chargestate = data.soc;
			ov.swbatt.ahcap = data.Q_design / 3600.0;
			ov.swbatt.statflags = 0;
			ov.swbatt.adminflags = BATMIN_FG_FOUND | BATMIN_FG_ONLINE | BATMIN_FG_BATT;
		} else {
			logd(LG_WRN, "batt_get_data(%d): %s", ov.swbatt.node, rv_str(r));
		}
	}

	/* Update ovd battery data */

	{
		struct accum_batt_data data;
		r = bci_get_accum_data(&data);
			
		ov.i6020.battery_type = 0xA0; /* lithium ion */

		if(r == RV_OK) {

			ov.U_batt = data.U * 1024;
			ov.I_batt = data.I * 1000;
			ov.T_batt = data.T_batt * 10;
			ov.soc = data.soc;
			ov.cycle_count = data.cycle_count;
			ov.I_req = data.I_request;

			ov.i6020.ah_capacity = data.Q_design / 3600.0;
			ov.i6020.I_charge_max = data.I_charge_max;

		} else {
			ov.U_batt = 0;
			ov.I_batt = 0;
			ov.T_batt = 0;
			ov.soc = 0;
			ov.cycle_count = 0;
			
			ov.i6020.ah_capacity = 0;
			ov.i6020.I_charge_max = 0;
		}
	}

}


static void on_ev_tick_10hz(event_t *ev, void *data)
{
	static uint8_t n = 0;
	if(n == 5u) {
		update_ov();
		n = 0;
	}
	n = n + 1u;
}


/*
 * CANopen stack application callbacks
 */

bool appGetIdentityObject(CoHandle ch, uint32 *vendorId, uint32 *prodCode, uint32 *revNum, uint32 *serialNum)
{
	*vendorId = 0x0000037Cu;
	*prodCode = 0x00000007u;
	
	struct sysinfo_version ver;
	sysinfo_get_version(&ver);

	*revNum   = 
		((uint32_t)ver.dev << 31u) | 
		((uint32_t)ver.maj << 24u) | 
		((uint32_t)ver.min << 16u) | 
		((uint32_t)ver.rev);
	*serialNum = 0xdef0u;
	return true;
}


coErrorCode appODWriteNotify(CoHandle ch, uint16 objIdx, uint8 subIdx, uint16 notificationGroup)
{
	coErrorCode err;
	
	if((objIdx == 0x5000u) && (subIdx == 0x00u)) {
		logd(LG_DBG, "CANopen command %d arg %d",
				ov.cmd & 0xff,
				ov.cmd >> 8);
	}

	if(objIdx == 0x2FFDu) {
		if(subIdx == 1u) {
			uint32_t cmd = coReadObject(ch, objIdx, subIdx, &err);

			switch(cmd) {

				case SWCMD_FULLSCAN:
					bci_full_scan(BATT_ID_MIN, BATT_ID_MAX);
					ov.swit.stat = SWCMD_FULLSCAN;
					break;
				case SWCMD_BRDOFF:
					device_state_cmd(DEVICE_CMD_OFF);
					break;
				case SWCMD_BRDON:
					device_state_cmd(DEVICE_CMD_ON);
					break;
				case SWCMD_BRDPRE:
					device_state_cmd(DEVICE_CMD_PRECHARGE);
					break;
				case SWCMD_BRDERROR:
					break;
				case SWCMD_BRDRESET:
					device_state_cmd(DEVICE_CMD_RESET);
					break;
				default:
					logd(LG_WRN, "Unhandled cmd %d\n", cmd);
					break;
			}
		}
	}

	/* Battery select */

	if(objIdx == 0x2FFEu) {
		if(subIdx == 1u) {

			uint8_t batt_id = (uint8_t)coReadObject(ch, objIdx, subIdx, &err);
			uint8_t scan = batt_id & 0x80u;
			batt_id &= 0x7Fu;

			if(scan != 0u) {
				enum batt_state state_list[BATT_ID_MAX+1u];
				rv r = bci_get_batt_state(state_list, BATT_ID_MAX);
				if(r == RV_OK) {
					uint8_t i;
					for(i=batt_id+1u; i<=BATT_ID_MAX; i++) {
						if(state_list[i] == BATT_STATE_AVAILABLE) {
							batt_id = i;
							break;
						}
					}
				}
			}
			ov.swbatt.node = batt_id;
			update_ov();
		}
	}

	/* Module info */

	if((objIdx == 0x2FF8u) && (subIdx == 2u)) {
		struct od_module *odm = &ov.module;
			
		const struct module *mod;
		rv r = module_get_info(odm->Index, &mod);
		if(r == RV_OK) {

			memset(ov.module.Name, ' ', sizeof(ov.module.Name));
			size_t l = strlen(mod->name);
			if(l > sizeof(ov.module.Name)) {
				l = sizeof(ov.module.Name);
			}
			memcpy(ov.module.Name, mod->name, l);

			odm->Flags = 255;
			odm->Options = 255;
			mod_read_ptr = 0;
		}
	}

	/* Bridging */

	if((objIdx == 0x2FFFu) && (subIdx == 1u)) {
		co_bridge_to(0, ov.bridge.node);
		if(ov.bridge.node != 0) {
			can_scanner_pause();
		} else {
			can_scanner_resume();
		}
	}

	//printd("%s %04x.%02x %d\n", __FUNCTION__, objIdx, subIdx, notificationGroup);
	return 0;
}


coErrorCode appODReadNotify(CH uint16 objIdx, uint8 subIdx, uint16 notificationGroup)
{
	coErrorCode ret = CO_OK;

	if((objIdx == 0x2FF8u) && (subIdx == 0x05u)) {
	
		/* Voodoo to send the module description to the Battery
		 * Monitor. Copy 8 bytes of the description into the od_module
		 * struct and advance pointer. Why does the Battery Monitor not
		 * simply use a regular CANopen segmented transfor for this
		 * string? */
		
		const struct module *mod;
		rv r = module_get_info(ov.module.Index, &mod);

		if(r == RV_OK) {
			if(mod_read_ptr < strlen(mod->description)) {
				size_t l = strlen(mod->description) - mod_read_ptr;
				if(l > 8u) {
					l = 8u;
				}
				memset(ov.module.Description, 32, 8lu);
				memcpy(ov.module.Description, &mod->description[mod_read_ptr], l);
				mod_read_ptr += 8u;
				ret = CO_OK;
			} else {
				ret = CO_ERR_GENERAL;
			}
		} else {
			ret = CO_ERR_GENERAL;
		}
	}

	return ret;
}


/*
 * End
 */
