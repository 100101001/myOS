
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
PUBLIC void welcome();
PUBLIC void animation();
void ls(char * filepath);
void cd(char * filepath);
void clearArr(char *arr, int length);
void createFile(char * filepath, char * buf);
void getwholepath(char * filename);
void createdirectory(char * filepath);
void readFile(char * filepath);
void colorful();
void help();
void readDirectory(char * filepath);
PUBLIC int proc_detail();
PUBLIC void clear();
char location[128] = "/";
char wholefilepath[128] = "";

/*****************************************************************************
 *                               kernel_main
 *****************************************************************************/
/**
 * jmp from kernel.asm::_start. 
 * 
 *****************************************************************************/
PUBLIC int kernel_main()
{
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		 "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	int i, j, eflags, prio;
        u8  rpl;
        u8  priv; /* privilege */

	struct task * t;
	struct proc * p = proc_table;

	char * stk = task_stack + STACK_SIZE_TOTAL;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++,t++) {
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p->p_flags = FREE_SLOT;
			continue;
		}

	        if (i < NR_TASKS) {     /* TASK */
                        t	= task_table + i;
                        priv	= PRIVILEGE_TASK;
                        rpl     = RPL_TASK;
                        eflags  = 0x1202;/* IF=1, IOPL=1, bit 2 is always 1 */
			prio    = 15;
                }
                else {                  /* USER PROC */
                        t	= user_proc_table + (i - NR_TASKS);
                        priv	= PRIVILEGE_USER;
                        rpl     = RPL_USER;
                        eflags  = 0x202;	/* IF=1, bit 2 is always 1 */
			prio    = 5;
                }

		strcpy(p->name, t->name);	/* name of the process */
		p->p_parent = NO_TASK;

		if (strcmp(t->name, "INIT") != 0) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p->ldts[INDEX_LDT_C].attr1  = DA_C   | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		}
		else {		/* INIT process */
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_desc(&p->ldts[INDEX_LDT_C],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_desc(&p->ldts[INDEX_LDT_RW],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}

		p->regs.cs = INDEX_LDT_C << 3 |	SA_TIL | rpl;
		p->regs.ds =
			p->regs.es =
			p->regs.fs =
			p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip	= (u32)t->initial_eip;
		p->regs.esp	= (u32)stk;
		p->regs.eflags	= eflags;

		p->ticks = p->priority = prio;

		p->p_flags = 0;
		p->p_msg = 0;
		p->p_recvfrom = NO_TASK;
		p->p_sendto = NO_TASK;
		p->has_int_msg = 0;
		p->q_sending = 0;
		p->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p->filp[j] = 0;

		stk -= t->stacksize;
	}

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	init_clock();
        init_keyboard();

	restart();

	while(1){}
}

PUBLIC int lseek(int fd, int offset, int whence)
{
	MESSAGE msg;
	msg.type   = LSEEK;
	msg.FD     = fd;
	msg.OFFSET = offset;
	msg.WHENCE = whence;

	send_recv(BOTH, TASK_FS, &msg);

	return msg.OFFSET;
}

PUBLIC int send_rece_cd(const char *pathname, int flags,int file_type)
{
	MESSAGE msg;

	msg.type	= CD;
	msg.PATHNAME	= (void*)pathname;
	msg.FLAGS	= flags;
	msg.NAME_LEN	= strlen(pathname);
	msg.FILE_TYPE = file_type;

	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.FD;
}
/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


struct time get_time()
{
	struct time t;
	MESSAGE msg;
	msg.type = GET_RTC_TIME;
	msg.BUF= &t;
	send_recv(BOTH, TASK_SYS, &msg);
	return t;
}

