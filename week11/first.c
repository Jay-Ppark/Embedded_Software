#include <avr/io.h>  //ATmage128 레지스터 정의
#include<avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#define START 1
#define STOP 0
typedef unsigned char uc;
const uc digit[10] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f };
const uc fnd_sel[4] = { 0x08,0x04,0x02,0x01 };
const uc dot = 0x80;
int signal;
int count;
void display_fnd(int c) {
    int t;
    int div = 1000;
    for (t = 0; t < 4; t++) {
        if (t == 1) {
            PORTC = digit[(c / div) % 10] + dot;
        }
        else {
            PORTC = digit[(c / div) % 10];
        }
        PORTG = fnd_sel[t];
        div = div / 10;
        _delay_ms(2.5);
    }
}
ISR(INT4_vect) {
    if (signal == START)
        signal = STOP;
    else
        signal = START;
    _delay_ms(10);
}
ISR(INT5_vect) {
    if (signal == STOP) {
        count = 0;
        display_fnd(count);
    }
    _delay_ms(10);
}
void main() {
    DDRC = 0xff;
    DDRG = 0x0f;
    DDRE = 0xcf;
    EICRB = 0x0A;
    EIMSK = 0x30;
    SREG |= 1 << 7;
    for (;;) {
        display_fnd(count);
        if (signal == START)
            count = (count + 1) % 10000;
    }
}
