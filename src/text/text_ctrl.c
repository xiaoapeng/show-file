#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <wchar.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>




#include <encoding_manager.h>
#include <text_display.h>
#include <text_ctrl.h>
#include <text_stack.h>
#include <code.h>
#define MODULE_NAME "text_ctrl" 


/* 
 *	控制字符映射结构体
 * 	code_num	映射后的编码格式
 *	target_char	映射后的编码值
 *	ascii_char	映射前的ascii
 */
struct ctrl_char{
	wchar_t target_char;
	char 	ascii_char;
};

#define StructureEntry(target_char,ascii_char)	{ (target_char), (ascii_char) }


/********************************
 *	页描述符结构体
 *		desc 		显示描述符
 *		read		读指针 在文件中的偏移值
 *		len			该页的长度
 *		is_ready	就绪标志
 ********************************/
struct page_desc{
	int desc;
	int read;
	int len;
	int is_ready;
};


/****************************************
 *	控制器结构体
 *
 *	file_desc 				文件描述符

 *
 *	coding_format			编码格式
 *	a_ctrl_char				控制字符映射表
 *	file_size				文件大小
 *	file_read				文件读写指针 即将要读的字符数
 *	file_buf				文件缓冲区，避免反复读，提高效率
 *	file_buf_read			缓冲区的第一个字符对应文件中的位置
 ****************************************/
 #define FILE_BUF_SIZE		4096
 struct text_ctrl{
	FILE* file_desc;

	struct page_desc next;	
	struct page_desc prev;
	struct page_desc current;

	struct file_stack *pt_stack;
	struct ctrl_char  *p_ctrl_char;
} ;



/*
 *	编码控制字符映射表
 *	coding_num		对应的编码
 *	coding_map		ascii与该编码的映射
 */
struct coding_ctrl_char{
	int coding_num;
	struct ctrl_char *coding_map;
	wchar_t space_char;
};

/* 支持的编码 	 如果是其他编码       	 就需要转化 */
static int g_SupportedEncoding[]={CODE_UTF16_LE,CODE_GB2312};
#define SUPPORTEENCODING_SIZE (sizeof(g_SupportedEncoding)/sizeof(g_SupportedEncoding[0]))

/* utf16le 与gb2312对控制字符的映射是一样的 */
static struct ctrl_char utf16le_Gb2312[] ={
	StructureEntry(0x01, 0x01),StructureEntry(0x02, 0x02),StructureEntry(0x03, 0x03),
	StructureEntry(0x04, 0x04),StructureEntry(0x05, 0x05),StructureEntry(0x06, 0x06),
	StructureEntry(0x07, 0x07),StructureEntry(0x08, 0x08),StructureEntry(0x09, 0x09),
	StructureEntry(0x0A, 0x0A),StructureEntry(0x0B, 0x0B),StructureEntry(0x0C, 0x0C),
	StructureEntry(0x0D, 0x0D),StructureEntry(0x0E, 0x0E),StructureEntry(0x0F, 0x0F),
	StructureEntry(0x10, 0x10),StructureEntry(0x11, 0x11),StructureEntry(0x12, 0x12),
	StructureEntry(0x13, 0x13),StructureEntry(0x14, 0x14),StructureEntry(0x15, 0x15),
	StructureEntry(0x16, 0x16),StructureEntry(0x17, 0x17),StructureEntry(0x18, 0x18),
	StructureEntry(0x19, 0x19),StructureEntry(0x1A, 0x1A),StructureEntry(0x1B, 0x1B),
	StructureEntry(0x1C, 0x1C),StructureEntry(0x1D, 0x1D),StructureEntry(0x1E, 0x1E),
	StructureEntry(0x1F, 0x1F),StructureEntry(0x7F, 0x7F),StructureEntry(0x00, 0x00),
};

static struct coding_ctrl_char  a_coding_ctrl_char[] = {
	{CODE_UTF16_LE,utf16le_Gb2312,' '},
	{CODE_GB2312,utf16le_Gb2312,' '},
	{-1,NULL,'\0'},
};

static struct coding_ctrl_char * look_for_ctrl_char(int coding)
{
	struct coding_ctrl_char * pos;
	pos = a_coding_ctrl_char;
	while(pos->coding_num != -1)
	{
		if(pos->coding_num == coding)
			return pos;
		pos++;
	}
	return NULL;
}
static char GetCtrlChar(wchar_t Coding,struct ctrl_char* p_ctrl_char)
{
	struct ctrl_char* pos;
	pos = p_ctrl_char;
	while(pos->ascii_char != 0x00 )
	{
		if(pos->target_char == Coding )
		{
			//printf("target_char = 0x%04x\n",Coding);
			//printf("ascii_char = 0x%04x\n",pos->ascii_char);
			return pos->ascii_char;
		}
		pos ++ ;
	}

	return 'N';
}


