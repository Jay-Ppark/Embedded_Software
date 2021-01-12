#include<iostream>
using namespace std;
unsigned char LEDs;
int LED_shiftleft()
{
	LEDs = (LEDs << 1);
	return LEDs;
}
int LED_shiftright()
{
	LEDs = (LEDs >> 1);
	return LEDs;
}
int main(void)
{
	LEDs = 1;
	int testcase = 4;
	while (testcase > 0)
	{
		for (int i = 0; i < 8; i++)
		{
			if (i == 0)
			{
				cout << (int)LEDs;
			}
			else
			{
				cout <<", "<<LED_shiftleft();
			}
		}
		for (int i = 0; i < 7; i++)
		{
			cout << ", " << LED_shiftright();
		}
		cout << '\n';
		testcase--;
	}
	return 0;
}