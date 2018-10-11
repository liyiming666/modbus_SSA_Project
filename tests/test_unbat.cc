
#include <iostream>

using namespace std;

typedef unsigned short u16;


u16 unbat(u16 old, int& sb, int& nb, int& value)
{
	int t = ~(((1 << nb) - 1) << sb);	
	old = (old & t);

	int data = 0;	

	if (value >= 0) data = old | (value << sb);
	else {
		value = ~value + 1;
		data = old | (value << sb);
	} 
	
	return data;

}
int main(void)
{
	int old_value = 7;
	int sb = 2;
	int nb = 1;
	int value = 0;
	int new_value = unbat(old_value, sb, nb, value);
	cout << new_value << endl;


	return 0;
}
