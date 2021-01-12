#include<iostream>
using namespace std;
unsigned char LEDs;
int countOne(int input)
{
	int cnt = 0;
	while (input > 1)
	{
		if (input % 2 == 1)
		{
			cnt++;
		}
		input = input / 2;
	}
	if (cnt != 0 || input != 0)
	{
		cnt++;
	}
	return cnt;
}
int shiftAllLeft(int n)
{
	LEDs = 1;
	for (int i = 0; i < n - 1; i++)
	{
		LEDs |= (LEDs << 1);
	}
	for (int i = 0; i < 8 - n; i++)
	{
		LEDs = (LEDs << 1);
	}
	return LEDs;
}
int main(void)
{
	int inputnum;
	cout << "십진수를 입력하시오:";
	cin >> inputnum;
	int output = countOne(inputnum);
	cout << "1의 개수? " << output << '\n';
	cout << "Shift 시의 값은? "<<shiftAllLeft(output) << '\n';
	return 0;
}

/*
   while (1) {
	   rand_num = random(64);
	   if (OSMapTbl[rand_num >> 3] & *myRdyGrp && myRdyTbl[OSMapTbl[rand_num >> 3]] & OSMapTbl[rand_num & 0x07]) continue;
	   else {
		   *myRdyGrp |= OSMapTbl[rand_num >> 3] & *myRdyGrp;
		   myRdyTbl[OSMapTbl[rand_num >> 3]] |= OSMapTbl[rand_num & 0x07];
		   return rand_num;
	   }
   }
   */