/* 从文件中获取编码 */
static wchar_t GetCoding(FILE *ptFileDesc,long int offset)
{
	wchar_t coding;
	fseek(ptFileDesc, offset, SEEK_SET);
	if(fread(&coding, sizeof(coding), 1, ptFileDesc) != 1)
	{
		return 0;
	}
	return coding;
}

#define TEXT_NEXT 		0x0
#define TEXT_PREV 		0x1
#define TEXT_CURRENT 	0x2
/* 
 *	对页进行就绪
 *	参数：
 *		ptTextCtrl		对象指针
 *		iFlag			需要对哪一页进行就绪
 *			TEXT_NEXT		下一页
 *			TEXT_PREV		上一页
 *			TEXT_CURRENT	当前页
 *	返回值：
 *		0 成功
 *		-1 失败
 *		
 *
 */
 #define RETURN_TEXT_SUCCEED 0
 #define RETURN_TEXT_EOF 	-1
 #define RETURN_TEXT_ERROR	-2
 
static	int TextReady(struct text_ctrl* ptTextCtrl,int iFlag)
{
	wchar_t Coding;
	struct page_desc *ptPageDesc;

	int error;
	char ctrl_char;
	switch (iFlag)
	{
		case TEXT_CURRENT:
			/* 一般情况下只会在open中调用到 */
			ptPageDesc = &ptTextCtrl->current;
		break;
		case TEXT_NEXT:
			ptPageDesc = &ptTextCtrl->next;
			/*  */
		break;
		case TEXT_PREV:
			ptPageDesc = &ptTextCtrl->prev;
		break;
	}
	ptPageDesc->len = 0;
	while(1)
	{
		Coding = GetCoding(ptTextCtrl->file_desc,ptPageDesc->read + ptPageDesc->len);
		if(Coding == 0 && ptPageDesc->is_ready == 0)
		{
			return RETURN_TEXT_EOF;
		}
		
		/* 进行编码判断，查看是控制字符还是可打印的普通字符 */
		ctrl_char = GetCtrlChar(Coding, ptTextCtrl->p_ctrl_char);
		if(ctrl_char != 'N')
		{
			/* 控制字符 */
			
			error =  TextAddCtrlWord(ptPageDesc->desc, ctrl_char);
		}
		else
		{
			/* 普通字符 */
			error = TextAddWord(ptPageDesc->desc,Coding);
		}

		if(error == 0)
		{
			ptPageDesc->is_ready = 1;
			ptPageDesc->len += sizeof(Coding);
			continue;
		}
		if(error == TEXT_EOF)
			break;
		if(error == TEXT_ERROR)
		{
			printf(MODULE_NAME": Error adding character\n");
			return RETURN_TEXT_ERROR;
		}		
	}
	return RETURN_TEXT_SUCCEED;
	
}





#define READ_END 0
#define WRITE_END 1

#define BUF_SIZE 4096

static void _signal_handler(int signo)
{
	/* 给子进程收尸 */
	if (signo == SIGCHLD) {
        while (waitpid(-1, NULL, WNOHANG) > 0) 
        {
        	/* NULL */	
        }
    }
}


