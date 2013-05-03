
#include "controller.h"
#include "lm3s1968.h"
#include "inc/hw_types.h"
#include "sysctl.h"

void Controller_Init(void){
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  GPIO_PORTG_DIR_R &= ~0xF8;
	GPIO_PORTG_PUR_R |= 0xF8;
  GPIO_PORTG_AFSEL_R &= ~0xFC;
  GPIO_PORTG_DEN_R |= 0x0FC;
	GPIO_PORTA_DIR_R &= ~0x01;
	GPIO_PORTA_PDR_R |= 0x01;
	GPIO_PORTA_AFSEL_R &= ~0x01;
	GPIO_PORTA_DEN_R |= 0x01;
}

unsigned long get_buttons(void)
{
	return (((~GPIO_PORTG_DATA_R & 0xF8) >> 3) |
		((~GPIO_PORTA_DATA_R & 0x01) << 5));
}
