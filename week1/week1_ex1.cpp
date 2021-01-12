#include<iostream>
using namespace std;
unsigned char LEDs;
int LED_function(int input)
{
	LEDs |= (1 << input);
	return LEDs;
}
int main(void)
{
	int inputnum;
	cout << "0~7 숫자를 입력하시오:";
	cin >> inputnum;
	int output = LED_function(inputnum);
	cout << "입력 값 : " << inputnum << " 출력 값 : " << output << '\n';
	return 0;
}