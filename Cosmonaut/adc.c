
#include "lm3s1968.h"

// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// Max sample rate: <=125,000 samples/second
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: software trigger
// SS3 1st sample source: programmable using variable 'channelNum' [0:7]
// SS3 interrupts: enabled but not promoted to controller
void ADC_Init(unsigned char channelNum){
  if(channelNum > 7){
    return;   // 0 to 7 are valid channels on the LM3S1968
  }
  SYSCTL_RCGC0_R |= 0x00010000;   // 1) activate ADC
  SYSCTL_RCGC0_R &= ~0x00000300;  // 2) configure for 125K
  ADC_SSPRI_R = 0x3210;           // 3) Sequencer 3 is lowest priority
  ADC_ACTSS_R &= ~0x0008;         // 4) disable sample sequencer 3
  ADC_EMUX_R &= ~0xF000;          // 5) seq3 is software trigger
  ADC_SSMUX3_R &= ~0x0007;        // 6) clear SS3 field
  ADC_SSMUX3_R += channelNum;     //    set channel
  ADC_SSCTL3_R = 0x0006;          // 7) no TS0 D0, yes IE0 END0
  ADC_IM_R &= ~0x0008;            // 8) disable SS3 interrupts
  ADC_ACTSS_R |= 0x0008;          // 9) enable sample sequencer 3
}

//------------ADC_InSeq3------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 10-bit result of ADC conversion
unsigned long ADC_In(void){  unsigned long result;
  ADC_PSSI_R = 0x0008;             // 1) initiate SS3
  while((ADC_RIS_R&0x08)==0){};    // 2) wait for conversion done
  result = ADC_SSFIFO3_R&0x3FF;    // 3) read result
  ADC_ISC_R = 0x0008;              // 4) acknowledge completion
  return result;
}