/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 * 
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char * filename)
{
	printf("[extract `%s'\n", filename);
	int fd = open(filename, O_RDWR,0);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);
	int i = 0;
	int bytes = 0;

	while (1) {
		bytes = read(fd, buf, SECTOR_SIZE);
		assert(bytes == SECTOR_SIZE); /* size of a TAR file
					       * must be multiple of 512
					       */
		if (buf[0] == 0) {
			if (i == 0)
				printf("    need not unpack the file.\n");
			break;
		}
		i++;

		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWR | O_TRUNC,0);
		if (fdout == -1) {
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]\n");
			close(fd);
			return;
		}
		printf("    %s\n", phdr->name);
		while (bytes_left) {
			int iobytes = min(chunk, bytes_left);
			read(fd, buf,
			     ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			bytes = write(fdout, buf, iobytes);
			assert(bytes == iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}

	if (i) {
		lseek(fd, 0, SEEK_SET);
		buf[0] = 0;
		bytes = write(fd, buf, 1);
		assert(bytes == 1);
	}

	close(fd);

	printf(" done, %d files extracted]\n", i);
}

/*****************************************************************************
 *                                shabby_shell
 *****************************************************************************/
/**
 * A very very simple shell.
 * 
 * @param tty_name  TTY file name.
 *****************************************************************************/
void shabby_shell(const char * tty_name)
{
	int fd_stdin  = open(tty_name, O_RDWR,0);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR,0);
	assert(fd_stdout == 1);

	char rdbuf[128];
	char cmd[128];
    char arg1[128];
    char arg2[128];
    char head[30]="[root@virtual_machine]";
    int root_num=1;
	clear();	
	colorful();
	animation();
	welcome();
	currentTime();
	printf("press any key to start:\n");
	int r = read(0, rdbuf, 70);
      
	while (1) {

		write(1, head, strlen(head));
		write(1, location, strlen(location));
		write(1, "$ ", 2);
		int r = read(0, rdbuf, 70);
		rdbuf[r] = 0;

		int argc = 0;
		char * argv[PROC_ORIGIN_STACK];
		char * p = rdbuf;
		char * s;
		int word = 0;
		char ch;
		do {
			ch = *p;
			if (*p != ' ' && *p != 0 && !word) {
				s = p;
				word = 1;
			}
			if ((*p == ' ' || *p == 0) && word) {
				word = 0;
				argv[argc++] = s;
				*p = 0;
			}
			p++;
		} while(ch);
		argv[argc] = 0;
		int fd = open(argv[0], O_RDWR,0);
		if (fd == -1) {
			int i=0,j=0;
			memset(cmd, 0, 12);
			memset(arg1, 0, 12);
			memset(arg2, 0, 12);
			while(rdbuf[i]!=' ' && rdbuf[i]!=0)
			{
				//printf("cmd:%c\n", rdbuf[i]);
				cmd[i]=rdbuf[i];
				i++;
			}
			cmd[i]=0;
			i++;
			while(rdbuf[i]!=' ' && rdbuf[i]!=0)
			{
				//printf("arg1:%c\n", rdbuf[i]);
				arg1[j]=rdbuf[i];
				i++;
				j++;
			}
			i++;
			j=0;
			while(rdbuf[i]!=' ' && rdbuf[i]!=0)
			{
				//printf("arg2::%c\n", rdbuf[i]);
				arg2[j]=rdbuf[i];
				j++;
				i++;
			}
			//printf("cmd%s\n", cmd);
			//printf("arg1%s\n", arg1);
			//printf("arg2%s\n", arg2);
			if(strcmp(cmd,"ls")==0)
			{
				if(root_num==0 && strlen(location)==1){
					ls(location);
					ls(location);
					ls(location);
					ls(location);
					ls(location);
					root_num=1;
				}
				ls(location);
			}
			else if(strcmp(cmd,"mkdir")==0)
			{
				getwholepath(arg1);
				//printf("%s\n", wholefilepath);
				createdirectory(wholefilepath);
				clearArr(wholefilepath, 128);
			}
			else if(strcmp(cmd,"vim")==0)
			{
				getwholepath(arg1);
				//printf("%s\n", wholefilepath);
				createFile(wholefilepath,arg2);
				clearArr(wholefilepath, 128);
			}
			else if(strcmp(cmd,"cd")==0)
			{
				if(strcmp(arg1,"..")==0)
				{
					if(strlen(location)==1)
					{
						printf("you are already at root");
					}
					else
					{
						int first=0;
						for(int i=strlen(location)-1;i>=0;i--)
						{
							if(location[i]=='/')
							{
								first++;
							}
							if(first<2)
							{
								location[i]=0;
							}
							else
							{
								printf("now location is %s\n", location);
								break;
							}
						}
					}
				}
				else
				{
					getwholepath(arg1);
					cd(wholefilepath);
					clearArr(wholefilepath, 128);
				}
				
			}
			else if(strcmp(cmd, "cat") == 0)
			{
				getwholepath(arg1);
				readFile(wholefilepath);
				clearArr(wholefilepath, 128);
					//readFile(arg1);	
			}
			else if(strcmp(cmd, "readdirectory") == 0)
			{
				getwholepath(arg1);
				readDirectory(wholefilepath);
				clearArr(wholefilepath, 128);
					//readFile(arg1);	
			}
			else if(strcmp(cmd, "backtoroot") == 0)
			{
				memset(location,0,128);
				location[0]='/';
				printf("location is %s\n",location);
				//readFile(arg1);	
			}
			else if(strcmp(cmd, "rm-f") == 0)
			{
				getwholepath(arg1);
				deleteFileorDir(wholefilepath,0);
				clearArr(wholefilepath, 128);
				//readFile(arg1);	
			}
			else if(strcmp(cmd, "rm-d") == 0)
			{
				getwholepath(arg1);
				deleteFileorDir(wholefilepath,1);
				clearArr(wholefilepath, 128);
			}
			else if(strcmp(cmd, "change") == 0)
			{
				getwholepath(arg1);
				clearandedit(wholefilepath,arg2);
				clearArr(wholefilepath, 128);
			}
			else if(strcmp(cmd, "vi-A") == 0)
			{
				getwholepath(arg1);
				AppandStrToFile(wholefilepath,arg2);
				clearArr(wholefilepath, 128);
			}
			else if(strcmp(cmd, "cls") == 0)
			{
				clear();
			}
			else if(strcmp(cmd, "help") == 0)
			{
				help();
			}
			else if(strcmp(cmd, "octocat") == 0)
			{
				animation();
			}
			else if(strcmp(cmd, "time") == 0)
			{
				currentTime();
			}
			else if(strcmp(cmd, "welcome") == 0)
			{
				welcome();
			}
			else if(strcmp(cmd, "ps") == 0)
			{
				proc_detail();
			}
			else if(strcmp(cmd, "colorful") == 0)
			{
				colorful();
			}
			else
			{
				printf("no such command\n");
			}

		}
		else {
			close(fd);
			int pid = fork();
			if (pid != 0) { /* parent */
				int s;
				wait(&s);
			}
			else {	/* child */
				execv(argv[0], argv);
			}
		}
	}

	close(1);
	close(0);
}

