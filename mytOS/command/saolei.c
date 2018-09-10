#include<stdio.h>
#include<string.h>

#define rows 11
#define cols 11
#define Count 10

char mine[rows][cols];
char show[rows][cols];

void init()
{
    int i = 0;  
    int j = 0;  
    for (i = 0; i < rows - 1; i++)  
    {  
        for (j = 0; j < cols - 1; j++)  
        {  
            mine[i][j] = '0';  
            show[i][j] = '*';  
        }  
    }
}

//设置雷的位置  
void set_mine()  
{  
    mine[1][3] = '1';
    mine[2][6] = '1';
    mine[3][6] = '1';
    mine[4][1] = '1';
    mine[5][4] = '1';
    mine[6][8] = '1';
    mine[7][3] = '1';
    mine[8][8] = '1';
    mine[9][5] = '1';
    mine[7][9] = '1';
}  
  
//打印下棋完了显示的界面  
void display(char a[rows][cols])    
{  
    int i = 0;  
    int j = 0;
    printf("\n");  
    printf(" ");  
    for (i = 1; i < cols - 1; i++)  
    {  
        printf(" %d ", i);  
    }  
    printf("\n");  
    for (i = 1; i < rows - 1; i++)  
    {  
        printf("%d", i);  
        for (j = 1; j < cols - 1; j++)  
        {  
            printf(" %c ", a[i][j]);  
        }  
        printf("\n");  
    }  
}  
  
//计算雷的个数  
int get_num(int x, int y)  
{  
    int count = 0;  
    if (mine[x - 1][y - 1] == '1')//左上方  
    {  
        count++;  
    }  
    if (mine[x - 1][y] == '1')//左边  
    {  
        count++;  
    }  
    if (mine[x - 1][y + 1] == '1')//左下方  
    {  
        count++;  
    }  
    if (mine[x][y - 1] == '1')//上方  
    {  
        count++;  
    }  
    if (mine[x][y + 1] == '1')//下方  
    {  
        count++;  
    }  
    if (mine[x + 1][y - 1] == '1')//右上方  
    {  
        count++;  
    }  
    if (mine[x + 1][y] == '1')//右方  
    {  
        count++;  
    }  
    if (mine[x + 1][y + 1] == '1')//右下方  
    {  
        count++;  
    }  
    return  count;  
}  

//扫雷  
int sweep()  
{  
    int count = 0;  
    int x = 0, y = 0;  
    char cx[2], cy[2];
    while (count!=((rows-2)*(cols-2)-Count))  
    {  
        printf("Please input row number: ");  
        int r = read(0, cx, 2);
	if (cx[0] == 'q')
	    return 0;
	x = cx[0] - '0';
	while(x <= 0 || x > 9)
	{
	    printf("Wrong Input!\n");
	    printf("Please input row number: ");  
            r = read(0, cx, 2);
	    if (cx[0] == 'q')
	        return 0;
	    x = cx[0] - '0';
	}
        
	printf("Please input col number: ");  
        r = read(0, cy, 2);
	if (cy[0] == 'q')
	    return 0;
	y = cy[0] - '0';
	while(y <= 0 || y > 9)
	{
	    printf("Wrong Input!\n");
	    printf("Please input col number: ");  
            r = read(0, cy, 2);
	    if (cy[0] == 'q')
	        return 0;
	    y = cy[0] - '0';
	}

        if (mine[x][y] == '1')  
        {  
            printf("BOOM SHAKALAKA! Game Over!\n"); 
	    display(mine); 
            return 0;  
        }  
        else  
        {  
            int ret = get_num(x, y);  
            show[x][y] = ret + '0';  
            display(show);  
            count++;  
        }  
    }  
    printf("YOU WIN!\n");  
    display(mine);  
    return 0;  
}  

int main()  
{  
    printf("**************************************\n");
    printf("*              saolei                *\n");
    printf("**************************************\n");
    printf("*      1. 10 bombs total             *\n");
    printf("*      2. Enter 1-9 row number       *\n");
    printf("*      3. Enter 1-9 col number       *\n");
    printf("*      4. Enter q to quit            *\n");
    printf("**************************************\n");
    
    init();
    set_mine();
    display(show);
    sweep();
    
    printf("\n");
    return 0;  
}  
