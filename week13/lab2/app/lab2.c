#include "includes.h"

#define F_CPU	16000000UL	// CPU frequency = 16 Mhz
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>

#define  TASK_STK_SIZE  OS_TASK_DEF_STK_SIZE
#define  N_TASKS        5

OS_STK       TaskStk[N_TASKS][TASK_STK_SIZE];
OS_EVENT *Mbox;
OS_EVENT *current_temp_sem;
OS_EVENT* set_temp_sem;
OS_EVENT *finish_set_temp_grp;
OS_EVENT *MsgQueue;

const uc digit[10] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f };
const uc fnd_sel[4] = { 0x08,0x04,0x02,0x01 };
const uc led_v[8] = { 1,2,4,8,16,32,64,128 };
volatile INT8U	FndNum;

//Function
int ReadTemperature();
void InitI2C();
void DisplaySetTemp();
//Task
void TemperatureTask(void *data);
void FndDisplayTask(void *data);
void StartHeatTask(void* data);
void LEDTask(void* data);
void StopHeatTask(void *data);

violate int working = 0; // 0이면 작동 전, 1이면 작동 중
violate int set_temp = 25;
violate int current_temp;

INT8U err;

int main (void)
{
  OSInit();
  OS_ENTER_CRITICAL();
  TCCR0 = 0x07;
  TIMSK = _BV(TOIE0);
  TCNT0 = 256 - (CPU_CLOCK_HZ / OS_TICKS_PER_SEC / 1024);
  OS_EXIT_CRITICAL();
  //Event 객체 초기화
  current_temp_sem = OSSemCreate(1);
  set_temp_sem = OSSemCreate(1);
  finish_set_temp_grp = OSFlagCreate(0x00, &err);

  OSTaskCreate(TemperatureTask, (void *)0, (void *)&TaskStk[0][TASK_STK_SIZE - 1], 0);
  OSTaskCreate(FndDisplayTask, (void *)0, (void *)&TaskStk[1][TASK_STK_SIZE - 1], 2);
  OSTaskCreate(StartHeatTask, (void*)0, (void*)&TaskStk[2][TASK_STK_SIZE - 1], 2);
  OSTaskCreate(LEDTask, (void*)0, (void*)&TaskStk[3][TASK_STK_SIZE - 1], 2);
  OSTaskCreate(StopHeatTask, (void*)0, (void*)&TaskStk[4][TASK_STK_SIZE - 1], 2);

  OSStart();

  return 0;
}
//버튼 4를 눌렀을 경우 희망온도 올라감 35도를 넘어가면 원래 default 온도로
//default 온도는 25
ISR(INT4_vect) {
	if(working == 1) {
		OSSemPend(set_temp_sem, 0, &err);
		if (set_temp == 35) {
			set_temp = 25;
		}
		else {
			set_temp++;
		}
		OSSemPost(set_temp_sem);
	}
}
//버튼 5를 눌렀을 경우 히터 작동
ISR(INT5_vect) {
	if (working == 0) {
		OSSemPend(current_temp_sem, 0, &err);
		current_temp = result / 10 * 10;
		current_temp = current_temp + (result % 10 * 10);
		OSSemPost(current_temp_sem, 0, &err);
		if (current_temp < set_temp) {
			working = 1;
		}
	}
}
int ReadTemperature()
{
	int value;

	TWCR = _BV(TWSTA) | _BV(TWINT) | _BV(TWEN);
	while (!(TWCR & _BV(TWINT)));

	TWDR = 0x98 + 1; //TEMP_I2C_ADDR + 1
	TWCR = _BV(TWINT) | _BV(TWEN);
	while (!(TWCR & _BV(TWINT)));

	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
	while (!(TWCR & _BV(TWINT)));

	//온도센서는 16bit 기준으로 값을 가져오므로
	//8비트씩 2번을 받아야 한다.
	value = TWDR << 8;
	TWCR = _BV(TWINT) | _BV(TWEN);
	while (!(TWCR & _BV(TWINT)));

	value |= TWDR;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);

	value >>= 8;

	TIMSK = (value >= 33) ? TIMSK | _BV(TOIE2) : TIMSK & ~_BV(TOIE2);

	return value;
}

void InitI2C()
{
    PORTD = 3; 						// For Pull-up override value
    SFIOR &= ~(1 << PUD); 			// PUD
    TWSR = 0; 						// TWPS0 = 0, TWPS1 = 0
    TWBR = 32;						// for 100  K Hz bus clock
	TWCR = _BV(TWEA) | _BV(TWEN);	// TWEA = Ack pulse is generated
									// TWEN = TWI 동작을 가능하게 한다
}

void DisplaySetTemp(void) {
	//이미 이 함수를 사용하기 위해서는 semaphore를 통해서만 사용가능 하기 때문에
	//set_temp를 사용해도 semaphore를 따로 추가하지 않음
	if(working == 1)
}
void TemperatureTask (void *data)
{
	INT8U err;
    data = data;
    InitI2C();

    write_twi_1byte_nopreset(ATS75_CONFIG_REG, 0x00); // 9비트, Normal
    write_twi_0byte_nopreset(ATS75_TEMP_REG);
    while (1)  {
		OSSemPend(Current_Temp_sem, 0, &err);
		current_temp = ReadTemperature();
		OSSemPost(Current_Temp_sem);
		//온도를 설정하는 부분
		while (working == 0) {
			DisplaySetTemp();
		}
		OSMboxPost(Mbox, (void*)&value);
		OSTimeDly(100);
  }
}

void FndDisplayTask (void *data)
{
	INT8U err;
	INT8U result;
	data = data;

	// 작성
    DDRC = 0xff;
    DDRG = 0x0f;

    while(1)  {
		FndNum = *(INT8U*)OSMboxPend(Mbox, 0, &err);

		OSTimeDly(100);
		result = FndNum;

		PORTC = FND_DATA[result % 10];
		PORTG = 0x01;
		_delay_ms(2);
		PORTC = FND_DATA[result / 10];
		PORTG = 0x02;
		_delay_ms(2);
    }
}
void StartHeatTask(void* data) {

}
void LEDTask(void* data) {

}
void StopHeatTask(void* data) {

}
