/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */





#include <util/delay.h> //for delay_us, etc...






//delay_cyc intends to create "functions" that cause delays of a specific
//number of CPU cycles. This isn't by any means highly accurate...

//There are a couple compilation-options:
//  one uses _delay_loop_2 from <util/delay_basic.h> (this is newer)
//  the other is my original hokey implementation.

//The original "hokey" implementation was before I was familiar with
//assembly-inlining. Though, it makes use of gcc's optimizer to weed-out
//the unused code WHEN each function-call has a constant argument.
//Unfortunately, this means that different versions of C/gcc may compile
//these delays differently. In *my* configuration, it seems to work well,
//but no promises, here.
// 'avr-gcc -v' gives:
//Target: avr
//Configured with: ../gcc-4.4.5/configure --target=avr --enable-languages=c
//--disable-libssp
//Thread model: single
//gcc version 4.4.5 (GCC) 

// ALSO, there are *definitely* cases where it's called with VARIABLES as
// arguments, as opposed to constant, especially BLUE_DIAG_BAR(_SCROLL).
// In which case, things are pretty hokey.
// Both compilation-options work, in my testing, but not perfectly.

//(A more accurate method has since been developed in
// _commonCode/delay_cyc/, which can create a delay of exactly 1->127 
// cycles, if set-up ahead of time, even with a variable as an argument.
// This method is to be called via asm, and hasn't been implemented here).

// So, again, in cases here with a CONSTANT argument:
//  initial set-up math should optimize-out, and we'll be left with either
//  a specific number of loops, or a specific number of nops, inlined with
//  the original caller.
//  With _delay_loop_2() this results in a number of cycles rounded-up to
//  the nearest 4. (e.g. delay_cyc(3) results in 4 cycles. delay_cyc(13)
//  results in 16 cycles).
//  With the hokey method, it might very well result in exactly the number
//  of requested cycles, between a potential loop *and* remaining nops...
// And with a VARIABLE argument:
//  With _delay_loop_2() there's a bit of set-up overhead, which may in
//  fact be quite large (division by 4 => Shift-Rights. + an addition
//  + negative-argument testing). And then there's the number of 4-cycle
//  loops, rounded-up.
//  With the "hokey" method, it's probably all messed-up, a bunch of
//  switch-statements, set-up, and more... lots of jumping, etc.

// As it stands, this may result in row-timing (H-Blanks + DE) that aren't
// constant and/or aren't exactly as requested...
// This *may* affect some display's sync-ability.
// So far, it hasn't been found to be a problem, except visually where,
// e.g. in BLUE_DIAG_BAR(_SCROLL) the bar is not perfectly diagonal, more
// stair-stepped (or even slightly jagged, in the "hokey" case).


#include "delay_cyc.h"

//This'll optimze-out in some cases.
// The timings and instructions used are probably specific to the
// instruction-set in the MCU, the GCC version, optimization-level, etc.
// a/o LCDdirectLVDS11 HLow_delay() this is how it's compiling...

// The actual number of cycs will probably be a few higher...
// (or who knows, if things optimze-out, e.g. small ~0-8 numCycs)
// Due to init, and division...
// Using rounding-up as well...

// the argument is int32_t to allow for negative value testing
// BUT: numCyc is only tested against a uint16_t...
//  the greatest value (?) is (UINT16_MAX - 7)
// (maybe it'd make more sense to do the math outside the delay_cyc call
// and allow the preprocessor to change it to 0, instead of doing it here.

//This should probably be reimplemented using _delay_loop_1/2() from 
// util/delay_basic.h
//  1 uses three cycles per count, counts from 1-256, 
//      256 counts: _delay_loop_1(0)
//  2 uses four, counts from 1-65536, 65536 is passed as 0

//#define DELAY_CYC_DELAY_LOOP TRUE//FALSE

