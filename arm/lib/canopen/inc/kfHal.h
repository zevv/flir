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
** Hardware dependent functions except CAN.
*/

#ifndef _KF_HAL_H_
#define _KF_HAL_H_

#include "kfCommon.h"

#ifdef __WIN32__
  #define TIMERFREQ 1000 /* Approx timer frequency in Hz */
#elif defined __MPC555__
/* This a copy of the TIMER_VALUE from the ThreadX tx_ill.pcc
 * It it loaded into the decrementer register of the MPC555
 * which triggers a timer interrupt when it reaches zero */ 
/*  #define MPC555_DECREMENTER_VALUE 0x00007FFF*/
/*  #define TIMERFREQ (SYS_DECTICKS_PERSEC / MPC555_DECREMENTER_VALUE)*/
#elif defined __uC_OS_II
  #define TIMERFREQ 1000
#elif defined __M16C
  #define TIMERFREQ 1000
#else
  #error Timer can not be supported
#endif
typedef unsigned long KfHalTimer;

/* --- Function prototypes */
#ifdef __cplusplus
extern "C" {
#endif

void kfhalTimerSetup(void);
KfHalTimer kfhalReadTimer(void);

#if TIMERFREQ > 0xffff
  /* We must divide in this order to avoid overflow.
   * We will loose some precision, but it is acceptable for the moment.
   * In the future, we might introduce more cases. */
  #define kfhalMsToTicks(ms) ((ulong)(ms)*(TIMERFREQ/1000))
  #define kfhalTicksToMs(ticks) ((ticks)/(TIMERFREQ/1000))
#elif TIMERFREQ < 1000
  /*
  *  Make sure there is at least one tick so we don't get overloads
  */
  #define kfhalMsToTicks(ms) ((ms < 1000/TIMERFREQ)? 1: ((ulong)(ms)*TIMERFREQ)/1000)
  #define kfhalTicksToMs(ticks) ((ticks)*1000L/TIMERFREQ)
#else
  #define kfhalMsToTicks(ms) (((ulong)(ms)*TIMERFREQ)/1000)
  #define kfhalTicksToMs(ticks) ((ticks)*1000L/TIMERFREQ)
#endif
#define kfhalUsToTicks(us)    kfhalMsToTicks((us)/1000)
#define kfhalTicksToUs(ticks) (1000L*kfhalTicksToMs(ticks))


/* Give up the current time slice if running in a multi-tasking environment.*/
void kfhalDoze(void);

/* Reset the CPU. */
void kfhalReset (void);

#ifdef __cplusplus
}
#endif

#endif /* _KF_HAL_H_ */
