#include <avr/io.h>  //ATmage128 레지스터 정의
#define F_CPU 16000000UL
#include <util/delay.h>
void main(){
     unsigned char value = 128;
     unsigned char way = 1;
     DDRA= 0xff; // 포트 A를 출력 포트로 사용 
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
