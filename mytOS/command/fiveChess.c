#include <stdio.h>

unsigned char data[64];  //��¼���̣�0Ϊ���ӣ�1Ϊ���ӣ�2Ϊ���� 
unsigned char data_record[64][8][2];  //��¼����ÿ��������������ᡢ������б����б ��4������8��ֵ,��ά���飬���һά�Ǳ�ʾ��ɫ 
int value_standard[5]={1,100,400,2000,10000};    //��0�ӵ�������������ֱ�׼
long data_calcuate[64];

unsigned char sdata(int x, int y)  //��x��y����Ѱ������ֵ 
{
	return data[8*x+y];
}

void wdata(int x, int y, int i, unsigned char color, unsigned char d)  //д��4����������ֵ 
{
	int m;
	m = x*8+y;
	data_record[m][i][color-1] = d;
}

int check_Result(unsigned char color) //����Ƿ�Ӯ�� 
{
	int i,j;
	for(i=0;i<8;i++)  //�жϺ��� 
	{
		for(j=0;j<4;j++)
		{
			if(color==sdata(i,j)&&color==sdata(i,j+1)
			&&color==sdata(i,j+2)&&color==sdata(i,j+3)
			&&color==sdata(i,j+4)) 
				return 1;
		}
	}
	
	for(i=0;i<4;i++)  //�ж����� 
	{
		for(j=0;j<8;j++)
		{
			if(color==sdata(i,j)&&color==sdata(i+1,j)
			&&color==sdata(i+2,j)&&color==sdata(i+3,j)
			&&color==sdata(i+4,j))
				return 1;	
		}	
	}
	
	for(i=0;i<4;i++) //�жϡ�/�� ���� 
	{
		for(j=7;j>3;j--)
		{
			if(color==sdata(i,j)&&color==sdata(i+1,j-1)
			&&color==sdata(i+2,j-2)&&color==sdata(i+3,j-3)
			&&color==sdata(i+4,j-4))
				return 1;
		}
	}
	
	for(i=0;i<4;i++) //�жϡ�\������
	{
		for(j=0;j<4;j++) 
		{
			if(color==sdata(i,j)&&color==sdata(i+1,j+1)
			&&color==sdata(i+2,j+2)&&color==sdata(i+3,j+3)
			&&color==sdata(i+4,j+4))
				return 1;
		}
	} 
	
	return 0;
}

void evaluate(unsigned char color) //������������ÿ��������� 
{
	int a,b,c;
	unsigned char d;
	for(a=0;a<8;a++) 
	{
		for(b=0;b<8;b++)
		{
			if(sdata(a,b)==0)//�հ׵�
			{
				d=0;
				for(c=1;c<5;c++) //�������� 
				{
					if(b==0||sdata(a,b-c)!=color)   break;
					else d++;
					if(b-c==0) break;
				}
				wdata(a,b,0,color,d);	
				
				d=0;
				for(c=1;c<5;c++)  //��������
				{
					if(b==7||sdata(a,b+c)!=color)   break;
					else d++;
					if(b+c==7)  break; 
				} 
				wdata(a,b,1,color,d);
				
				d=0;
				for(c=1;c<5;c++)   //��������
				{
					if(a==0||sdata(a-c,b)!=color)  break;
					else d++;
					if(a-c==0)  break;
				} 
				wdata(a,b,2,color,d);
				
				d=0;
				for(c=1;c<5;c++)  //�������� 
				{
					if(a==7||sdata(a+c,b)!=color)  break;
					else d++;
					if(a+c==7)  break;
				}
				wdata(a,b,3,color,d);
				
				d=0;
				for(c=1;c<5;c++)  //��\�������� 
				{
					if(a==0||b==0||sdata(a-c,b-c)!=color)  break;
					else d++;
					if(a-c==0||b-c==0)  break;
				}
				wdata(a,b,4,color,d);
				
				d=0;
				for(c=1;c<5;c++)  //��\��������
				{
					if(a==7||b==7||sdata(a+c,b+c)!=color)   break;
					else d++;
					if(a+c==7||b+c==7)   break;
				} 
				wdata(a,b,5,color,d);
				
				d=0;
				for(c=1;c<5;c++)  //��/�������� 
				{
					if(a==0||b==7||sdata(a-c,b+c)!=color)  break;
					else d++;
					if(a-c==0||b+c==7)  break;
				}
				wdata(a,b,6,color,d);
				
				d=0;
				for(c=1;c<5;c++)  //��/��������
				{
					if(a==7||b==0||sdata(a+c,b-c)!=color)  break;
					else d++;
					if(a+c==7||b-c==0)  break;
				} 
				wdata(a,b,7,color,d);
			}
		}
	}
}

