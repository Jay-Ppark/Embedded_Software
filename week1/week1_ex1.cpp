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
	cout << "0~7 ���ڸ� �Է��Ͻÿ�:";
	cin >> inputnum;
	int output = LED_function(inputnum);
	cout << "�Է� �� : " << inputnum << " ��� �� : " << output << '\n';
	return 0;
}