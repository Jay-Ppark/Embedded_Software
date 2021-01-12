#include "includes.h"
#include <avr/io.h>   
#include <util/delay.h>
#include <stdio.h> 
#include <avr/interrupt.h>

#define		F_CPU   16000000UL   // CPU frequency = 16 Mhz
#define		N_TASKS        4
#define		TASK_STK_SIZE  OS_TASK_DEF_STK_SIZE            
#define		UCHAR unsigned char // UCHAR 정의
#define     USHORT unsigned short
#define		ATS75_ADDR 0x98 // 0b10011000, 7비트를 1비트 left shift
#define		ATS75_CONFIG_REG 1
#define		ATS75_TEMP_REG 0
#define		CDS_VALUE 871

OS_STK      TaskStk[N_TASKS][TASK_STK_SIZE];

OS_EVENT* curtemp_Mbox;//mailbox
OS_EVENT* temps_MsgQueue;//msg queue
OS_EVENT* curtemp_sem;//semaphore
OS_EVENT* hopetemp_sem;//semaphore
OS_EVENT* gaptemp_sem;//semaphore
OS_EVENT* start_grp;//flag
OS_EVENT* end_grp;//flag
OS_EVENT* reset_grp;//flag

void* QTable[2];//message queue의 결과를 담을 배열

volatile unsigned char fnd_sel[4] = { 0x01,0x02,0x04,0x08 };
volatile unsigned char led[9] = { 0x00, 0x01,0x02,0x04,0x08,0x10, 0x20, 0x40, 0x80 };
volatile unsigned char digit[14] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f, 0x71, 0x30, 0x77, 0x79 };
volatile int cnt = 0; //현재온도를 시작하기전에 한번만 측정하기 위해서 만든 변수
/*FUNCTION*/
void InitI2C(void);
int ReadTemperature(void);
void write_twi_1byte_nopreset(UCHAR reg, UCHAR data);
void write_twi_0byte_nopreset(UCHAR reg);
void LEDTask();
USHORT read_adc();
/*TASK*/
void FndDisplayTask(void* data);
void startHeatTask(void* data);
void endHeatTask(void* data);
void TemperatureTask(void* data);
INT8U err;

int cur_temp;//공유변수 현재온도
volatile int hope_temp = 20;//공유변수 희망온도
volatile int is_working = 0;//현재 작동 중인지 확인하는 변수 1이면 작동, 0이면 작동X
volatile int gap_temp;//공유변수 온도 차

int main(void)
{
	OSInit();
	/*<CRRITICAL SECTION IN>*/
	OS_ENTER_CRITICAL();
	TCCR0 = 0x07;
	TIMSK = _BV(TOIE0);
	TCNT0 = 256 - (CPU_CLOCK_HZ / OS_TICKS_PER_SEC / 1024);
	
	ADMUX = 0x00;
	ADCSRA = 0x87;

	DDRC = 0xff;
	DDRB = 0x10;
	DDRG = 0x0f;
	DDRA = 0xff;
	DDRE = 0xCF;
	EICRB = 0x0A;
	EIMSK = 0x30;
	SREG |= 1 << 7;
	PORTG = 0x0f;
	OS_EXIT_CRITICAL();
	/*<CRRITICAL SECTION OUT>*/
	//Event 객체 초기화
	curtemp_sem = OSSemCreate(1);
	hopetemp_sem = OSSemCreate(1);
	gaptemp_sem = OSSemCreate(1);
	curtemp_Mbox = OSMboxCreate((void*)0);
	temps_MsgQueue = OSQCreate((void*)QTable, 2);
	start_grp = OSFlagCreate(0x00, &err);
	end_grp = OSFlagCreate(0x00, &err);
	reset_grp = OSFlagCreate(0x00, &err);

	//Task 생성
	OSTaskCreate(TemperatureTask, (void*)0, (void*)&TaskStk[0][TASK_STK_SIZE - 1], 0);
	OSTaskCreate(FndDisplayTask, (void*)0, (void*)&TaskStk[1][TASK_STK_SIZE - 1], 1);
	OSTaskCreate(startHeatTask, (void*)0, (void*)&TaskStk[2][TASK_STK_SIZE - 1], 2);
	OSTaskCreate(endHeatTask, (void*)0, (void*)&TaskStk[3][TASK_STK_SIZE - 1], 3);


	OSStart();
	return 0;
}

