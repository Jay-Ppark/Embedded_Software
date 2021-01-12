#include <avr/io.h>  //ATmage128 레지스터 정의
#define F_CPU 16000000UL
#include <util/delay.h>
typedef unsigned char uc;
const uc digit[10] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f };
const uc fnd_sel[4] = { 0x08,0x04,0x02,0x01 };
const uc dot = 0x80;
void printNum(uc* num) {
    int t;
    for (t = 0; t < 4; t++) {
        PORTC = num[t];
        PORTG = fnd_sel[t];
        _delay_ms(2.5);
    }
}
void main(){
    uc num[4] = { 0x3f,0x3f,0x3f,0x3f };
    int i;
    int j;
    int k;
    int l;
    DDRC = 0xff;
    DDRG = 0x0f;
     for (i = 0;;i++) {
         if (i == 10) {
             i = 0;
         }
         num[0] = digit[i];
         for (j = 0; j < 10; j++) {
             num[1] = digit[j] + dot;
             for (k = 0; k < 10; k++) {
                 num[2] = digit[k];
                 for (l = 0; l < 10; l++) {
                     num[3] = digit[l];
                     printNum(num);
                 }
             }
         }
     }
 }