/* 建立子进程进行编码转化 */
static FILE* Conversion(struct text_set* ptTextSet,int TargetCodeNum)
{
	int fd[2];
	int error;
	int var;
	int SrcCodeNum;
	char *filename;
	char new_filename[24];
	int i;
	/* 做好准备工作 */
	SrcCodeNum = ptTextSet->coding_format;

	filename = ptTextSet->filename;
	if(filename == NULL)
		return NULL;
	if(strlen(filename)>(24-1-4))
	{
		/* 大于必须截断 */
		for(i=0;i<(24-1-4);i++)
			new_filename[i] = filename[i];
		new_filename[i] = '\0';
		strcat(new_filename,".tem");
	}
	else
	{
		strcpy(new_filename, filename);
		strcat(new_filename,".tem");
	}


	
	/* 先建立通信，使用无名管道 */
	error = pipe(fd);
	if(error)
	{
		printf(MODULE_NAME": Unable to create nameless pipe\n");
		return NULL;
	}
	error = fork();
	if(error < 0)
	{
		printf(MODULE_NAME": Child process creation failed\n");
		return NULL;
	}
	if(error == 0)
	{
		int src_fd,target_fd;
		int err;
		char * buf,*read_buf;
		struct stat src_stat;
		wchar_t *target_coding;
		int read_count,Conversion_len, residue_len,write_len;
		/* 子进程 */
		/* 关闭读描述符 */
		close(fd[READ_END]);
		/* 打开源编码文件 */
		src_fd = open(filename,O_RDONLY);
		if(src_fd < 0)
		{
			err = -1;
			printf(MODULE_NAME": Failed to open file\n");
			goto src_open;
		}
		/* 进行mmap映射 */
		err = stat(filename, &src_stat);
		if(err<0)
		{
			err = -1;
			printf(MODULE_NAME": Failed to open file\n");
			goto src_stat;
		}
		buf =(char*) mmap(NULL,src_stat.st_size ,PROT_READ ,MAP_SHARED,src_fd,0);
		if(buf == (char*)-1)
		{
			err = -1;
			printf(MODULE_NAME": Failed to open file\n");
			goto src_mmap;
		}
		read_buf = buf;
		/* 创建目标编码文件 */
		target_fd = open(new_filename, O_WRONLY|O_CREAT|O_TRUNC);
		if(target_fd<0)
		{	
			err = -1;
			printf(MODULE_NAME": File creation failed\n");
			goto target_open;
		}
		residue_len = (int)src_stat.st_size;
		while(residue_len)
		{
			
			Conversion_len = BUF_SIZE < residue_len? BUF_SIZE : residue_len;
			target_coding = CodeConversion(SrcCodeNum, TargetCodeNum,read_buf,Conversion_len, &read_count);
			if(target_coding == NULL)
			{
				err = -1;
				printf(MODULE_NAME": Convert defeat\n");
				goto target_CodeConversion;
			}
			/* 写入进新文件中去 */
			write_len = wcslen(target_coding)*sizeof(wchar_t);

			err = write(target_fd,target_coding,write_len);
			CodeFreeCodeData(target_coding);
			if(err != write_len)
			{
				err = -1;
				printf(MODULE_NAME": write error\n");
				goto target_write;
			}
			/* 进行递增 */
			read_buf += read_count;
			residue_len -= read_count;
			
			if(fd[WRITE_END] != -1 && (read_buf-buf) >= (BUF_SIZE*8))
			{
				err = 0;
				write(fd[WRITE_END],&err,sizeof(int));
				close(fd[WRITE_END]);
				fd[WRITE_END] = -1;
			}
			
		}
		
		target_write:
		target_CodeConversion:
		close(target_fd);
		target_open:
		munmap(buf,src_stat.st_size);
		src_mmap:
		src_stat:
		close(src_fd);
		src_open:
		if(fd[WRITE_END] != -1)
		{
			write(fd[WRITE_END],&err,sizeof(int));
			close(fd[WRITE_END]);
		}
		exit(0);
	}
	else
	{
		/* 父进程 */
		/* 关闭写描述符 */
		signal(SIGCHLD,_signal_handler);
		close(fd[WRITE_END]);
		read(fd[READ_END],&var,sizeof(int));
		close(fd[READ_END]);
		/* 通过判断var来判断子进程的操作是否成功 */
		if(var < 0)
			return NULL;
		ptTextSet->coding_format = TargetCodeNum;
		return fopen(new_filename,"rb");
	}
}

#undef BUF_SIZE
#undef WRITE_END
#undef READ_END



/* 进行编码处理 		返回文件描述符 */
static FILE* CodingProcessor(struct text_set* ptTextSet)
{
	int SrcCodeNum,TargetCodeNum = -1;
	int i;
	SrcCodeNum = ptTextSet->coding_format;
	switch (SrcCodeNum)
	{
		case CODE_AUTO_DISCERN:
		{
			/* 这种情况下，需要自己去识别编码，识别失败返回-1 */
			int file_desc;
			char src_code[1024];
			int len;
			file_desc = open(ptTextSet->filename,O_RDONLY);
			if(file_desc<0)
			{
				printf(MODULE_NAME": Failed to open file\n");
				return NULL;
			}
			len = read(file_desc, src_code, 1024);
			if(len <= 0)
			{
				printf(MODULE_NAME": Empty file?\n");
				return NULL;
			}
			SrcCodeNum = CodeGuess(src_code, len);
			if(SrcCodeNum == -1)
			{
				printf(MODULE_NAME": Unrecognized code\n");
				return NULL;
			}
			ptTextSet->coding_format = SrcCodeNum;
			close(file_desc);
		}	
		default :
		/* 需要进行编码转化 当然要进行枚举*/
		for(i=0;i<SUPPORTEENCODING_SIZE;i++)
		{
			if(CodeConversionTest(SrcCodeNum,g_SupportedEncoding[i]))
			{
				TargetCodeNum = g_SupportedEncoding[i];
			}
		}
		break;
	}
	if(TargetCodeNum == -1)
	{
		printf(MODULE_NAME": This coding is not supported\n");
		return NULL;
	}
	/* 在下面进行编码转化 		  可能会使用几毫秒的时间 */
	return Conversion(ptTextSet,TargetCodeNum);
}


