#ifndef bios_h
#define bios_h

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "config.h"
#include "plat.h"


/*
 * Basic types used in the bios
 */

typedef float voltage;     /* (U)  V  */
typedef float current;     /* (I)  A  */
typedef float temperature; /* (T)  K  */
typedef float resistance;  /* (R)  Ω  */
typedef float power;       /* (P)  W  */
typedef float charge;      /* (Q)  C = A⋅s */
typedef float energy;      /* (E)  J = W·s */

typedef enum {
	RV_OK,        /* No error */
	RV_ENOENT,    /* Not found */
	RV_EIO,       /* I/O error */
	RV_ENODEV,    /* No such device */
	RV_EIMPL,     /* Not implemented */
	RV_ETIMEOUT,  /* Timeout */
	RV_EBUSY,     /* Busy */
	RV_EINVAL,    /* Invalid argument */
	RV_ENOTREADY, /* Not ready */
	RV_ENOSPC,    /* No space left */
	RV_EPROTO,    /* Protocol error */
	RV_ENOMEM,    /* Out of memory */
} rv;

/*
 * Hardware
 */

void bios_init(void);
const char *rv_str(rv r);

#endif
