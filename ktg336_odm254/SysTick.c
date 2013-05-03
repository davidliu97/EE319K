// SysTickInts.c
// Runs on LM3S1968
// Use the SysTick timer to request interrupts at a particular period.
// Daniel Valvano
// June 27, 2011

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to the Arm Cortex M3",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2011

   Program 5.12, section 5.7

 Copyright 2011 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// oscilloscope or LED connected to PD0 for period measurement
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "lm3s1968.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
volatile unsigned long Counts = 0;

// **************SysTick_Init*********************
// Initialize Systick periodic interrupts
// Input: interrupt period
//        Units of period are 20ns
//        Maximum is 2^24-1
//        Minimum is determined by lenght of ISR
// Output: none
void SysTick_Init(unsigned long period){
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOG; // activate port D
  Counts = 1;
	Counts = 0;
  GPIO_PORTG_DIR_R |= 0x04;   // make PG2 out
	GPIO_PORTG_AFSEL_R &= ~0x04;
	GPIO_PORTG_PUR_R |= 0x04;
  GPIO_PORTG_DEN_R |= 0x04;   // enable digital I/O on PG2
	GPIO_PORTG_DATA_R |= 0x04;
	
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
	if (period) {
		NVIC_ST_RELOAD_R = period-1;// reload value
		NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x50000000; // priority 2
                              // enable SysTick with core clock and interrupts
		NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC+NVIC_ST_CTRL_INTEN;
	}
	else 
	{
		NVIC_ST_RELOAD_R = 0x00FFFFFF;
		NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC;
	}
}

void SysTick_Wait1ms(void)
{
	unsigned long start = NVIC_ST_CURRENT_R, 
		current = NVIC_ST_CURRENT_R;
	while (((start - current) & 0x00FFFFFF) < 50000)
		current = NVIC_ST_CURRENT_R;
}

void SysTick_Wait(unsigned long duration)
{
	for (; duration > 0; duration--)
		SysTick_Wait1ms();
}
