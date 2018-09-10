#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>

char* month_str[] = {"January","February","March","April","May","June","July","August","September","October","November","December"}; 
char* week[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"}; 

int leap(int year) //判断闰年
{ 
	if ((year%4==0 && year%100!=0) || year%400==0)
    		return 1;
    	else 
		return 0;
}

int month_day(int year, int month) //判断这一个月有多少天
{ 
	int mon_day[] = {31,28,31,30,31,30,31,31,30,31,30,31};
	if (leap(year) && month==2) 
		return 29; 
	else 
		return (mon_day[month-1]); 

}

int firstday(int year, int month, int day) //计算给定的日期是星期几 
{
	if(month==1 || month==2) 
	{
        	month += 12;
        	year--;
    	}
    	int iWeek = (day+2*month+3*(month+1)/5+year+year/4-year/100+year/400) % 7;
	if(iWeek == 6)
		return 0;
	else
    		return iWeek + 1;
	
	/*int c = 0; 
	float s; 
	int m; 
	for(m=1; m<month; m++) 
		c = c + month_day(year, m); 
	c = c + day; 
	s = year-1+(float)(year-1)/4+(float)(year-1)/100+(float)(year-1)/400-40+c; 
	return ((int)s%7); */
}

int PrintAllYear(int year) //打印整年日历 
{ 
	int a, b; 
	int i, j=1, n=1, k; 
	printf("\n\n**************Year %d**************\n",year); 
	for(k=1; k<=12; k++) 
	{
	    	j = 1;
		n = 1;
		b = month_day(year, k);
		a = firstday(year, k, 1);
		printf("\n\n%s(%d)\n", month_str[k-1],k);  
		printf("               Sun Mon Tue Wed Thu Fri Sat \n**************");
		if(a == 7)
		{
			for(i=1; i<=b; i++)
			{
				printf("%4d", i);
	            		if(i%7 == 0)
				{
	    				printf("**************\n**************");
				}
			}
		}
	    	if(a != 7)
		{
			while (j <= 4*a)
			{
				printf(" ");
	            		j++;
			}
	        	for(i=1; i<=b; i++)
			{
				printf("%4d", i); 
	            		if(i == 7*n-a)
				{
	    				printf("**************\n**************");
	                		n++;
				}
			}
		}
		printf("**************\n"); 
	}
	return 1;
}

int getNum(char * bufr)
{
	int i = 0;
	int ten = 1, res = 0;
	for (i = 0; i < strlen(bufr) - 1; i++)
	{
		ten *= 10;
	}
	for (i = 0; i < strlen(bufr); i++)
	{
		res += (bufr[i] - '0') * ten;
		ten /= 10;
	}
	return res;
}

int main()
{
	int da; 
	int year, month, day; 
	printf("*********************************************************\n");
	printf("*                      calendar                         *\n");
	printf("*********************************************************\n");
	printf("*     1. From Year 1990 to future                       *\n");
	printf("*     2. Display current month                          *\n");
	printf("*     3. Enter 1 to get what day is the input date      *\n");
	printf("*     4. Enter 2 to judge leap year or not              *\n");
	printf("*     5. Enter 3 to print the whole calendar of a year  *\n");
	printf("*     6. Enter 4 to quit                                *\n");
	printf("*********************************************************\n\n");
	
	//调用系统时间
	/*time_t tval;
	struct tm *now;
	tval = time(NULL);
	now = localtime(&tval);
	printf("Current time:  %4d/%d/%02d  %d:%02d:%02d",  now->tm_year+1900, now->tm_mon+1, now->tm_mday,now->tm_hour, now->tm_min, now->tm_sec);*/
	//调用结束
	
	int i, j=1, k=1;
	int a, b;
	b = month_day(2017, 9);
	a = firstday(2017, 9, 1);
	printf("%s:\n", month_str[8]);
	printf(" Sun Mon Tue Wed Thu Fri Sat \n");
	if(a == 7)
	{
		for(i=1; i<=b; i++)
		{
			printf("%4d",i);
	        if(i%7 == 0)
			{
	    		printf("\n");
			}
		}
	}
	if(a != 7)
	{
		while(j <= 4*a)
		{
			printf(" ");
	        	j++;
		}
	    	for(i=1; i<=b; i++)
		{
			printf("%4d", i); 
	        	if(i == 7*k-a)
			{
	    			printf("\n");
	            		k++;
			}
		}
	}
	printf("\n");
	
	char option[2];
	char conti[2];
	char Syear[5];
	char Smonth[3];
	char Sday[3];
	int r;
	while(1) 
	{ 
		printf("\nEnter 1 to get what day is the input date"); 
		printf("\nEnter 2 to judge leap year or not"); 
		printf("\nEnter 3 to print the whole calendar of a year"); 
		printf("\nEnter 4 to quit"); 
		printf("\nPlease enter the service:"); 
		r = read(0, option, 2); 
	
		switch(option[0]) 
		{ 
			case '1': 
				while(1) 
				{ 
					printf("\nPlease input the year(XXXX):");
					r = read(0, Syear, 5);
 					printf("\nPlease input the month(XX):");
					r = read(0, Smonth, 3);
					printf("\nPlease input the day(XX):");
					r = read(0, Sday, 3);
					year = getNum(Syear);
					month = getNum(Smonth);
					day = getNum(Sday);
					da = firstday(year, month, day); 
					printf("\n%d-%d-%d is %s\n", year, month, day, week[da]);
					printf("\nDo you want to continue?(Y/N)"); 
					r = read(0, conti, 2); 
					if(conti[0]=='N' || conti[0]=='n') 
						break; 
				} 
				break; 
			case '2': 
				while(1) 
				{ 
					printf("\nPlease input the year which needs searched?(XXXX)"); 
					r = read(0, Syear, 5);
					year = getNum(Syear);
					if(leap(year)) 
						printf("\n%d is Leap year\n", year); 
					else 
						printf("\n%d is not Leap year\n", year);
					printf("\nDo you want to continue?(Y/N)");
					r = read(0, conti, 2); 
					if(conti[0]=='N' || conti[0]=='n') 
						break; 
				} 
				break; 
			case '3': 
				while(1) 
				{ 
					printf("\nPlease input the year which needs printed(XXXX)"); 
					r = read(0, Syear, 5);
					year = getNum(Syear);
					PrintAllYear(year); 
					printf("\nDo you want to continue to print(Y/N)?"); 
					r = read(0, conti, 2); 
					if(conti[0]=='N' || conti[0]=='n') 
						break;
				} 
				break; 
			case '4': 
				exit(0); 
				break; 
			default: 
				printf("\nError:Sorry,there is no this service now!\n"); 
				break; 
		} 
	} 
	return 0;
}