#if(defined(DELAY_CYC_DELAY_LOOP) && DELAY_CYC_DELAY_LOOP)
#warning "This has only been tested with v54+... rowSegBuffer, etc."
void delay_cyc(int32_t numCyc)
{
   if(numCyc <= 0)
      return;
#define V6651_delay_cyc
//Also, numCyc+3/4 is being converted to >> and GCC warns that it's
//assuming no signed-overflow occurs... So, for now, we're using v66.51's
#ifndef V6651_delay_cyc
   uint16_t numLoops = (numCyc+3)/4;

   _delay_loop_2(numLoops);
#else
	//In v66.51, this was modified as follows...
	// It's probably unnecessary (and likely why v91 is dang-near
	// perfectly-stable in terms of pixel-jitter)
	// But here it is, anyhow.
	// Actually, on the BOE display, it seems to have no effect...
	// Though, certainly, it takes more instructions to run...
	int32_t numLoops = (numCyc+3)>>2;

	while(numLoops > 65536)
	{
		_delay_loop_2(0);
		numLoops -= 65536;
	}

	if(numLoops > 0)
	{
		_delay_loop_2(numLoops);
	}
#endif
/*   uint32_t delayLoops = (numCyc+3)/4;

   do
   {
      delayLoops--;
//      _delay_loop_2(delayLoops&0xffff);

//      delayLoops -= (delayLoops&0xffff);
   } while(delayLoops);
*/
/*
   //Since this is inline, AND it's only called with values computed
   // at compile time, only one of these should be compiled in...
   // as necessary...
   // If called without precomputed value (i.e. a variable)????
   // Maybe I should only use delay_loop_2...
   if(numCyc < 256*3)
      _delay_loop_1(numCyc/&0xff);

   else
*/
}
//Also from v66.51, but also commented-out there...
/* This only works when numCyc is constant
#elif __HAS_DELAY_CYCLES && defined(__OPTIMIZE__)
extern void __builtin_avr_delay_cycles(unsigned long);

void delay_cyc(int32_t numCyc)
{
   __builtin_avr_delay_cycles(numCyc);
}
*/



