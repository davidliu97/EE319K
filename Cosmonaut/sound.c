#include "dac.h"
#include "lm3s1968.h"

const unsigned char testsound[100]= {0};
void EnableInterrupts(void);

short playflag, repeat;
unsigned char *startpt, *endpt, *playpointer;

void Timer0A_Init(unsigned long period) {
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0; // ACTIVATE TIMER0A
	period++;
	period -= 2;
	TIMER0_CTL_R &= ~0x00000001;
	TIMER0_CFG_R = 0x00000004;
	TIMER0_TAMR_R = 0x00000002;
	TIMER0_TAILR_R = period;									// Period
	TIMER0_TAPR_R = 0;
	TIMER0_ICR_R = 0x00000001;
	TIMER0_IMR_R |= 0x00000001;
	NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x20000000;
	NVIC_EN0_R |= NVIC_EN0_INT19;
	TIMER0_CTL_R |= 0x00000001;
	playflag = 0;
	EnableInterrupts();
}

void Timer0A_Handler(void) {
	TIMER0_ICR_R = TIMER_ICR_TATOCINT;
	if (playflag)
		DAC_Out(*(playpointer++));
	if (playpointer == endpt) {
		if (repeat) playpointer = startpt;
		else playflag = 0;
	}
}

void playWAV(unsigned char * song, 
	unsigned int length, unsigned int rpt)
{
	startpt = playpointer = song;
	endpt = song + length;
	playflag = 1;
	repeat = rpt;
}

void Sound_Init() {
	// Kevin is a bitch.
	// Omar is awesome.
	// Alex is sort of next to us.
	// Yinjie is a bitch.
	DAC_Init();
	Timer0A_Init(4535);
}
