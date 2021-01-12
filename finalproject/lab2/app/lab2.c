#include "includes.h"
#include <avr/io.h>   
#include <util/delay.h>
#include <stdio.h> 
#include <avr/interrupt.h>

#define		F_CPU   16000000UL   // CPU frequency = 16 Mhz
#define		N_TASKS        4
#define		TASK_STK_SIZE  OS_TASK_DEF_STK_SIZE            
#define		UCHAR unsigned char // UCHAR ����
#define     USHORT unsigned short
#define		ATS75_ADDR 0x98 // 0b10011000, 7��Ʈ�� 1��Ʈ left shift
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

void* QTable[2];//message queue�� ����� ���� �迭

volatile unsigned char fnd_sel[4] = { 0x01,0x02,0x04,0x08 };
volatile unsigned char led[9] = { 0x00, 0x01,0x02,0x04,0x08,0x10, 0x20, 0x40, 0x80 };
volatile unsigned char digit[14] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f, 0x71, 0x30, 0x77, 0x79 };
volatile int cnt = 0; //����µ��� �����ϱ����� �ѹ��� �����ϱ� ���ؼ� ���� ����
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

int cur_temp;//�������� ����µ�
volatile int hope_temp = 20;//�������� ����µ�
volatile int is_working = 0;//���� �۵� ������ Ȯ���ϴ� ���� 1�̸� �۵�, 0�̸� �۵�X
volatile int gap_temp;//�������� �µ� ��

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
	//Event ��ü �ʱ�ȭ
	curtemp_sem = OSSemCreate(1);
	hopetemp_sem = OSSemCreate(1);
	gaptemp_sem = OSSemCreate(1);
	curtemp_Mbox = OSMboxCreate((void*)0);
	temps_MsgQueue = OSQCreate((void*)QTable, 2);
	start_grp = OSFlagCreate(0x00, &err);
	end_grp = OSFlagCreate(0x00, &err);
	reset_grp = OSFlagCreate(0x00, &err);

	//Task ����
	OSTaskCreate(TemperatureTask, (void*)0, (void*)&TaskStk[0][TASK_STK_SIZE - 1], 0);
	OSTaskCreate(FndDisplayTask, (void*)0, (void*)&TaskStk[1][TASK_STK_SIZE - 1], 1);
	OSTaskCreate(startHeatTask, (void*)0, (void*)&TaskStk[2][TASK_STK_SIZE - 1], 2);
	OSTaskCreate(endHeatTask, (void*)0, (void*)&TaskStk[3][TASK_STK_SIZE - 1], 3);


	OSStart();
	return 0;
}

// ����ġ1 ��ư�� ������ ����µ��� 1�� �ö�
// ���Ͱ� �۵� ���϶��� �µ��� ��½�ų �� ����
// 36�� �̻��̸� �ٽ� default�� 20���� �ٲ�
ISR(INT4_vect) {
	if (is_working == 0) {
		//����µ��� ���������� ����ϰ� �ֱ� ������ semaphore�� ���� ��ȣ
		/*<CRRITICAL SECTION IN>*/
		OSSemPend(hopetemp_sem, 0, &err);
		//36�� �̻��̸� �ٽ� default�� 20���� �ٲ�
		if (hope_temp >= 35) {
			hope_temp = 19;
		}
		hope_temp++;
		OSSemPost(hopetemp_sem);
		/*<CRRITICAL SECTION OUT>*/
		_delay_ms(10);
	}
}

// ����ġ2 ��ư�� ������ ���Ͱ� �۵�
// ����µ��� ����µ����� ������쿡�� �۵�
ISR(INT5_vect) {
	if (is_working == 0 && (hope_temp - cur_temp > 0)) {
		is_working = 1;
		_delay_ms(10);
	}
}

