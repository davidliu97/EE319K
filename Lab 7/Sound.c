
#include "dac.h"
#include "Sound.h"
#include "SysTickInts.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"

#define GPIO_PORTD0             (*((volatile unsigned long *)0x40007004))

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value

int count;	// Index to sine.
short inc;	// Flag to disable incrementing count.

const unsigned char sine[32] = {
	128,153,177,199,218,234,245,253,255,253,
	245,234,218,199,177,153,128,103,79,57,38,
	22,11,3,1,3,11,22,38,57,79,103
};
/*const unsigned long freqs[] = {
	5972, 5637, 5320, 5022,
	4740, 4474, 4223, 3985,
	3762, 3551, 3351, 3164,
	2986, 2818, 2660, 2511,
	2370, 2237, 2111, 1993,
	1881, 1775, 1675, 1581,
	1493
};*/
const unsigned int freqs[] = {
//	6164,
	5818, 5492, 5184, 4893,
	4618, 4359, 4114, 3883,
	3665, 3459, 3265, 3082,
	2909, 2746, 2592, 2446,
	2309, 2179, 2057, 1941,
	1832, 1729, 1632, 1541
};

__asm void wait1ms(void){
	LDR R0, =0x411A;
busy_loop;
	SUBS R0, R0, #0x01;
	BNE busy_loop;
	BX LR;
}

void waitsecs(unsigned long dur){
	int i;
	for (i = 0; i < dur; i++)
		wait1ms();
}

void Sound_Init(void){
	count = 0;
	SysTick_Init(50000);
	EnableInterrupts();
	DAC_Init();
}

void Sound_Play(int note){
	if (note == Rest) inc = 0;
	else
	{
		inc = 1;
		SysTick_SetReload(freqs[note]);
	}
}

void Sound_PlaySong(
	const note *song, 
	long len, int offset, 
	unsigned long wait)
{
	int i;
	note *songptr = (note*) song;
	for (i = 0; i < len; i++)
	{
		Sound_Play((*songptr).freq+O*offset);
		waitsecs((*songptr).length*wait);
		songptr++;
	}
}

// Interrupt service routine
// Executed every 20ns*(period)
void SysTick_Handler(void){
  GPIO_PORTD0 ^= 0x01;        // toggle PD0
	if (count >= 32) count = 0;
	DAC_Out(sine[count]);
	count += inc;
	return;
}
