/*
 *	本文件提供给text_ctrl使用
 *	应用场景,当数据量比较大时使用该文件
 *	本文件提供一个无限大的栈，当然在磁盘
 *	充足的情况下
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <text_stack.h>


/* 命名计数，防止在多个文件使用时文件名冲突 */
static unsigned long name_count=0;
#define FILE_NAME	"file_stack"
#define MODULE_NAME "text_stack"

#define FILE_NAME_SIZE 	40
#define BUF_SIZE		1024



/*
 *	file_stack 对象结构
 *	filename	用于缓冲的文件名
 *	file_desc	文件描述符
 *	top			栈顶指针
 *	buf			缓冲区
 */
struct file_stack {
	char filename[FILE_NAME_SIZE];
	int file_desc;
	unsigned long top;
	char buf[BUF_SIZE];
};



static int FileWriteSync(struct file_stack*  ptFileStack)
{
	int file_desc = ptFileStack->file_desc;
	char *buf = ptFileStack->buf;
	int err;
	lseek(file_desc,ptFileStack->top - BUF_SIZE, SEEK_SET);
	err = write(file_desc, buf, BUF_SIZE);
	if(err != BUF_SIZE)
	{
		printf(MODULE_NAME": write error\n");
		return -1;
	}
	return 0;
}
static int FileReadSync(struct file_stack*  ptFileStack)
{
	int file_desc = ptFileStack->file_desc;
	char *buf = ptFileStack->buf;
	int err;
	lseek(file_desc,ptFileStack->top - BUF_SIZE , SEEK_SET);
	err = read(file_desc, buf, BUF_SIZE);
	if(err != BUF_SIZE)
	{
		printf(MODULE_NAME": read error\n");
		return -1;
	}
	return 0;
}





/* 
 *	申请一个新的栈对象
 *	返回一个栈对象
 */
struct file_stack* FileStackNew(void)
{
	struct file_stack* ptFileStack;
	int file_desc;
	ptFileStack = (struct file_stack*)malloc(sizeof(struct file_stack));
	if(ptFileStack == NULL)
	{
		printf(MODULE_NAME": Space allocation failure\n");
		return NULL;
	}
	/* 
	 *	希望这里不要溢出,不过即使数组溢出一般也不影响程序，
	 *	后面还有1024的缓冲区 
	 */
	sprintf(ptFileStack->filename,FILE_NAME"%lu.stack",name_count);
	ptFileStack->filename[FILE_NAME_SIZE-1] = '\0';
	file_desc = open(ptFileStack->filename,O_RDWR|O_CREAT);
	if(file_desc < 0)
	{
		printf(MODULE_NAME": File creation failed\n");
		return NULL;
	}
	ptFileStack->file_desc=file_desc;
	ptFileStack->top = 0;
	name_count++;
	return ptFileStack;
}

void FileStackDel(struct file_stack*ptFileStack)
{
	close(ptFileStack->file_desc);
	remove(ptFileStack->filename);
	free(ptFileStack);
}





 


/*
 *	圧栈函数
 *	参数：
 *		ptFileStack		对象指针
 *		date			数据的指针
 *		len				数据的长度
 *	返回值：
 *		成功返回0
 *		失败返回-1
 */
int FilePush(struct file_stack*  ptFileStack, void *vDate,unsigned long iLen)
{
	unsigned char* date = (unsigned char*)vDate;
	unsigned long top = ptFileStack->top;
	char *buf = ptFileStack->buf;
	unsigned long buf_residue ;
	unsigned long buf_write;
	unsigned long buf_write_len;
	
	if(iLen > BUF_SIZE/2 ) return -1;
	
	buf_write = top%BUF_SIZE;
	buf_residue = BUF_SIZE - buf_write;
	buf_write_len = buf_residue >=  iLen ? iLen: buf_residue;
	memcpy(buf+buf_write, date, buf_write_len);
	top += buf_write_len;
	ptFileStack->top = top;
	if(top%BUF_SIZE == 0  && FileWriteSync(ptFileStack) == -1)
	{
		printf(MODULE_NAME": Synchronization failure\n");
		return -1;
	}
	
	date += buf_write_len;
	iLen -= buf_write_len;
	memcpy(buf+top%BUF_SIZE,date,iLen);
	top += iLen;
	ptFileStack->top = top;
	
	return 0;
}


/*
 *	出栈函数
 *	参数：
 *		ptFileStack		对象指针
 *		date			数据的指针
 *		len				数据的长度
 *	返回值：
 *		成功返回0
 *		失败返回-1
 *		空栈返回1
 */
int FilePop(struct file_stack*  ptFileStack,void *vDate,unsigned long iLen)
{
	unsigned char* date = (unsigned char*)vDate;
	unsigned long top = ptFileStack->top;
	char *buf = ptFileStack->buf;
	unsigned long buf_read_len;
	unsigned long buf_residue ;

	
	if(iLen > BUF_SIZE/2 || iLen > top) return -1;
	if(top == 0)	return 1;
	
	buf_residue = top%BUF_SIZE;
	buf_read_len = iLen < buf_residue ? iLen : buf_residue;
	memcpy(date + (iLen - buf_read_len), buf + (buf_residue - buf_read_len), buf_read_len);
	top -= buf_read_len;
	ptFileStack->top = top;
	if(top%BUF_SIZE == 0 && top!=0 && FileReadSync(ptFileStack) == -1)
	{		
		printf(MODULE_NAME": Synchronization failure\n");
		return -1;
	}
	buf_read_len = iLen - buf_read_len;
	//printf("buf_read_len = %lu\n",buf_read_len);
	memcpy(date,buf + (BUF_SIZE - buf_read_len),buf_read_len);
	top -= buf_read_len;
	ptFileStack->top = top;
	return 0;
}