// 스위치1 버튼을 누르면 희망온도가 1도 올라감
// 히터가 작동 중일때는 온도를 상승시킬 수 없음
// 36도 이상이면 다시 default인 20도로 바꿈
ISR(INT4_vect) {
	if (is_working == 0) {
		//희망온도를 공유변수로 사용하고 있기 때문에 semaphore로 값을 보호
		/*<CRRITICAL SECTION IN>*/
		OSSemPend(hopetemp_sem, 0, &err);
		//36도 이상이면 다시 default인 20도로 바꿈
		if (hope_temp >= 35) {
			hope_temp = 19;
		}
		hope_temp++;
		OSSemPost(hopetemp_sem);
		/*<CRRITICAL SECTION OUT>*/
		_delay_ms(10);
	}
}

// 스위치2 버튼을 누르면 히터가 작동
// 희망온도가 현재온도보다 높을경우에만 작동
ISR(INT5_vect) {
	if (is_working == 0 && (hope_temp - cur_temp > 0)) {
		is_working = 1;
		_delay_ms(10);
	}
}

//9비트, Normal
void write_twi_1byte_nopreset(UCHAR reg, UCHAR data){
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // START 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || ((TWSR & 0xf8) != 0x08 && (TWSR & 0xf8) != 0x10)); // ACK를 기다림
	TWDR = ATS75_ADDR | 0;  // SLA+W 준비, W=0
	TWCR = (1 << TWINT) | (1 << TWEN);  // SLA+W 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x18);
	TWDR = reg;    // aTS75 Reg 값 준비
	TWCR = (1 << TWINT) | (1 << TWEN);  // aTS75 Reg 값 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xF8) != 0x28);
	TWDR = data;    // DATA 준비
	TWCR = (1 << TWINT) | (1 << TWEN);  // DATA 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xF8) != 0x28);
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); // STOP 전송
}

//Temp Reg 포인팅
void write_twi_0byte_nopreset(UCHAR reg){
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // START 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || ((TWSR & 0xf8) != 0x08 && (TWSR & 0xf8) != 0x10));  // ACK를 기다림
	TWDR = ATS75_ADDR | 0; // SLA+W 준비, W=0
	TWCR = (1 << TWINT) | (1 << TWEN);  // SLA+W 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x18);
	TWDR = reg;    // aTS75 Reg 값 준비
	TWCR = (1 << TWINT) | (1 << TWEN);  // aTS75 Reg 값 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xF8) != 0x28);
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); // STOP 전송
}

void InitI2C(){
	PORTD = 3;						// For Pull-up override value
	SFIOR &= ~(1 << PUD);			// PUD
	TWSR = 0;						// TWPS0 = 0, TWPS1 = 0
	TWBR = 32;						// for 100  K Hz bus clock
	TWCR = _BV(TWEA) | _BV(TWEN);   // TWEA = Ack pulse is generated
									// TWEN = TWI 동작을 가능하게 한다
}