/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 * 
 *****************************************************************************/
void Init()
{
	int fd_stdin  = open("/dev_tty0", O_RDWR,0);
	assert(fd_stdin  == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR,0);
	assert(fd_stdout == 1);

	printf("Init() is running ...\n");

	/* extract `cmd.tar' */
	untar("/cmd.tar");
			

	char * tty_list[] = {"/dev_tty0"};

	int i;
	for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
		int pid = fork();
		if (pid != 0) { /* parent process */
			printf("[parent is running, child pid:%d]\n", pid);
		}
		else {	/* child process */
			printf("[child is running, pid:%d]\n", getpid());
			close(fd_stdin);
			close(fd_stdout);
			
			shabby_shell(tty_list[i]);
			assert(0);
		}
	}

	while (1) {
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}

	assert(0);
}


/*======================================================================*
                              ls
 *======================================================================*/
void ls(char * filepath)
{
	//printf("ls:current location is %s\n", filepath);
	int fd = -1;
	int n;
	int i=0;
	char directorylocation[30];
	for(i=0;i<strlen(filepath);i++)
	{

		directorylocation[i]=filepath[i];
	}
	directorylocation[i-1]=0;
	//printf("ls:open directory path is %s\n", directorylocation);
	fd = open(directorylocation, O_RDWR,1);
	if(fd == -1)
	{
		printf("Fail, please check and try again!!\n");
		return;
	}
	if(fd == -3)
	{
		return;
	}
	close(fd);
}
/* Appand */
void AppandStrToFile(char * filepath, char * buf)
{
	
	int fd = -1;
	int n, i = 0;
	char bufr[1024] = "";
	char empty[1024];
	
	for (i = 0; i < 1024; i++)
		empty[i] = '\0';
	fd = open(filepath, O_RDWR,0);
	if(fd == -1)
	{
		printf("please check all information is right\n");
		return;
	}

	n = read(fd, bufr, 1024);
	n = strlen(bufr);
	
	for (i = 0; i < strlen(buf); i++, n++)
	{	
		bufr[n] = buf[i];
		bufr[n + 1] = '\0';
	}
	write(fd, empty, 1024);
	fd = open(filepath, O_RDWR,0);
	write(fd, bufr, strlen(bufr));
	close(fd);
	printf("edit succeed \n");
}

/* Edit File Cover */
void clearandedit(char * filepath, char * buf)
{
	
	int fd = -1;
	int n, i = 0;
	char bufr[1024] = "";
	char empty[1024];
	
	for (i = 0; i < 1024; i++)
		empty[i] = '\0';

	fd = open(filepath, O_RDWR,0);
	if (fd == -1)
	{

		printf("please check all information is right\n");
		return;
	}
	write(fd, empty, 1024);
	close(fd);
	fd = open(filepath, O_RDWR,0);
	write(fd, buf, strlen(buf));
	close(fd);
	printf("edit succeed \n");
}

/*======================================================================*
                              getwholepath
 *======================================================================*/
