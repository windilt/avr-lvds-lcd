//Auto-generated by makefile

#ifndef __PROJINFO_H__
#define __PROJINFO_H__
#include <inttypes.h>

#if (!defined(PROJINFO_SHORT) || !PROJINFO_SHORT)
 uint8_t __attribute__ ((progmem)) \
   header0[] = " /Users/meh/_avrProjects/LCDdirectLVDS/59-reallyCommon2 ";
 uint8_t __attribute__ ((progmem)) \
   header1[] = " Fri Dec 13 02:13:25 PST 2013 ";
 uint8_t __attribute__ ((progmem)) \
   headerOpt[] = " WDT_DIS=TRUE ";
#else //projInfo Shortened
 uint8_t __attribute__ ((progmem)) \
   header[] = "LCDdirectLVDS59 2013-12-13 02:13:25";
#endif

//For internal use...
//Currently only usable in main.c
#define PROJ_VER 59
#define COMPILE_YEAR 2013

#endif