//9��Ʈ, Normal
void write_twi_1byte_nopreset(UCHAR reg, UCHAR data){
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // START ����
	while (((TWCR & (1 << TWINT)) == 0x00) || ((TWSR & 0xf8) != 0x08 && (TWSR & 0xf8) != 0x10)); // ACK�� ��ٸ�
	TWDR = ATS75_ADDR | 0;  // SLA+W �غ�, W=0
	TWCR = (1 << TWINT) | (1 << TWEN);  // SLA+W ����
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x18);
	TWDR = reg;    // aTS75 Reg �� �غ�
	TWCR = (1 << TWINT) | (1 << TWEN);  // aTS75 Reg �� ����
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xF8) != 0x28);
	TWDR = data;    // DATA �غ�
	TWCR = (1 << TWINT) | (1 << TWEN);  // DATA ����
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xF8) != 0x28);
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); // STOP ����
}

//Temp Reg ������
void write_twi_0byte_nopreset(UCHAR reg){
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // START ����
	while (((TWCR & (1 << TWINT)) == 0x00) || ((TWSR & 0xf8) != 0x08 && (TWSR & 0xf8) != 0x10));  // ACK�� ��ٸ�
	TWDR = ATS75_ADDR | 0; // SLA+W �غ�, W=0
	TWCR = (1 << TWINT) | (1 << TWEN);  // SLA+W ����
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x18);
	TWDR = reg;    // aTS75 Reg �� �غ�
	TWCR = (1 << TWINT) | (1 << TWEN);  // aTS75 Reg �� ����
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xF8) != 0x28);
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); // STOP ����
}

void InitI2C(){
	PORTD = 3;						// For Pull-up override value
	SFIOR &= ~(1 << PUD);			// PUD
	TWSR = 0;						// TWPS0 = 0, TWPS1 = 0
	TWBR = 32;						// for 100  K Hz bus clock
	TWCR = _BV(TWEA) | _BV(TWEN);   // TWEA = Ack pulse is generated
									// TWEN = TWI ������ �����ϰ� �Ѵ�
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

	//�µ������� 16bit �������� ���� �������Ƿ�
	//8��Ʈ�� 2���� �޾ƾ� �Ѵ�.
	value = TWDR << 8;
	TWCR = _BV(TWINT) | _BV(TWEN);
	while (!(TWCR & _BV(TWINT)));

	value |= TWDR;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);

	value >>= 8;

	TIMSK = (value >= 33) ? TIMSK | _BV(TOIE2) : TIMSK & ~_BV(TOIE2);

	return value;
}

// ����µ��� �о���� Task
void TemperatureTask(void* data){
	INT8U err;
	data = data;

	InitI2C();
	
	write_twi_1byte_nopreset(ATS75_CONFIG_REG, 0x00); // 9��Ʈ, Normal
	write_twi_0byte_nopreset(ATS75_TEMP_REG);
	while (1) {
		//����µ��� ���������̱� ������ semaphore�� �� ��ȣ
		/*<CRRITICAL SECTION IN>*/
		OSSemPend(curtemp_sem, 0, &err);
		cur_temp = ReadTemperature();//���� �µ��� �о��
		OSSemPost(curtemp_sem);
		/*<CRRITICAL SECTION OUT>*/
		OSMboxPost(curtemp_Mbox, (void*)&cur_temp);	//���� �µ����� MailBox�� Post�Ѵ�.
		//���� �۵��� ������ ���� wait
		//���Ͱ� �۵��� �� ������ ��� ���� reset ��Ű�� �����
		OSFlagPend(reset_grp, 0x02, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);	
		cnt = 0;	//����µ��� �ѹ��� �����ϱ� ���� ����
		_delay_ms(10);
	}
}