void getwholepath(char * filename)
{
	int i=0, j=0;	
	for (i=0; i<strlen(location);i++)
	{
		wholefilepath[i] = location[i];
	}
	for(j = 0; j < strlen(filename); j++, i++)
	{	
		wholefilepath[i] = filename[j];
	}
	wholefilepath[i] = '\0';
}
/* Create Directory */
void createdirectory(char * filepath)
{
	
	int fd = -1, i = 0, pos;
	fd = open(filepath, O_CREAT | O_RDWR,1);
	printf("directory name: %s \n", filepath);
	if(fd == -1)
	{
		printf("Fail, please check and try again!!\n");
		return;
	}
	if(fd == -2)
	{
		printf("Fail, directory exsists!!\n");
		return;
	}
	close(fd);

		
}
/* Create file */
void createFile(char * filepath, char * buf)
{
	int fd = -1;
	fd = open(filepath, O_CREAT | O_RDWR,0);
	printf("file name: %s content: %s\n", filepath, buf);
	if(fd == -1)
	{
		printf("Fail, please check all information!!\n");
		return;
	}
	printf("content %s\n", buf);
	write(fd, buf, strlen(buf));
	close(fd);
	

		
}
/* Create file */
void deleteFileorDir(char * path, int isDir)
{
	if(unlink(path,isDir) == 0)
	{
		if(isDir)
			printf("directory removed: %s\n",path);
		else
			printf("file removed: %s\n",path);
		return;
	}
	else
	{
		if(isDir)
			printf("directory removed fail: %s\n",path);
		else
			printf("file removed fail: %s\n",path);
		return;
	}

		
}
void clearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}
/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	for(;;);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	for(;;);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	for(;;);
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}
void readFile(char * filepath)
{
	
	int fd = -1;
	int n;
	char bufr[1024] = "";
	fd = open(filepath, O_RDWR,0);
	if(fd == -1)
	{
		printf("Fail, please check all information!!\n");
		return;
	}
	if(fd == -5)
	{
		printf("you can not read a directory!!\n");
		return;
	}
	n = read(fd, bufr, 1024);
	bufr[n] = '\0';
	printf("%s(fd=%d) : %s\n", filepath, fd, bufr);
	close(fd);
}
void readDirectory(char * filepath)
{
	
	int fd = -1;
	int n;
	fd = open(filepath, O_RDWR,1);
	if(fd == -1)
	{
		printf("Fail, please check all information!!\n");
		return;
	}
	close(fd);
}
/*======================================================================*
                              cd
 *======================================================================*/
void cd(char * filepath)
{
	int fd = -1;
	fd = send_rece_cd(filepath,O_RDWR,1);
	//printf("file name: %s content: \n",filepath);
	if(fd == -1)
	{
		printf("Fail, please check and try again!!\n");
		return;
	}
	if(fd == -3)
	{
		printf("Fail, you can only cd directory!!\n");
		return;
	}
	int i;
	for(i=0;i<strlen(filepath);i++)
		location[i]=wholefilepath[i];
	location[i]='/';
	location[i+1]=0;
	printf("location:%s\n", location);
	close(fd);
}

PUBLIC void welcome()
{

	printf("              ******************************************************\n");
	printf("              *                                                    *\n");
	printf("              *        Welcome to Our Operating System             *\n");
	printf("              *                                                    *\n");
	printf("              ******************************************************\n");
	printf("              *                                                    *\n");
	printf("              *                                                    *\n");
	printf("              *                1552729  Li Yixuan                  *\n");
	printf("              *                1652690  Su ZhaoFan                 *\n");
	printf("              *                                                    *\n");
	printf("              *                                                    *\n");
	printf("              *          enter help to get all commands            *\n");
	printf("              ******************************************************\n\n");
	milli_delay(8000);
}