/**************************************************
 *	打开一个文本文件
 *	参数：
 *		ptTextSet	其他接口需要构造的结构 详情请看结构体成员
 *	返回值：
 *		返回一个控制对象
 *
 **************************************************/
struct text_ctrl* TextOpen(struct text_set* ptTextSet)
{
	struct text_ctrl* ptTextCtrl;
	struct coding_ctrl_char * pt_coding_ctrl_char;
	struct text_formatting  Info;
	FILE *FileDesc;
	int CodeNum;
	/* 进行必要的编码处理 得到文件描述符*/
	FileDesc = CodingProcessor(ptTextSet);
	if( FileDesc == NULL)
	{
		printf(MODULE_NAME": The encoding format is not supported\n");
		return NULL;
	}
	
	CodeNum = ptTextSet->coding_format;
	pt_coding_ctrl_char = look_for_ctrl_char(CodeNum);
	if(pt_coding_ctrl_char == NULL)
	{
		
		printf(MODULE_NAME": No control characters were found(%d)\n",CodeNum);
		return NULL;
	}
		
	/* 动态分配一个对象 */
	ptTextCtrl = (struct text_ctrl*) malloc(sizeof(struct text_ctrl));
	if(ptTextCtrl == NULL)
	{
		printf(MODULE_NAME": Space allocation failure\n");
		return NULL;
	}
	memset(ptTextCtrl,0,sizeof(struct text_ctrl));
	/* 1.设置Info，获取页描述符 */
	Info.CodingFormat =  GetCodeingFormatStr(CodeNum);
	Info.direction = ptTextSet->direction;
	Info.font		= ptTextSet->font;
	Info.space_code = pt_coding_ctrl_char->space_char;
	Info.line_spacing	= ptTextSet->line_spacing;
	Info.word_size	= ptTextSet->word_size;
	Info.word_spacing	= ptTextSet->word_spacing;
	Info.backg_colour	=	ptTextSet->backg_colour;
	Info.word_colour	=	ptTextSet->word_colour;
	/* 2.设置TextCtrl对象 */
	/* 2.1获取当前页 和下一页的描述符 和上一页描述符*/
	ptTextCtrl->current.desc = TextDispNew(&Info);
	if(ptTextCtrl->current.desc == -1)
	{
		return NULL;
	}
	ptTextCtrl->prev.desc = TextDispNew(&Info);
	if(ptTextCtrl->prev.desc == -1)
	{
		return NULL ;		
	}
	ptTextCtrl->next.desc = TextDispNew(&Info);
	if(ptTextCtrl->next.desc == -1)
	{
		return NULL;
	}
	/* 2.2获取文件关键信息 */
	ptTextCtrl->file_desc = FileDesc;
	
	/* 2.3获取文件栈对象 */
	ptTextCtrl->pt_stack = FileStackNew();
	
	/* 2.4生成控制字符列表 */
	
	ptTextCtrl->p_ctrl_char = pt_coding_ctrl_char->coding_map;
	/* 3.就绪当前页，和下一页 */

	/* 3.1设置当前页读写指针 且就绪当前页*/
	ptTextCtrl->current.read = 0;
	TextReady(ptTextCtrl,TEXT_CURRENT);
	/* 3.2设置下一页读写指针，且就绪下一页 */
	ptTextCtrl->next.read = ptTextCtrl->current.read + ptTextCtrl->current.len;
	TextReady(ptTextCtrl,TEXT_NEXT);
	
	
	return ptTextCtrl;
}


 void TextClose(struct text_ctrl* ptTextCtrl)
 {
	FileStackDel(ptTextCtrl->pt_stack);
	TextDispDel(ptTextCtrl->current.desc);
	TextDispDel(ptTextCtrl->next.desc);
	TextDispDel(ptTextCtrl->prev.desc);
	fclose(ptTextCtrl->file_desc);
	free(ptTextCtrl);
 }





