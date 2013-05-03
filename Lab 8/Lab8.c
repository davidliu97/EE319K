// These are the testing files for Lab 8

#include "hw_types.h"
#include "pll.h"
#include "adc.h"
#include "LCD.h"
#include "SysTick.h"

#define N 256

unsigned long Convert(unsigned long);
void WaitForInterrupt(void);
unsigned long Data;

//unsigned long Data;
int main1(void){
  PLL_Init();         // Bus clock is 50 MHz 
  ADC_Init(2);        // turn on ADC, set channel to 2, sequencer 3
  while(1){                
    Data = ADC_In();  // sample 10-bit channel 2
  }
}


//unsigned long Data;
int main2(void){ int i; unsigned long sum;
  PLL_Init();              // Bus clock is 50 MHz 
  LCD_Open();   
  LCD_Clear();
  ADC_Init(2);             // turn on ADC, set channel to 2, sequencer 3
  while(1){  
    sum = 0;      
    for(i=0; i<N; i++){    // take N samples and perform the average
      sum = sum+ADC_In();  // sample 10-bit channel 2
    }
    Data = sum/N;          // noise reducing filter
    LCD_GoTo(0);
    LCD_OutFix(Data); LCD_OutString("    "); 
  }
}


//unsigned long Data;      // 10-bit ADC
unsigned long Position;  // 16-bit fixed-point 0.001 cm
#define N 256
int main3(void){ int i; unsigned long sum;
  PLL_Init();         // Bus clock is 50 MHz 
  LCD_Open();   
  LCD_Clear();
  ADC_Init(2);        // turn on ADC, set channel to 2, sequencer 3
  while(1){  
    sum = 0;      
    for(i=0; i<N; i++){        // take N samples and perform the average
      sum = sum+ADC_In();      // sample 10-bit channel 2
    }
    Data = sum/N;	           // noise reducing filter
    Position = Convert(Data);  // you write this function
    LCD_GoTo(0);
    LCD_OutFix(Position);
  }
}


int main(void)
{
  PLL_Init();         // Bus clock is 50 MHz 
  LCD_Open();   
  LCD_Clear();
	SysTick_Init(2000000);
  ADC_Init(2);        // turn on ADC, set channel to 2, sequencer 3
	while(1)
	{
    Position = Convert(Data);  // you write this function
    LCD_GoTo(0);
    LCD_OutFix(Position);
		WaitForInterrupt();
	}
}

void SysTick_Handler(void)
{
	int i, sum = 0;      
  for(i=0; i<N; i++)        // take N samples and perform the average
		sum = sum+ADC_In();      // sample 10-bit channel 2
	Data = sum/N;	           // noise reducing filter
}

unsigned long Convert(unsigned long pre) {
	// Min is 2, Max is 949
	return 7*(pre-2)/4;
}
// write an actual main to handle interrupts and mail box