PUBLIC void animation()
{
	int dTime = 50;
	clear();	
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("                                                                      \n");
printf("                                                                                      \n");
printf("               BBBBdv                                           iUDBBBv               \n");
milli_delay(dTime);
printf("              rBBQBBBBBgi                                    sBBBBBBBBB               \n");
milli_delay(dTime);
printf("              BBQBBBQBBBBBq                               iBBBBBBBBBBBBr              \n");
milli_delay(dTime);
printf("              BBBBBBBBBQBBBBQ   ruDBBBBBBBBBBBBBBBBBMVv vQBBBBBBBBBBBBBD              \n");
milli_delay(dTime);
printf("              BBBBQBQBBBBBBBBBBBQBBBQBBBBBBBBBBBBBBBBBBBBBBBBBBBQBBBBBBB              \n");
milli_delay(dTime);
printf("             iBBBBBBBQBBBBBBBBBQBBBBBBBBBBBQBBBBBQBBBQBBBBBBBBBBBBBBBBBB              \n");
milli_delay(dTime);
printf("             iBBBBBBQBBBBBBBQBBBBBBBQBBBBBQBBBBBQBBBBBBBBBBBBBBBBBBBBBQB              \n");
milli_delay(dTime);
printf("              BBBBBBBQBBBBBBBBBBBBBBBBBBBBBQBBBQBBBBBBBBBQBBBBBBBBBBBQBB              \n");
milli_delay(dTime);
printf("              BBBBBBBBBBQBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBQBBBBBBBBBBBBv              \n");
milli_delay(dTime);
printf("              vQBBBBBBBQBBBBBBBBBBBBBBBQBBBBBBBBBBBBBBBBBBBBBBBBBBBQBQBi              \n");
milli_delay(dTime);
printf("              uBBBBBBBBBBBBBBBQBBBBBQBBBBBBBBBBBBBBBBBBBBBBBQBBBBBBBBBBBv             \n");
milli_delay(dTime);
printf("             gBBBBBBBQBBBBBBBBBBBBBQBBBBBBBBBBBBBBBBBBBBBBBQBQBBBBBQBBBBBI            \n");
milli_delay(dTime);
printf("            gBBBBBBBBBBBBBBBBBBBBBBBQBBBBBBBBBBBBBBBBBBBQBBBBBBBQBBBBBBBBBs           \n");
milli_delay(dTime);
printf("           uBBBBBBBBBBBBBBBQBQBBBQBQBBBBBBBQBBBQBBBBBQBBBBBBBBBBBBBBBQBBBBB           \n");
milli_delay(dTime);
printf("           BBBBBBBBBQBBBQBBBBBBBBBQBBBQBBBQBBBBBBBBBQBBBQBBBBBQBBBBBBBBBBBQB          \n");
milli_delay(dTime);
printf("          BQBBBBBBBQBBBQBBBBBBBBBBBBBBBBBBBQBBBQBQBQBBBBBBBBBBBBBBBBBBBBBBBBr         \n");
milli_delay(dTime);
printf("          BBBBBBBBBBBBBBBBBBBBBBBBBBQBBBBBBBBBQBBBBBQBBBBBQBBBBBBBBBBBQBQBBBB         \n");
milli_delay(dTime);
printf("         LBQBBBBBBBBBBBBBQBBBBBQBBBBBQBQBBBBBQBBBBBBBQBBBQBBBBBBBBBBBQBBBBBBB         \n");
milli_delay(dTime);
printf("         RBBBBBBBBBBBBBBQBRPuJLYYuUqPgMBBBBBBBBBMgPKkuLLvYJXgBBBBBBBBBBBBBBBBi        \n");
milli_delay(dTime);
printf("         BBBBBBBBQBBBBBKi                                      uBBBBBBBBBBBBBv        \n");
milli_delay(dTime);
printf("         BQBBBBBBBBBBB                                   i       gBBBBBBBBBBBs        \n");
milli_delay(dTime);
printf("         QBBBBBQBBBBP     ii    i                     iii   i     uBBBBBBBBBBv        \n");
milli_delay(dTime);
printf("         BBBBBBBBBBd     i       i                   ii            JBBBBBBBBBv        \n");
milli_delay(dTime);
printf("         DBBBBBQBBB     i    i    i                  i    i    i    MBBBBBBQBi        \n");
milli_delay(dTime);
printf("         UQBBBBBBBi    i   iPdbS   i                i   JEPEv        BBBBBBBBi        \n");
milli_delay(dTime);
printf("         iBQBBBQBB         Eksuqk  i                i  vdjuuZi  i    BBQBBBBB         \n");
milli_delay(dTime);
printf("          BBBBBBBB     i  vPssLuP  i               i   PUsYJkV  ii   gBBBBQBB         \n");
milli_delay(dTime);
printf("          MBBBBBBB     i  JSJvsjP   i               i  KuLsYII  i    MBBBBBQg         \n");
milli_delay(dTime);
printf("          vBBBBBBB     i  idjssIq  i                i  ISJYJKY  i    BBBBBBBY         \n");
milli_delay(dTime);
printf("iiiiiiiii  QBBBBBBi    i   kdIXdi  i                i   EKVKP   i    BBBBQBB  iiiiiiii\n");
milli_delay(dTime);
printf("           vQBBBBBP    ii   rksi   i                 i   YUv   i    iBBBBBBg          \n");
milli_delay(dTime);
printf("Lvvrriiiii  vQBBBQB     ii        i       rPj                 i i   BBBBBBB  iiiiirrvr\n");
milli_delay(dTime);
printf("             kBBBBQB     ii     ii i      rZj         ii     i     rBBBBBB            \n");
milli_delay(dTime);
printf("              vBBBBBB      iiiiii                       iiiii     iBBBBBB             \n");
milli_delay(dTime);
printf("                BBBBBBr                 iu   vY                  vBBBBQB              \n");
milli_delay(dTime);
printf("                 vBBBBBQv                rJvYv                 iBQBBBBL               \n");
milli_delay(dTime);
printf("                   rBBBBBBQji                               iXBBBBBBv                 \n");
milli_delay(dTime);
printf("                      sQBBBBBBBBduviiii           iiiivubQBBBBBBBI                    \n");
milli_delay(dTime);
printf("       BBribi            iYMBBBBQBBBBBBBBBBBBBBBBBBBQBBBBBQBQIi                       \n");
milli_delay(dTime);
printf("       UBBBBis                   iRQBBBBBBBBBQBQBBBBBbiii                             \n");
milli_delay(dTime);
printf("         kBBqRdi                 sBBQBBBBBBBBBBBQBBBBBi                               \n");
milli_delay(dTime);
printf("       i   BBuvQ                BBBBBBBBBBBBBBBBBBBBBBBd                              \n");
milli_delay(dTime);
printf("       r    BBB Qi             QBBBBBBBBBBQBBBBBBBBBBBQBL                             \n");
milli_delay(dTime);
printf("             BBgEKv            BBBBBBBBBBQBBBBBQBBBQBQBBB                             \n");
milli_delay(dTime);
printf("             LBBsiBEr       ivBBBBBBBBBBBBBJSBBBBBBBBBBBQi                            \n");
milli_delay(dTime);
printf("              BBBQu BBsBPrBEiQBBBBBBiYBBBBB  BBBBBrPBBBBBU                            \n");
milli_delay(dTime);
printf("               BBBBBBX BgiBBUBQBBBBB jBBBBB  BBBBB  BBQBQq                            \n");
milli_delay(dTime);
printf("                BBBBBQBBBBBBBBBBBBBu XBBBBB  BBBBBi BBBBBX                            \n");
milli_delay(dTime);
printf("                 rBBQBBBBBBBBBQBBBQv PBBQBB  BBBBBi DBBBBq                            \n");
milli_delay(dTime);
printf("                    vEBBBBBBRjBBBBBv KBBBBB  BBBBBi QQBBBS                            \n");
milli_delay(dTime);
printf("                              MBBBBY PBBBBB  BBBBBi MBBBBK                            \n");
milli_delay(dTime);
printf("                              BBBBBL KBBBBB  BBBBBi QBBBBX                            \n");
milli_delay(dTime);
printf("                              BBBBBs PBBBBB  BBBBBi RBBBBq                            \n");
milli_delay(dTime);
printf("                              BQBBBv KBBBBB  BBBBBi QBBBBX                            \n");
milli_delay(dTime);
printf("                              BBBBBY PBBQBQ  BBBBBi MBBBQK                            \n");
milli_delay(dTime);
printf("                              BBBBBL KBBBBB  BBBBQi QBBBBk                            \n");
milli_delay(dTime);
printf("                              BBBBBS DBBBBQ iBBBBBv BBBBBq                            \n");
milli_delay(dTime);
printf("                          i  iBBBBBY dBBBBB iBBBBBL MBBBBQ  iii                       \n");
milli_delay(dTime);
printf("                     iiiii   BBBBBB  BBBBBB  BQBBBK iBBBBBg   iiiiii                  \n");
milli_delay(dTime);
printf("                  iiiiiii ikBQBBBBi iBBQBBv  PBBBBB  rBBBBBQJ   iiiiiii               \n");
milli_delay(dTime);
printf("               iiiiiiiii  BBBBBDv   BBBBBP    BBQBQq   JMBBBBB i iiiiiiii             \n");
milli_delay(dTime);
printf("              iiiiiii ii  rvii     BBBBBY   i  SBBBBQ     iivi  i i i iiiii           \n");
milli_delay(dTime);
printf("             iii iiiii i   iiiiii iBBMv    i i   jQBQ iiiiii i iii i iiiiiii          \n");
milli_delay(dTime);
printf("            iiiiiiiiiii i   iiiiiiii      iii       iiiiiii   iii i iii iiii          \n");
milli_delay(dTime);
printf("            iiiiiiii i i ii  iiiiiiiiiiiii i  iriiiiiiiiiii  iiiii iii iiiii          \n");
milli_delay(dTime);
printf("             iiii i iii i i i iiiiiiiiiiiii  iriiii iiiiii  i i iiiiiiiiiiii          \n");
milli_delay(dTime);
printf("              iiiiiiii i i i  iiiiii iiiiri  iiiiii iiiiii iii iiiii iiiii            \n");
milli_delay(dTime);
printf("                iiiiiiiiiiiii iiiiii iiiiii iiiiiii iiiiii  i iiiiiiiiii              \n");
milli_delay(dTime);
printf("                   iiiiiiiii  iiiiii iiiiii iiiiiii iiiiii iiiiiiiiiii                \n");
milli_delay(dTime);
printf("                      iiiiiii iiiiri iriiii iiiiiii iiiiriiiiiiiii                    \n");
milli_delay(dTime);
printf("                             irvrvvr rrrirriivrriviirvrvvrii                          \n");
milli_delay(dTime);
printf("                                                                                     \n");
	printf("                                                                      \n\n");
	milli_delay(1500);                                                      
}