int ReadTemperature(void){
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

// 현재온도를 읽어오는 Task
void TemperatureTask(void* data){
	INT8U err;
	data = data;

	InitI2C();
	
	write_twi_1byte_nopreset(ATS75_CONFIG_REG, 0x00); // 9비트, Normal
	write_twi_0byte_nopreset(ATS75_TEMP_REG);
	while (1) {
		//현재온도가 공유변수이기 때문에 semaphore로 값 보호
		/*<CRRITICAL SECTION IN>*/
		OSSemPend(curtemp_sem, 0, &err);
		cur_temp = ReadTemperature();//현재 온도를 읽어옴
		OSSemPost(curtemp_sem);
		/*<CRRITICAL SECTION OUT>*/
		OSMboxPost(curtemp_Mbox, (void*)&cur_temp);	//읽은 온도값을 MailBox에 Post한다.
		//히터 작동이 끝날때 까지 wait
		//히터가 작동이 다 끝나면 모든 값을 reset 시키고 재시작
		OSFlagPend(reset_grp, 0x02, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);	
		cnt = 0;	//현재온도를 한번만 측정하기 위한 변수
		_delay_ms(10);
	}
}

//희망온도와 현재온도가 같아짐
//히터 작동이 끝났음을 알리는 Task
//buzzer음 출력
void endHeatTask(void* data){
	data = data;
	while (1) {
		// 현재온도와 희망온도가 같을때 까지 wait
		OSFlagPend(end_grp, 0x01, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);
		is_working = 0;//작동이 끝났으므로 값 초기화
		LEDTask();//작동이 끝났으므로 바람세기 0으로 초기화
		_delay_ms(1000);
		int i;
		//작동이 끝났음을 알리는 음 출력
		for (i = 0; i < 10; i++)							
		{
			PORTB = 0x10;
			_delay_ms(10);
			PORTB = 0x00;
			_delay_ms(10);
		}
		_delay_ms(100);
		//희망온도가 공유변수이므로 semaphore로 값 보호
		//희망온도 초기화
		/*<CRRITICAL SECTION IN>*/
		OSSemPend(hopetemp_sem, 0, &err);
		hope_temp = 20;
		OSSemPost(hopetemp_sem);
		/*<CRRITICAL SECTION OUT>*/
		//값을 다 초기화 했음을 알림
		OSFlagPost(reset_grp, 0x01, OS_FLAG_SET, &err);
	}
}

//히터를 작동시키는 Task
//1초에 현재온도가 1씩 상승
void startHeatTask(void* data){
	data = data;

	while (1) {
		//히터가 작동되기 전까지 wait
		OSFlagPend(start_grp, 0x01, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);
		//현재온도가 공유변수이므로 semaphore로 값 보호
		/*<CRRITICAL SECTION IN>*/
		OSSemPend(cur_temp, 0, &err);
		cur_temp++;
		_delay_ms(1000);
		OSSemPost(cur_temp);
		/*<CRRITICAL SECTION OUT>*/
		//현재온도와 희망온도 message queue로 값 전달
		OSQPost(temps_MsgQueue, (void*) &cur_temp);
		OSQPost(temps_MsgQueue, (void*) &hope_temp);
	}
}

//야간모드인지 아닌지를 알기 위해서 광센서로 값 읽어오는 함수
USHORT read_adc() {
	UCHAR adc_low, adc_high;
	USHORT value;
	ADCSRA |= 0x40; // ADC start conversion, ADSC = '1'
	while ((ADCSRA & (0x10)) != 0x10); // ADC 변환 완료 검사
	adc_low = ADCL;
	adc_high = ADCH;
	value = (adc_high << 8) | adc_low;

	return value;
}

//현재 바람세기를 나타내는 함수
//희망온도-현재온도 차이만큼 LED에 표시
//야간모드일 경우 바람세기 1로 작동
void LEDTask(){
	//온도차가 공유변수이므로 semaphore로 값 보호
	/*<CRRITICAL SECTION IN>*/
	OSSemPend(gaptemp_sem, 0, &err);
	int LED_value;
	DDRA = 0xff;
	LED_value = 0;
	int i;
	USHORT value;
	value = read_adc();
	//야간 모드인 경우에 바람세기 1
	if (value < CDS_VALUE && gap_temp != 0) {
		LED_value = led[1];
	}
	else {
		//온도차가 8이상인 경우 모든 LED출력
		if (gap_temp >= 8) {
			for (i = 0; i <= 8; i++){
				LED_value += led[i];					
			}
		}
		//온도차가 0, 작동이 끝나서 LED출력 X
		else if (gap_temp == 0) {
			LED_value = led[0];
		}
		//온도차 만큼 LED 출력(온도차1은 바람세기1)
		else {
			for (i = gap_temp; i >= 1; i--){
				LED_value += led[i];						
			}
		}
	}
	PORTA = LED_value; //바람세기를 LED로 출력
	_delay_ms(20);
	OSSemPost(gaptemp_sem);
	/*<CRRITICAL SECTION OUT>*/
}

//FND에 현재온도와 희망온도를 출력하는 Task
void FndDisplayTask(void* data){
	int fnd[4], temp;
	INT8U err;
	data = data;
	
	while (1){
		//히터가 작동 전인 경우
		if (is_working == 0) {
			//현재온도를 한번도 측정하지 않은경우
			if (cnt == 0) {
				//현재온도를 측정하기 전까지 wait
				cur_temp = *(int*)OSMboxPend(curtemp_Mbox, 0, &err);
				cnt++;//현재온도를 시작하기전에 한번만 측정하기 위해서 만든 변수
			}
			fnd[3] = cur_temp / 10;	//현재온도의 10의 자리
			fnd[2] = cur_temp % 10;	//현재온도의 1의 자리
			fnd[1] = hope_temp / 10; //희망온도의 10의 자리
			fnd[0] = hope_temp % 10; //희망온도의 1의 자리
			int i, j;
			for (i = 0; i < 100; i++) {
				for (j = 0; j < 4; j++){
					//fnd 2번일 때 .을 출력해서 현재온도와 희망온도를 구분
					if (j == 2)	{
						temp = digit[fnd[j]] + 128;	// 0x80을 더해줘 fnd 2번에 dot까지 찍음
						PORTC = temp;
					}
					else {
						PORTC = digit[fnd[j]]; //C 포트는 FND 데이터
					}
					PORTG = fnd_sel[j];	//G 포트는 FND 선택
					_delay_ms(2);
				}
			}
		}
		//히터가 작동 중인 경우
		else {
			OSFlagPost(start_grp, 0x01, OS_FLAG_SET, &err);//flag로 히터를 작동하라고 알림
			cur_temp = *(int*)OSQPend(temps_MsgQueue, 0, &err);	//MsgQueue로 현재온도가 올 때까지 wait
			hope_temp = *(int*)OSQPend(temps_MsgQueue, 0, &err);//MsgQueue에 희망온도가 올 때까지 wait
			//공유변수인 현재온도와 희망온도를 계산하는것이기 때문에 semaphore로 값을 보호
			//현재온도와 희망온도를 이용해서 온도 차를 구함
			/*<CRRITICAL SECTION IN>*/
			OSSemPend(gaptemp_sem, 0, &err); 
			gap_temp = hope_temp - cur_temp;
			OSSemPost(gaptemp_sem);
			/*<CRRITICAL SECTION OUT>*/

			//온도차가 0인 경우 히터를 종료
			if (gap_temp == -1) {
				//flag로 히터를 종료시키라고 알림 
				OSFlagPost(end_grp, 0x01, OS_FLAG_SET, &err);
				//flag로 히터를 재시작하고 변수들을 reset하기 전까지 wait
				OSFlagPend(reset_grp, 0x01, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);
				//히터를 재시작하고 현재온도를 다시 측정하기 위해 flag로 알림
				OSFlagPost(reset_grp, 0x02, OS_FLAG_SET, &err);
			}
			else {
				LEDTask();//바람세기를 LED로 표시
				fnd[3] = cur_temp / 10;	//현재온도의 10의 자리
				fnd[2] = cur_temp % 10;	//현재온도의 1의 자리
				fnd[1] = hope_temp / 10; //희망온도의 10의 자리
				fnd[0] = hope_temp % 10; //희망온도의 1의 자리
				int i, j;
				for (i = 0; i < 100; i++) {
					for (j = 0; j < 4; j++){
						//fnd 2번일 때 .을 출력해서 현재온도와 희망온도를 구분
						if (j == 2)	{
							temp = digit[fnd[j]] + 128;	// 0x80을 더해줘 fnd 2번에 dot까지 찍음		
							PORTC = temp;
						}
						else {
							PORTC = digit[fnd[j]]; //C 포트는 FND 데이터
						}
						PORTG = fnd_sel[j];	//G 포트는 FND 선택
						_delay_ms(2);
					}
				}
			}
		}
	}
}