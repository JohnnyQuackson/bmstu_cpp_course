#include "int2str.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "stdio.h"

char* int2str(int number)
{
	char reversed[100];
	int l = 0;
	unsigned int n;
	bool negative = false;
	if (number < 0)
	{
		negative = true;
		n = (unsigned int)(-(number + 1)) + 1;
	}
	else if (number == 0)
	{
		n = (unsigned int)number;
		reversed[l] = '0';
		l++;
	}
	else
	{
		n = (unsigned int)number;
	}
	while (n != 0)
	{
		reversed[l] = (char)((n % 10) + (int)'0');
		n /= 10;
		l++;
	}
	if (negative)
	{
		reversed[l] = '-';
		l++;
	}
	char* str = malloc(l + 1);
	for (int i = 0; i < l; i++)
	{
		str[i] = reversed[l - 1 - i];
	}
	str[l] = '\0';
	return str;
}
