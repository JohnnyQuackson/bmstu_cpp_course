#include "str2int.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "stdio.h"

int INT_MAX = 2147483647, INT_MIN = -2147483648;

int str2int(const char* str)
{
	int n = 0, digit;
	unsigned int x;
	bool negative = false;
	if (*str == '-')
	{
		negative = true;
		str++;
	}
	else if (*str == '+')
	{
		str++;
	}
	if (*str == '\0')
	{
		abort();
	}
	while (*str != '\0')
	{
		digit = (int)(*str) - (int)'0';
		x = 10 * n + digit;
		if ((negative == false && x > INT_MAX) ||
			(negative == true && x > INT_MAX + 1))
		{
			abort();
		}

		n = 10 * n + digit;
		str++;
	}
	return (negative) ? -n : n;
}