/*
 *	在屏幕上显示上一页，下一页
 *	参数：
 *		ptTextCtrl		控制对象
 *		iFlag			显示标志
 *			SHOW_NEXT_PAGE		0x0	
 *			SHOW_PREV_PAGE		0x1 
 *			SHOW_CURRENT_PAGE 	0x2
 *	返回值：
 *			TEXT_SUCCEED		成功
 *			TEXT_NO_NEXT		没有下一页了
 *			TEXT_NO_PREV		没有上一页
 *			TEXT_NO_CURRENT		没有当前页	这种情况是文件长度为0的情况
 *			TEXT_ERROR			发生错误
 */
int TextShow(struct text_ctrl* ptTextCtrl,int iFlag)
{
	struct page_desc PageDescTmp;
	int error;
	switch (iFlag)
	{
		case SHOW_NEXT_PAGE:
			/* 先查看下一页是否就绪 */
			if(!ptTextCtrl->next.is_ready)
			{
				/*	没有就绪就说明没有下一页 */
				return TEXT_NO_NEXT;
			}
			/* 将下一页设置为当前页				    */
			/* 将当前页设置为上一页 			 */
			PageDescTmp = ptTextCtrl->prev;
			ptTextCtrl->prev = ptTextCtrl->current;
			ptTextCtrl->current = ptTextCtrl->next;
			ptTextCtrl->next = PageDescTmp;
			/* 显示设置后的当前页 */
			if(TextDisplay(ptTextCtrl->current.desc) == -1)
				return TEXT_ARISE_ERROR;

			/* 重新配置下次显示的下一页，提高操作效率 */
			
			/*
			 *	由上面的代码可以看出 此时的 ptTextCtrl->next 是即将准备丢弃的prev
			 *	在这个内容丢弃之前，我们要先将他们压在栈中,当我们往回翻页的时候，
			 *	再将他们弹出来
			 */
			 if(ptTextCtrl->next.is_ready)
				FilePush(ptTextCtrl->pt_stack,&ptTextCtrl->next.read , sizeof(ptTextCtrl->next.read));
			
			TextClean(ptTextCtrl->next.desc);
			/* 设置下一页未就绪，设置下一页读指针		 */
			ptTextCtrl->next.is_ready = 0;
			ptTextCtrl->next.read = ptTextCtrl->current.read + ptTextCtrl->current.len;
			if(TextReady(ptTextCtrl,TEXT_NEXT) == RETURN_TEXT_ERROR)
				return TEXT_ARISE_ERROR;
		break;

		case SHOW_CURRENT_PAGE:
			if(!ptTextCtrl->current.is_ready)
				return TEXT_NO_CURRENT;
			/* 直接将当前页在屏幕上描绘 */			
			if(TextDisplay(ptTextCtrl->current.desc) == -1)
				return TEXT_ARISE_ERROR;
		break;

		case SHOW_PREV_PAGE:
			if(!ptTextCtrl->prev.is_ready)
				return TEXT_NO_PREV;
			/* 将上一页设置为当前页          */
			/* 将当前页设置为下一页 */
			PageDescTmp = ptTextCtrl->next;
			ptTextCtrl->next = ptTextCtrl->current;
			ptTextCtrl->current = ptTextCtrl->prev;
			ptTextCtrl->prev = PageDescTmp;

			/* 显示配置后的当前页 */
			if(TextDisplay(ptTextCtrl->current.desc) == -1)
				return TEXT_ARISE_ERROR;
			/* 重新配置下次显示        新的上一页 */
			TextClean(ptTextCtrl->prev.desc);
			/* 设置上一页未就绪,设置上一页的读写指针，只能从栈中取出数据 */
			ptTextCtrl->prev.is_ready = 0;
			error = FilePop(ptTextCtrl->pt_stack, &ptTextCtrl->prev.read, sizeof(ptTextCtrl->prev.read));
			if(error == -1)
				return TEXT_ARISE_ERROR;
			if(error == 1)
				return 0;
			if(TextReady(ptTextCtrl,TEXT_PREV) == RETURN_TEXT_ERROR)
				return TEXT_ARISE_ERROR;
			
		break;

		default:
			return TEXT_ARISE_ERROR;
		break;
	}
	return TEXT_SUCCEED;
}








/* 初始化函数 */
int TextCtrlInit(void)
{
	int error;
	error=CodeInit();
	if(error)
		return -1;
	error = TextDisplayInit();
	if(error)
		return -1;

	return 0;
}

/* 结束退出函数 */
void TextCtrlExit(void)
{
	CodeExit();
	TextDisplayExit();
}