#else
void delay_cyc(int32_t numCyc)
{

   //This shouldn't happen often, but some delay_cyc() calls use math
   // to calculate the number of cycles, and it could be negative
   if(numCyc < 0)
      return;
                                 //#Clocks
   //Two instructions:
   // ldi r24, 0                  //1          i=0
   // ldi r25, 0                  //1
   uint16_t i;


   //Loop instructions:
   //nop                           //1
   //adiw  r24, 0x01   ; 1         //2          i++
   //cpi   r24, 0x77   ; 119      //1          i<numCyc (119 in this case)
   //cpc   r25, r1               //1          Apparently it's also testing
                                 //           the high byte is 0
   //brne  .-10        ;         //"1/2"      return to nop
                                 //            if I understand, this is two
                                 //            cyc when branching
                                 //            or one if not (when complete)

   //READ THIS:
   // Current Compilation Settings: A/O v18:
   //    THIS LOOP WILL BE UNROLLED if numLoops <= 5!!!
   //    Which then turns into numLoops*2 cycles (instead of numLoops*8)
   // 
   // Here's an attempted hack...
   //  it *should* optimize the test out in either case, so it's like a 
   //  preprocessing directive...
	uint8_t loopRemainder = numCyc & 0x07;
	uint16_t numLoops = (((uint16_t)(numCyc)+7)>>3);


   // HACK ATTEMPT 2: THATS A LOT OF CODE.
      switch(numLoops)
      {
         case 5:
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
         case 4:
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
         case 3:
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
         case 2:
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
         case 1:
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            return;
            break;
         default:
            break;
      }
   // THUS:
   // Each loop is 7 cycles, make it 8 by adding an extra nop and we can
   // use >> instead of / for calculations...
   // +7 assures rounding-up...
#warning "This loop seems to be optimizing out!"
   //for(i=0; i<((numCyc+7)>>3); i++)
   for(i=0; i<numLoops; i++)
   {
      //THIS IS A HACK DUE TO OPTIMIZATION, see above
      // It will NOT likely be happy with different versions of gcc...
   /*   // NOGO: Apparently it won't expand the loop if this is part of it
      // so then we have 5 loops AND 8 instructions /within/ the loop
      // AND the comparison overhead!`
      if(numLoops <= 5)
      {
         asm("nop");
         asm("nop");
      //   asm("nop");
      //   asm("nop");
      //   asm("nop");
      //   asm("nop");
      }
      else
      {
         asm("nop");
         asm("nop");
      }
   */
      //Apparently this loop will optimize-out without this:
      // Obviously, one instruction each...
      asm("nop");
      asm("nop");
   }

	//This addition a/o v62-20
	// When delay_cyc is called with a constant-value, it should optimize
	// out to merely the number of nops... but if it's non-constant, then
	// this will slow things a bit... UNTESTED.
	// (Actually, briefly tested with BLUE_DIAG_BAR_SCROLL and seems to
	// work, but not highly precise, as expected.
	// In Other Words: It worked better in BLUE_DIAG_BAR_SCROLL withOUT the
	// following addition, because this function is then called with a
	// variable, instead of a constant, and it does not optimize out.)

	//Realistically, this should probably be done in assembly, the whole
	//thing...
	// but doing-so might make optimization more difficult...?
	switch(loopRemainder)
	{
		case 7:
			asm("nop");
		case 6:
			asm("nop");
		case 5:
			asm("nop");
		case 4:
			asm("nop");
		case 3:
			asm("nop");
		case 2:
			asm("nop");
		case 1:
			asm("nop");
		default:
			break;
	}

}
#endif
/* mehPL:
 *    I would love to believe in a world where licensing shouldn't be
 *    necessary; where people would respect others' work and wishes, 
 *    and give credit where it's due. 
 *    A world where those who find people's work useful would at least 
 *    send positive vibes--if not an email.
 *    A world where we wouldn't have to think about the potential
 *    legal-loopholes that others may take advantage of.
 *
 *    Until that world exists:
 *
 *    This software and associated hardware design is free to use,
 *    modify, and even redistribute, etc. with only a few exceptions
 *    I've thought-up as-yet (this list may be appended-to, hopefully it
 *    doesn't have to be):
 * 
 *    1) Please do not change/remove this licensing info.
 *    2) Please do not change/remove others' credit/licensing/copyright 
 *         info, where noted. 
 *    3) If you find yourself profiting from my work, please send me a
 *         beer, a trinket, or cash is always handy as well.
 *         (Please be considerate. E.G. if you've reposted my work on a
 *          revenue-making (ad-based) website, please think of the
 *          years and years of hard work that went into this!)
 *    4) If you *intend* to profit from my work, you must get my
 *         permission, first. 
 *    5) No permission is given for my work to be used in Military, NSA,
 *         or other creepy-ass purposes. No exceptions. And if there's 
 *         any question in your mind as to whether your project qualifies
 *         under this category, you must get my explicit permission.
 *
 *    The open-sourced project this originated from is ~98% the work of
 *    the original author, except where otherwise noted.
 *    That includes the "commonCode" and makefiles.
 *    Thanks, of course, should be given to those who worked on the tools
 *    I've used: avr-dude, avr-gcc, gnu-make, vim, usb-tiny, and 
 *    I'm certain many others. 
 *    And, as well, to the countless coders who've taken time to post
 *    solutions to issues I couldn't solve, all over the internets.
 *
 *
 *    I'd love to hear of how this is being used, suggestions for
 *    improvements, etc!
 *         
 *    The creator of the original code and original hardware can be
 *    contacted at:
 *
 *        EricWazHung At Gmail Dotcom
 *
 *    This code's origin (and latest versions) can be found at:
 *
 *        https://code.google.com/u/ericwazhung/
 *
 *    The site associated with the original open-sourced project is at:
 *
 *        https://sites.google.com/site/geekattempts/
 *
 *    If any of that ever changes, I will be sure to note it here, 
 *    and add a link at the pages above.
 *
 * This license added to the original file located at:
 * /Users/meh/_avrProjects/LCDdirectLVDS/93-checkingProcessAgain/delay_cyc.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