PUBLIC void clear()
{
	int i = 0;
	for (i = 0; i < 20; i++)
		printf("\n");
}

PUBLIC int currentTime() {
	struct time t = get_time();
	printf("%d/%d/%d %d:%d:%d\n", t.year, t.month, t.day, t.hour, t.minute, t.second);
	return 0;
}
void help()
{
	printf("********************************************************************************\n");
	printf("        name                    |                      function                      \n");
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("        help                    |           List all commands\n");
	printf("        welcome                 |           Welcome the users\n");
	printf("        octocat                 |           Play the video\n");
	printf("        ls                      |           List all files in current file path\n");
	printf("        cd [dir]                |           Go into the dir\n");
	printf("        vim [file] [str]        |           Create a file\n");
	printf("        mkdir [dir]             |           Create a directory\n");
	printf("        cat [file]              |           Read a file\n");
	printf("        rm-f  [file]            |           Delete a file\n");
	printf("        rm-d [dir]              |           Delete a directory\n");
	printf("        change [file][str]      |           Clear file content and write\n");
	printf("        vi-A [file][str]        |           Append content to a file\n");
	printf("        ps                      |           List process information\n");
	printf("        saolei                  |           Start game saolei\n");
	printf("        fiveChess               |           Start game fiveChess\n");
	printf("        calculator              |           Start a binary caculator\n");
	printf("        time                    |           Show current time\n");
	printf("        cls                     |           Clear the screen\n");
	printf("********************************************************************************\n");
	
}
void colorful()
{   
	int dTime = 100;
    int j = 0;
    for (j = 0; j < 1440; j++){disp_color_str("S", BLACK);}
	/* first line */
	disp_color_str("SSSSS",BLACK);
	disp_color_str("MMMMM",GREEN);
	disp_color_str("SSS",BLACK);
	disp_color_str("MMMMM",GREEN);
	disp_color_str("SSSSSS",BLACK);
	disp_color_str("WWWWWWWW",BLUE);
	disp_color_str("SSSSSSSSSSS",BLACK);
	disp_color_str("00000",RED);
	disp_color_str("SSSSSSSSSSS",BLACK);
	disp_color_str("00000",RED);
	disp_color_str("SSSSSSSSSSS",BLACK);
	disp_color_str("00000",RED);
	milli_delay(dTime);
	/* second line */
	disp_color_str("SSSSSS",BLACK);
	disp_color_str("MMM",GREEN);
	disp_color_str("SSSSS",BLACK);
	disp_color_str("MMM",GREEN);
	disp_color_str("SSSSSSSSS",BLACK);
	disp_color_str("WWWW",BLUE);
	disp_color_str("SSSSSSSSSSSSS",BLACK);
	disp_color_str("00000",RED);
	disp_color_str("SSSSSSSSSSS",BLACK);
	disp_color_str("00000",RED);
	disp_color_str("SSSSSSSSSSS",BLACK);
	disp_color_str("00000",RED);
	milli_delay(dTime);
	/* third line */
	disp_color_str("SSSSSS",BLACK);
	disp_color_str("MMM",GREEN);
	disp_color_str("SSSSS",BLACK);
	disp_color_str("MMM",GREEN);
	disp_color_str("SSSSSSSSS",BLACK);
	disp_color_str("WWWW",BLUE);
	disp_color_str("SSSSSSSSSSSSS",BLACK);
	disp_color_str("0000",RED);
	disp_color_str("SSSSSSSSSSSS",BLACK);
	disp_color_str("0000",RED);
	disp_color_str("SSSSSSSSSSSS",BLACK);
	disp_color_str("0000 ",RED);
	milli_delay(dTime);
	/* forth line */
	disp_color_str("SSSSSS",BLACK);
	disp_color_str("MMM",GREEN);
	disp_color_str("SSSSS",BLACK);
	disp_color_str("MMM",GREEN);
	disp_color_str("SSSSSSSSS",BLACK);
	disp_color_str("WWWW",BLUE);
	disp_color_str("SSSSSSSSSSSSS",BLACK);
	disp_color_str("0000",RED);
	disp_color_str("SSSSSSSSSSSS",BLACK);
	disp_color_str("0000",RED);
	disp_color_str("SSSSSSSSSSSS",BLACK);
	disp_color_str("0000 ", RED);
	milli_delay(dTime);
	/* fifth line */
	disp_color_str("SSSSSS",BLACK);
	disp_color_str("MMM",GREEN);
	disp_color_str("MMMMM",GREEN);
	disp_color_str("MMM",GREEN);
	disp_color_str("SSSSSSSSS",BLACK);
	disp_color_str("WWWW",BLUE);
	disp_color_str("SSSSSSSSSSSSS",BLACK);
	disp_color_str("000",RED);
	disp_color_str("SSSSSSSSSSSSS",BLACK);
	disp_color_str("000",RED);
	disp_color_str("SSSSSSSSSSSSS",BLACK);
	disp_color_str("000  ",RED);
	milli_delay(dTime);
	/* sixth line */
	disp_color_str("SSSSSS", BLACK);
	disp_color_str("MMM", GREEN);
	disp_color_str("MMMMM", GREEN);
	disp_color_str("MMM", GREEN);
	disp_color_str("SSSSSSSSS", BLACK);
	disp_color_str("WWWW", BLUE);
	disp_color_str("SSSSSSSSSSSSS", BLACK);
	disp_color_str("000", RED);
	disp_color_str("SSSSSSSSSSSSS", BLACK);
	disp_color_str("000", RED);
	disp_color_str("SSSSSSSSSSSSS", BLACK);
	disp_color_str("000  ", RED);
	milli_delay(dTime);
	/* seventh line */
	disp_color_str("SSSSSS", BLACK);
	disp_color_str("MMM", GREEN);
	disp_color_str("SSSSS", BLACK);
	disp_color_str("MMM", GREEN);
	disp_color_str("SSSSSSSSS", BLACK);
	disp_color_str("WWWW", BLUE);
	disp_color_str("SSSSSSSSSSSSS", BLACK);
	disp_color_str("SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS",BLACK);
	milli_delay(dTime);
	/* eighth line */
	disp_color_str("SSSSSS", BLACK);
	disp_color_str("MMM", GREEN);
	disp_color_str("SSSSS", BLACK);
	disp_color_str("MMM", GREEN);
	disp_color_str("SSSSSSSSS", BLACK);
	disp_color_str("WWWW", BLUE);
	disp_color_str("SSSSSSSSSSSSS", BLACK);
	disp_color_str("SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS", BLACK);
	milli_delay(dTime);
	/* nineth line */
	disp_color_str("SSSSSS", BLACK);
	disp_color_str("MMM", GREEN);
	disp_color_str("SSSSS", BLACK);
	disp_color_str("MMM", GREEN);
	disp_color_str("SSSSSSSSS", BLACK);
	disp_color_str("WWWW", BLUE);
	disp_color_str("SSSSSSSSSSSSS", BLACK);
	disp_color_str("000", RED);
	disp_color_str("SSSSSSSSSSSSS", BLACK);
	disp_color_str("000", RED);
	disp_color_str("SSSSSSSSSSSSS", BLACK);
	disp_color_str("000  ", RED);
	milli_delay(dTime);
	/* tenth line */
	disp_color_str("SSSSS", BLACK);
	disp_color_str("MMMMM", GREEN);
	disp_color_str("SSS", BLACK);
	disp_color_str("MMMMM", GREEN);
	disp_color_str("SSSSSS", BLACK);
	disp_color_str("WWWWWWWW", BLUE);
	disp_color_str("SSSSSSSSSSS", BLACK);
	disp_color_str("000", RED);
	disp_color_str("SSSSSSSSSSSSS", BLACK);
	disp_color_str("000", RED);
	disp_color_str("SSSSSSSSSSSSS", BLACK);
	disp_color_str("000  ", RED);
    for (j = 0; j < 300; j++)
        disp_color_str(" ",BLACK);
    milli_delay(8000);
}
PUBLIC int proc_detail()
{
        struct proc * p = proc_table;
        int i;
    /* System Process */
        printf("************************* System Process *************************\n");
        for (i = 0; i < NR_TASKS; i++,p++) {                     
            printf("    pid: %d    Name: %s    Priority: %d \n", i, p->name, p->priority);
        }
    printf("\n");
    /* User Process */   
        printf("************************** User Process **************************\n");     
        for (; i < NR_TASKS + NR_NATIVE_PROCS; i++,p++) {
            printf("    pid: %d    Name: %s    Priority: %d    Running Status: %d\n", i, p->name, p->priority, p->p_flags);
        }
        return 0;
}