//����µ��� ����µ��� ������
//���� �۵��� �������� �˸��� Task
//buzzer�� ���
void endHeatTask(void* data){
	data = data;
	while (1) {
		// ����µ��� ����µ��� ������ ���� wait
		OSFlagPend(end_grp, 0x01, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);
		is_working = 0;//�۵��� �������Ƿ� �� �ʱ�ȭ
		LEDTask();//�۵��� �������Ƿ� �ٶ����� 0���� �ʱ�ȭ
		_delay_ms(1000);
		int i;
		//�۵��� �������� �˸��� �� ���
		for (i = 0; i < 10; i++)							
		{
			PORTB = 0x10;
			_delay_ms(10);
			PORTB = 0x00;
			_delay_ms(10);
		}
		_delay_ms(100);
		//����µ��� ���������̹Ƿ� semaphore�� �� ��ȣ
		//����µ� �ʱ�ȭ
		/*<CRRITICAL SECTION IN>*/
		OSSemPend(hopetemp_sem, 0, &err);
		hope_temp = 20;
		OSSemPost(hopetemp_sem);
		/*<CRRITICAL SECTION OUT>*/
		//���� �� �ʱ�ȭ ������ �˸�
		OSFlagPost(reset_grp, 0x01, OS_FLAG_SET, &err);
	}
}

//���͸� �۵���Ű�� Task
//1�ʿ� ����µ��� 1�� ���
void startHeatTask(void* data){
	data = data;

	while (1) {
		//���Ͱ� �۵��Ǳ� ������ wait
		OSFlagPend(start_grp, 0x01, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);
		//����µ��� ���������̹Ƿ� semaphore�� �� ��ȣ
		/*<CRRITICAL SECTION IN>*/
		OSSemPend(cur_temp, 0, &err);
		cur_temp++;
		_delay_ms(1000);
		OSSemPost(cur_temp);
		/*<CRRITICAL SECTION OUT>*/
		//����µ��� ����µ� message queue�� �� ����
		OSQPost(temps_MsgQueue, (void*) &cur_temp);
		OSQPost(temps_MsgQueue, (void*) &hope_temp);
	}
}

//�߰�������� �ƴ����� �˱� ���ؼ� �������� �� �о���� �Լ�
USHORT read_adc() {
	UCHAR adc_low, adc_high;
	USHORT value;
	ADCSRA |= 0x40; // ADC start conversion, ADSC = '1'
	while ((ADCSRA & (0x10)) != 0x10); // ADC ��ȯ �Ϸ� �˻�
	adc_low = ADCL;
	adc_high = ADCH;
	value = (adc_high << 8) | adc_low;

	return value;
}

//���� �ٶ����⸦ ��Ÿ���� �Լ�
//����µ�-����µ� ���̸�ŭ LED�� ǥ��
//�߰������ ��� �ٶ����� 1�� �۵�
void LEDTask(){
	//�µ����� ���������̹Ƿ� semaphore�� �� ��ȣ
	/*<CRRITICAL SECTION IN>*/
	OSSemPend(gaptemp_sem, 0, &err);
	int LED_value;
	DDRA = 0xff;
	LED_value = 0;
	int i;
	USHORT value;
	value = read_adc();
	//�߰� ����� ��쿡 �ٶ����� 1
	if (value < CDS_VALUE && gap_temp != 0) {
		LED_value = led[1];
	}
	else {
		//�µ����� 8�̻��� ��� ��� LED���
		if (gap_temp >= 8) {
			for (i = 0; i <= 8; i++){
				LED_value += led[i];					
			}
		}
		//�µ����� 0, �۵��� ������ LED��� X
		else if (gap_temp == 0) {
			LED_value = led[0];
		}
		//�µ��� ��ŭ LED ���(�µ���1�� �ٶ�����1)
		else {
			for (i = gap_temp; i >= 1; i--){
				LED_value += led[i];						
			}
		}
	}
	PORTA = LED_value; //�ٶ����⸦ LED�� ���
	_delay_ms(20);
	OSSemPost(gaptemp_sem);
	/*<CRRITICAL SECTION OUT>*/
}

