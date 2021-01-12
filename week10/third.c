#include <avr/io.h>  //ATmage128 레지스터 정의
#define F_CPU 16000000UL
#include <util/delay.h>
#include <stdlib.h>
typedef unsigned char uc;
const uc digit[10] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f };
const uc fnd_sel[4] = { 0x08,0x04,0x02,0x01 };
const uc led_v[8] = { 1,2,4,8,16,32,64,128};
void main(){
    int i;
    int j;
    int random;
    DDRC = 0xff;
    DDRG = 0x0f;
    DDRA = 0xff;
     for (;;) {
         random = rand() % 8;
         PORTA = led_v[random];
         for (j = 0; j < 600; j++) {
             PORTC = digit[random];
             PORTG = fnd_sel[j % 4];
             _delay_ms(1);
         }
         
     }
 }
