#include <avr/io.h>  //ATmage128 레지스터 정의
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
typedef unsigned char uc;
#define ON 1
#define OFF 0
#define DO 17
#define RE 43
#define MI 66
#define FA 77
#define SOL 97
#define LA 114
#define TI 117
#define UDO 137
volatile int state = OFF;
volatile int index;
const unsigned char mel[8] = { DO,RE,MI,FA,SOL,LA,TI,UDO };
ISR(TIMER0_OVF_vect) {
    if (state == ON) {
        PORTB = 0x00;
        state = OFF;
    }
    else {
        PORTB = 0x10;
        state = ON;
    }
    TCNT0 = mel[index];
}
ISR(INT5_vect) {
    index = (index + 1) % 8;
    _delay_ms(10);
}
void main(){
    DDRB = 0x10;
    DDRE = 0xdf;
    EICRB = 0x08;
    EIMSK = 0x20;
    SREG |= 1 << 7;
    TCCR0 = 0x03;
    TIMSK = 0x01;
    TCNT0 = mel[index];
    sei();
    while (1);
 }