//FND�� ����µ��� ����µ��� ����ϴ� Task
void FndDisplayTask(void* data){
	int fnd[4], temp;
	INT8U err;
	data = data;
	
	while (1){
		//���Ͱ� �۵� ���� ���
		if (is_working == 0) {
			//����µ��� �ѹ��� �������� �������
			if (cnt == 0) {
				//����µ��� �����ϱ� ������ wait
				cur_temp = *(int*)OSMboxPend(curtemp_Mbox, 0, &err);
				cnt++;//����µ��� �����ϱ����� �ѹ��� �����ϱ� ���ؼ� ���� ����
			}
			fnd[3] = cur_temp / 10;	//����µ��� 10�� �ڸ�
			fnd[2] = cur_temp % 10;	//����µ��� 1�� �ڸ�
			fnd[1] = hope_temp / 10; //����µ��� 10�� �ڸ�
			fnd[0] = hope_temp % 10; //����µ��� 1�� �ڸ�
			int i, j;
			for (i = 0; i < 100; i++) {
				for (j = 0; j < 4; j++){
					//fnd 2���� �� .�� ����ؼ� ����µ��� ����µ��� ����
					if (j == 2)	{
						temp = digit[fnd[j]] + 128;	// 0x80�� ������ fnd 2���� dot���� ����
						PORTC = temp;
					}
					else {
						PORTC = digit[fnd[j]]; //C ��Ʈ�� FND ������
					}
					PORTG = fnd_sel[j];	//G ��Ʈ�� FND ����
					_delay_ms(2);
				}
			}
		}
		//���Ͱ� �۵� ���� ���
		else {
			OSFlagPost(start_grp, 0x01, OS_FLAG_SET, &err);//flag�� ���͸� �۵��϶�� �˸�
			cur_temp = *(int*)OSQPend(temps_MsgQueue, 0, &err);	//MsgQueue�� ����µ��� �� ������ wait
			hope_temp = *(int*)OSQPend(temps_MsgQueue, 0, &err);//MsgQueue�� ����µ��� �� ������ wait
			//���������� ����µ��� ����µ��� ����ϴ°��̱� ������ semaphore�� ���� ��ȣ
			//����µ��� ����µ��� �̿��ؼ� �µ� ���� ����
			/*<CRRITICAL SECTION IN>*/
			OSSemPend(gaptemp_sem, 0, &err); 
			gap_temp = hope_temp - cur_temp;
			OSSemPost(gaptemp_sem);
			/*<CRRITICAL SECTION OUT>*/

			//�µ����� 0�� ��� ���͸� ����
			if (gap_temp == -1) {
				//flag�� ���͸� �����Ű��� �˸� 
				OSFlagPost(end_grp, 0x01, OS_FLAG_SET, &err);
				//flag�� ���͸� ������ϰ� �������� reset�ϱ� ������ wait
				OSFlagPend(reset_grp, 0x01, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);
				//���͸� ������ϰ� ����µ��� �ٽ� �����ϱ� ���� flag�� �˸�
				OSFlagPost(reset_grp, 0x02, OS_FLAG_SET, &err);
			}
			else {
				LEDTask();//�ٶ����⸦ LED�� ǥ��
				fnd[3] = cur_temp / 10;	//����µ��� 10�� �ڸ�
				fnd[2] = cur_temp % 10;	//����µ��� 1�� �ڸ�
				fnd[1] = hope_temp / 10; //����µ��� 10�� �ڸ�
				fnd[0] = hope_temp % 10; //����µ��� 1�� �ڸ�
				int i, j;
				for (i = 0; i < 100; i++) {
					for (j = 0; j < 4; j++){
						//fnd 2���� �� .�� ����ؼ� ����µ��� ����µ��� ����
						if (j == 2)	{
							temp = digit[fnd[j]] + 128;	// 0x80�� ������ fnd 2���� dot���� ����		
							PORTC = temp;
						}
						else {
							PORTC = digit[fnd[j]]; //C ��Ʈ�� FND ������
						}
						PORTG = fnd_sel[j];	//G ��Ʈ�� FND ����
						_delay_ms(2);
					}
				}
			}
		}
	}
}