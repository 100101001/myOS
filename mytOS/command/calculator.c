#include "stdio.h"
#include "string.h"

int getNum(char * bufr)
{
	int i = 0;
	int ten = 1, res = 0;
	if (bufr[0] != '-')
	{ 
		for (i = 0; i < strlen(bufr) - 1; i++)
		{
			ten *= 10;
		}
		for (i = 0; i < strlen(bufr); i++)
		{
			res += (bufr[i] - '0') * ten;
			ten /= 10;
		}
	}
	else
	{
		for (i = 1; i < strlen(bufr) - 1; i++)
		{
			ten *= 10;
		}
		for (i = 1; i < strlen(bufr); i++)
		{
			res -= (bufr[i] - '0') * ten;
			ten /= 10;
		}
	} 
	return res;
}

void ClearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}

int main()
{	
	int i, flag = 1;
	int num1 = 0, num2 = 0, res = 0;
	char bufr[128];
	printf("***************************************************\n");
	printf("*                 Calculator                      *\n");
	printf("***************************************************\n");
	printf("*  1. Only two parameters                         *\n");
	printf("*  2. Only for integer                            *\n");
	printf("*  3. Both positive and negative number           *\n");
	printf("*  4. Enter q to quit                             *\n");
	printf("***************************************************\n");
	while(flag == 1){	
		printf("\nPlease input num1:");
		i = read (0, bufr, 128);
		if (bufr[0] == 'q')
			break;
		num1 = getNum(bufr);
		ClearArr(bufr, 128);
		printf("num1: %d\n", num1);
		
		printf("Please input num2:");
		i = read (0, bufr, 128);
		if (bufr[0] == 'q')
			break;
		num2 = getNum(bufr);
		ClearArr(bufr, 128);
		printf("num2: %d\n", num2);
		
		printf("Please input op( + - * / ):");
		i = read (0, bufr, 1);
		switch(bufr[0])
		{
			case '+':
				res = num1 + num2;
				printf("%d + %d = %d\n", num1, num2, res);
				break;
			case '-':
				res = num1 - num2;
				printf("%d - %d = %d\n", num1, num2, res);
				break;
			case '*':
				res = num1 * num2;
				printf("%d * %d = %d\n", num1, num2, res);
				break;
			case '/':
				if(num2 == 0)
				{
					printf("Error!\n");
					break;
				}
				res = num1 / num2;
				printf("%d / %d = %d\n", num1, num2, res);
				break;
			case 'q':
				flag = 0;
				break;
			default:
				printf("No such command!\n");
		}
		ClearArr(bufr, 128);
	}
	return 0;
}

