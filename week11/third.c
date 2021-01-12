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
const unsigned char mel[45] = { DO,MI,SOL,UDO,LA,UDO,LA,SOL,FA,SOL,MI,DO,RE,DO,SOL,SOL,FA,FA,MI,SOL,MI,RE,SOL,SOL,FA,FA,MI,SOL,MI,RE,DO,MI,SOL,UDO,LA,UDO,LA,SOL,FA,SOL,MI,DO,RE,RE,DO };
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
void main() {
    DDRB = 0x10;
    TCCR0 = 0x03;
    TIMSK = 0x01;
    TCNT0 = mel[index];
    sei();
    while (1) {
        index = (index + 1) % 45;
        _delay_ms(200);
    }
}
