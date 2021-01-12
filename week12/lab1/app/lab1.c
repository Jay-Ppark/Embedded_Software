#include "includes.h"

#define F_CPU	16000000UL	// CPU frequency = 16 Mhz
#include <avr/io.h>	
#include <util/delay.h>

#define  TASK_STK_SIZE  OS_TASK_DEF_STK_SIZE            

OS_STK          LedTaskStk[TASK_STK_SIZE];
unsigned char value = 128;
unsigned char way = 1;
void  LedTask(void *data);


int main (void)
{
  OSInit();

  OS_ENTER_CRITICAL();
  TCCR0=0x07;  
  TIMSK=_BV(TOIE0);                  
  TCNT0=256-(CPU_CLOCK_HZ/OS_TICKS_PER_SEC/ 1024);   
  OS_EXIT_CRITICAL();
  
  OSTaskCreate(LedTask, (void *)0, (void *)&LedTaskStk[TASK_STK_SIZE - 1], 0);

  OSStart();                         
  
  return 0;
}


void LedTask (void *data)
{
  DDRA = 0xff;
  data = data;    
  // LED Task
  for (;;) {
      PORTA = value;
      _delay_ms(200);
      if (way == 1)
          value >>= 1;
      else
          value <<= 1;
      if (value == 0) {
          way = 0;
          value = 1;
      }
      if (value == 128)
          way = 1;
  }
}
