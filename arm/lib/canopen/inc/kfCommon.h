/*
**                        Copyright 2001-2003 by KVASER AB
**                    P.O Box 4076 SE-51104 KINNAHULT, SWEDEN
**
**                     Copyright 2004-2014 by TK Engineering Oy
**                       P.O. Box 810, FI-65101 Vaasa, FINLAND
**                     E-mail: info@tke.fi, Web: http://www.tke.fi
**
** This software is furnished under a license and may be used and copied
** only in accordance with the terms of such license.
**
**
** Common definitions: typedef's etc.
*/

#ifndef __KFCOMMON_H
#define __KFCOMMON_H

#include <stdint.h>

#define __M16C 1

//#define DEBUG 0   /* If defined, debug printouts etc are included. */

#define STATIC static  /* We use STATIC for temporary static variables (can be viewed in the watch window). */

#undef BIG_ENDIAN
#undef LITTLE_ENDIAN

#ifndef BIG_ENDIAN
#ifndef LITTLE_ENDIAN
#ifdef WIN32
  #define BIG_ENDIAN 0
  #define LITTLE_ENDIAN 1
#else
  #define BIG_ENDIAN 0
  #define LITTLE_ENDIAN 1
#endif
#endif
#endif

#if BIG_ENDIAN
#if LITTLE_ENDIAN
#error Both BIG_ENDIAN and LITTLE_ENDIAN are set
#endif
#endif

/* Architecture dependencies */
#ifdef __MPC555__
#define DEFAULT_LETTER_BTR  0x5c0f // 125 kBit/s for 40 MHz clock
#else
#define DEFAULT_LETTER_BTR  0x2307 // 125 kBit/s for a 82527
#endif
// qqq was defdocBtr


/* An assert that is evaluated during compilation (and uses no code space).
*  If the expression is zero, the compiler will warn that the vector
*  _Dummy[] has zero elements, otherwise it is silent.
*
* There will never be a warning from lint as error 85 only occurs if the
* 'extern' keyword is removed.
*
*  Lint warning 506 is "Constant value Boolean",
* 762 is "Redundantly declared ... previously declared at line ..."
* Warning 752 is "local declarator 'Symbol' (Location) not referenced", it can't
* be placed inside save/restore as the warning appears at the wrap-up.
* 778: Constant expression evaluates to 0 in operation '&'
*/
#define CompilerAssert(e)     \
  /*lint -esym(752,_Dummy) -save -e506 -e762 -e778*/ \
  extern char _Dummy[(e)?1:0] \
  /*lint -restore */

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned char uchar;


#ifdef __MPC555__

#include <mpc555/mpc555.h>
#else
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;

#ifndef false
#ifndef __cplusplus
typedef enum {false = 0, true = 1} bool;
#endif
#endif
#endif

/* Bithandling, bits by number (0, 1, 2, 3, ...) */
#define   checkbit(var,bit)  (var & (0x01 << (bit)))
#define   setbit(var,bit)    (var |= (0x01 << (bit)))
#define   togglebit(var,bit) (var ^= (0x01 << (bit)))
#define   clrbit(var,bit)    (var &= (~(0x01 << (bit))))

/* Bithandling, bitmasks (1, 2, 4, 8, ...) */
#define checkBM(var, bm) ((var) & (bm))
#define setBM(var, bm) ((var) |= bm)
#define clrBM(var, bm) ((var) &= ~(bm))
/*#define assignBM(var, bm, f) ((var) = ((var) & ~(bm)) | (f ? bm : 0)) */
#define assignBM(var, bm, f) {if (f) setBM(var, bm) else clrBM(var,bm)}

#define BM0 0x01
#define BM1 0x02
#define BM2 0x04
#define BM3 0x08
#define BM4 0x10
#define BM5 0x20
#define BM6 0x40
#define BM7 0x80



#ifndef min
 #define min(a,b) (a <= b ? a : b)
 #define max(a,b) (a >= b ? a : b)
#endif

typedef void(*funPtr)(void);
/* typedef void interrupt (*ifunPtr)(void); */

#define MSB(b) ((uchar)(((ushort)(b) >> 8) & 255))
#define LSB(b) ((uchar)((ushort)(b) & 255))
#define mkSHORT(b0,b1) (short)((ushort)(uchar)(b0)+((ushort)(uchar)(b1)<<8))
#define mkUSHORT(b0,b1) (ushort)((ushort)(uchar)(b0)+((ushort)(uchar)(b1)<<8))
#define mkULONG(b0,b1,b2,b3) ((ulong)(b0)+((ulong)(b1)<<8)+((ulong)(b2)<<16)+\
                             ((ulong)(b3)<<24))


#ifdef __cplusplus
extern "C" {
#endif
/* The cPrintf function is called by many HLPs, drivers etc for debug and
 * informational printouts. Should work as a normal printf(), but can be
 * adapted to send its output to another place (such as a console window in a
 * Windows program or a log file), made smaller by supporting only a subset of
 * what printf doees etc.
 * In small targets (or in the released code) it might be implemented as a
 * dummy function.
 * As it takes a variable number of arguments, it can't be defined to do
 * nothing. In this case, one would need to use the CPRINTF()-macro. */  
//void cPrintf(const char *fmt, ...);
/* Call this macro as 'CPRINTF(("Error %d\n", errCode));', and by redefining
* CPRINTF as on the second line, the call can med removed alltogether. */
//#define CPRINTF(a) cPrintf a
/* #define CPRINTF((a)) (void)0 */

#ifdef __cplusplus
}
#endif

#endif /* __KFCOMMON_H */