long calculate(unsigned char color ,int *px, int *py) //����������������ֵ�����ض�Ӧ������ 
{
	int x,y;
	long max_value=0;
	long temp_value=0; 
	for(x=0;x<8;x++)
	{
		for(y=0;y<8;y++)
		{
			if(sdata(x,y)==0)
			{
				int i;
				temp_value=0;
				for(i=0;i<8;i++)
				{
					temp_value=temp_value+value_standard[data_record[8*x+y][i][color]];
				}
				if(x==0||x==7||y==0||y==7)  temp_value=temp_value-1;
				if(temp_value>max_value)
				{
					max_value=temp_value;
					*px=x;
					*py=y;
				}
			}
		}
	}
	return max_value;
}
 
void print_chess()
{
	printf("    0 1 2 3 4 5 6 7\n");
	printf("  ------------------\n");
	int x,y;
	for(x=0;x<8;x++)
	{
		printf("%d : ", x);
		for(y=0;y<8;y++)
		{
			if(data[x*8+y] == 0)
				printf("- ");
			else
				printf("%d ", data[x*8+y]);
		}
		printf("\n");
	}
} 

int main()
{
	int temp_x, temp_y; 
	long baizi_value, heizi_value;
	int bx, by, hx, hy;
	char bufr[2];
	int r;
	int i;
	for(i=0; i<64; i++)
		data[i] = 0;
	
	printf("******************************************************\n");
	printf("*                   Five Chess                       *\n");
	printf("******************************************************\n");
	printf("*     1. 1 refers to user, 2 refers to computer      *\n");
	printf("*     2. - refers to available place                 *\n");
	printf("*     3. Enter row number 0-7                        *\n");
	printf("*     4. Enter col number 0-7                        *\n");
	printf("*     5. Five chess in one line then you win         *\n");
	printf("*     6. Enter q to quit                             *\n");
	printf("******************************************************\n\n");
	
	print_chess();
	
	while(check_Result(1)!=1 && check_Result(2)!=1)
	{
		printf("\nPlease enter row number 0-7: ");
		r = read(0, bufr, 2);
		if(bufr[0] == 'q')
			return 0;
		temp_x = bufr[0] - '0';
		while(temp_x<0 || temp_x>7)
		{
			printf("Wrong input!");
			printf("\nPlease enter row number 0-7: ");
			r = read(0, bufr, 2);
			if(bufr[0] == 'q')
				return 0;
			temp_x = bufr[0] - '0';
		}
		
		printf("Please enter col number 0-7: ");
		r = read(0, bufr, 2);
		if(bufr[0] == 'q')
			return 0;
		temp_y = bufr[0] - '0';
		while(temp_y<0 || temp_y>7)
		{
			printf("Wrong input!");
			printf("\nPlease enter col number 0-7: ");
			r = read(0, bufr, 2);
			if(bufr[0] == 'q')
				return 0;
			temp_y = bufr[0] - '0';
		}
		
		if(data[temp_x*8+temp_y] != 0)
		{
			printf("\nThis position is not available!\n");
			continue;
		}
		data[temp_x*8+temp_y]=1;
		printf("\nYou: \n");
		print_chess();
		evaluate(1);
		evaluate(2);
		baizi_value=calculate(1,&bx,&by);
		heizi_value=calculate(2,&hx,&hy);
		printf("\nComputer: \n");
		if(heizi_value>=baizi_value)
		{
			data[hx*8+hy]=2;
			print_chess();
		}
		else
		{
			data[bx*8+by]=2;
			print_chess();
		}
		
		if(check_Result(1)==1)  
			printf("\nWow! You Win! ^_^\n");
		else if(check_Result(2)==1)   
			printf("\nNot so lucky! You Lose! -_-\n"); 
	}
	return 0;
}
