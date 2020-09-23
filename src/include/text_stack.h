#ifndef __TEXT__STACK__H__
#define __TEXT__STACK__H__



/*
 * 	结构体不完全声明
 */
struct file_stack ;

/* 
 *	申请一个新的栈对象
 *	返回一个栈对象
 */
extern struct file_stack* FileStackNew(void);

/*
 *	释放分配的对象
 *
 */
extern void FileStackDel(struct file_stack*ptFileStack);


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
extern int FilePush(struct file_stack*  ptFileStack, void *vDate,unsigned long iLen);

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
extern int FilePop(struct file_stack*  ptFileStack,void *vDate,unsigned long iLen);

#endif /*__TEXT__STACK__H__*/

