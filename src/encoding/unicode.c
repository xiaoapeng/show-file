/*****************************************
 *将其他编码转化为unicode
 *支持 utf-8
 *
 *
 *
 *****************************************/

#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <encoding_manager.h>
#include <stdint.h>
#include <code.h>
#ifndef ARRY_SIZE
# define ARRY_SIZE(A) (sizeof(A)/sizeof(A[0]))
#endif

#define MODULE_NAME "unicode"

/*
 *	检查UTF8编码时的状态
 *	STATE_ASCII				是ASCII码
 *	STATE_UTF8				是正常的UTF8编码
 *	STATE_MESSY_CODE		乱码
 *	STATE_TAIL_MESSY_CODE	尾部出现乱码 只有结尾时 是UTF8中断了 才出现这种情况
 */

#define STATE_ASCII				(1)
#define STATE_UTF8				(2)
#define STATE_MESSY_CODE 		(3)
#define STATE_TAIL_MESSY_CODE	(4)
#define STATF_ERROR				(5)
/*
 *	itate			状态 STATE_XXXXX		
 *	src_count		处理的字符串长度			
 *	dest_count		可以转化的UTF8长度		
 */
struct Utf8CodingState{
	int itate;
	int src_count;
	int dest_count;
};

/* Test for GCC <= 4.3.2 */
#if __GNUC__ < 4 || \
    (__GNUC__ == 4 && (__GNUC_MINOR__ < 3 )) || \
                       ((__GNUC_MINOR__ == 3) && (__GNUC_PATCHLEVEL__ <= 2))
                        	
static size_t strnlen(const char *s, size_t maxlen)
{
	const char *start = s;
	while(start < s+maxlen && *start)		
		start++;
	return start - s;
}

#endif

static struct Utf8CodingState* _utf8_state(unsigned const char *utf8,unsigned const char *utf8_tail)
{	
	static struct Utf8CodingState  tReturnState;
	unsigned char var;
	int i;
	
	/* 检查 */
	if(utf8 >= utf8_tail || *utf8 == 0)
	{
		tReturnState.dest_count 	= 0;
		tReturnState.src_count 	= 0;
		tReturnState.itate			= STATF_ERROR;
		return &tReturnState;		
	}

	var = utf8[0];
	
	/* ascii码 */
	if( var <= 0x7F ){
		tReturnState.dest_count 	= 1;
		tReturnState.src_count 	= 1;
		tReturnState.itate			= STATE_ASCII;
		return &tReturnState;		
	}

	/* 先解决乱码情况 10xxxxxx     	   11111xxx开头就是乱码情况 */
	if( (( var >> 6 ) == 0x2 ) || ( var >> 3) == 0x1F)
	{
		tReturnState.dest_count 	= 1;
		tReturnState.src_count 	= 1;
		tReturnState.itate			= STATE_MESSY_CODE;		
		return &tReturnState;		
	}

	/* 可能是UTF8 */
	var <<= 1;
	
	for(i=1; ((utf8 + i) < utf8_tail) && (var & (1<<7)) ;i++,var <<=1 )
	{
		/* 还是乱码状态 */
		if( (utf8[i] >> 6)!= 0x2)
		{
			if(utf8[i]<=0x7F)
				i--;
			tReturnState.src_count 	= i + 1;
			tReturnState.dest_count 	= i + 1;	/* 以2个字符为一个乱码字符 */
			tReturnState.itate 		= STATE_MESSY_CODE;			
			return &tReturnState;		
		}
	}
	/* 确定为UTF8 */
	if((utf8 + i) <= utf8_tail && !(var & (1<<7)))
	{
		tReturnState.src_count 	= i;
		tReturnState.dest_count	= 1;
		tReturnState.itate 		= STATE_UTF8; 
		return &tReturnState;		
	}
	
	/* 结尾乱码 	这种状态可能是UTF8编码被中断的情况	 */
	tReturnState.src_count 	= i;
	tReturnState.dest_count	= 1;
	tReturnState.itate 		= STATE_TAIL_MESSY_CODE; 
	return &tReturnState;		

}




/* UTF8编码实现函数  		只要出现过一个UTF8编码就认为是UTF8*/
static int Utf8CodeIdentify(unsigned const char *strSrcCode, unsigned long ulCodeLen)
{
	unsigned const char * start;
	int state;
	struct Utf8CodingState *ptReturnStateTmp;
	int src_count;
	
	ulCodeLen = strnlen((char *)strSrcCode,ulCodeLen);
	start = strSrcCode;
	while(start<strSrcCode+ulCodeLen)
	{
		
		ptReturnStateTmp = _utf8_state(start,strSrcCode+ulCodeLen);
		state = ptReturnStateTmp->itate;
		src_count = ptReturnStateTmp->src_count;
		if(state == STATE_UTF8)
			return 1;
		start += src_count;
	}
	return 0;
}







#define MAX_SIZE 4096

static wchar_t * utf8_to_unicode_le(unsigned const char *strSrcCode, unsigned long ulCodeLen, int *SuccessedNum)
{
	wchar_t DestCode[MAX_SIZE];
	int processor_count;
	wchar_t *Write ;
	wchar_t *dest_unicode_le;
	unsigned const char * start;
	int state;
	struct Utf8CodingState *ptReturnStateTmp;
	int while_flag;
	


	while_flag = 1;
	Write = DestCode;
	/* 减1是因为需要存放\0的空间 */
	processor_count = ulCodeLen > (MAX_SIZE - 1) ? (MAX_SIZE - 1) : ulCodeLen;
	processor_count = strnlen((char *)strSrcCode,processor_count);
	start = strSrcCode;
	while(while_flag)
	{
		int src_count ;
		ptReturnStateTmp = _utf8_state(start,strSrcCode+processor_count);
		state = ptReturnStateTmp->itate;
		src_count = ptReturnStateTmp->src_count;
		switch (state)
		{
			case STATE_ASCII:
				*Write++ = *start++; 
			break;
			/************/
			case STATE_UTF8:
			{
				uint32_t tmp=0;
				unsigned char var;
				int ii;
				for(ii=1;ii<src_count;ii++)
				{
					var = start[ii];
					var &= ~(3<<6);
					tmp <<= 6;
					tmp |= var;
				}
				var = start[0]; 
				var <<= src_count;
				var >>= src_count;
				tmp |= ( ((uint32_t)var) << ((src_count - 1) * 6) );
				*Write++ = tmp;
				start += src_count;
				
				break;
			}
			/************/
			case STATE_MESSY_CODE:
			{
				int ii;
				for(ii=0;ii<src_count;ii++)
					*Write++ = '?';
				start += src_count;
				break;
			}
			/************/
			case STATE_TAIL_MESSY_CODE:
			{
				while_flag = 0;
			}
			break;
			/************/
			case STATF_ERROR:
			{
				while_flag = 0;
			}
			break;
			/************/
		}
	}
	*Write++ = '\0';
	dest_unicode_le = CodeAllocCodeData((Write - DestCode) * sizeof(wchar_t),CODE_UTF16_LE);
	if(dest_unicode_le == NULL)
	{
		printf(MODULE_NAME": Memory application failed\n");
		return NULL;
	}
	wcscpy(dest_unicode_le, DestCode);
	*SuccessedNum = start - strSrcCode;
	return dest_unicode_le;
	
}






/********************************************************
 * 编码转化函数  这里默认转化为 CODE_UTF16_LE 序编码
 ********************************************************/
static wchar_t * UnicodeLECodeGoalConversion(unsigned long ulSrcCodeFormat, 
				unsigned const char *strSrcCode,unsigned long ulCodeLen,int *pSuccessedNum)	
{
	wchar_t *pdwCodeTarget;	
	unsigned short * pwtmp;
	int i,count;
	switch (ulSrcCodeFormat)
	{
		case CODE_UTF8 :
			pdwCodeTarget = utf8_to_unicode_le(strSrcCode,ulCodeLen,pSuccessedNum);
			
			break;
		case CODE_UTF16_BE:
			if(ulCodeLen<2)
			{
				*pSuccessedNum = 0;
				return NULL;
			}
			/* 清除第0位 因为Unicode编码总是2字节对齐的*/
			ulCodeLen &= ~(1<<0);
			count = ulCodeLen;
			/* 对前置编码进行检查     		如果有前置编码的话  			需要滤过 */
			if(strSrcCode[0] == 0xfe && strSrcCode[1] == 0xff)
			{
				strSrcCode += 2;
				count -= 2;
			}
			pwtmp = (unsigned short *)strSrcCode;
			/* 需要分配\0的空间 */
			pdwCodeTarget = CodeAllocCodeData( ( count/2 + 1) * sizeof(wchar_t), CODE_UTF16_LE);
			
			for(i=0;pdwCodeTarget&&i<count/2;i++)
			{
				pdwCodeTarget[i]= (pwtmp[i] >> 8) | (pwtmp[i] << 8);
				pdwCodeTarget[i] &= 0xffff;
			}

			/* 核心层的内存分配函数 			会自动清除空间为0      		所以可以不添加\0*/
			*pSuccessedNum = ulCodeLen & (~(1<<0));
			
			break;
		case CODE_UTF16_LE:
			if(ulCodeLen<2)
			{
				*pSuccessedNum = 0;
				return NULL;
			}
			/* 清除第0位 因为Unicode编码总是2字节对齐的*/
			ulCodeLen &= ~(1<<0);
			count = ulCodeLen;
			/* 对前置编码进行检查			如果有前置编码的话 			需要滤过 */
			if(strSrcCode[0] == 0xff && strSrcCode[1] == 0xfe)
			{
				strSrcCode += 2;
				count -= 2;
			}
			pwtmp = (unsigned short *)strSrcCode;
			/* 需要分配\0的空间 */
			pdwCodeTarget = CodeAllocCodeData( ( count/2 + 1) * sizeof(wchar_t), CODE_UTF16_LE);
			
			for(i=0;pdwCodeTarget&&i<count/2;i++)
				pdwCodeTarget[i]= pwtmp[i];
			
			/* 核心层的内存分配函数			会自动清除空间为0			所以可以不添加\0*/
			*pSuccessedNum = ulCodeLen & (~(1<<0));
			
			break;
		default :
			printf(MODULE_NAME": This code is not supported (Does not support ID:%lu)\n",
						ulSrcCodeFormat);
			return NULL;
	}
	return pdwCodeTarget ;
}


/********************************************************
 * 编码转化函数 转化为CODE_UTF16_BE编码
 *
 * 当为UTF8 编码时先转换为CODE_UTF16_LE
 *	然后转化为 CODE_UTF16_BE
 ********************************************************/
static wchar_t * UnicodeBECodeGoalConversion(unsigned long ulSrcCodeFormat, 
			unsigned const char *strSrcCode,unsigned long ulCodeLen,int *pSuccessedNum)
{
	int i;
	wchar_t *pdwCodeTargetLe;
	unsigned char * pbtmp;
	unsigned short * pwtmp;
	int count ;

	switch (ulSrcCodeFormat)
	{
		case CODE_UTF8 :
			pdwCodeTargetLe = UnicodeLECodeGoalConversion(ulSrcCodeFormat,strSrcCode,ulCodeLen,pSuccessedNum);
			for(i=0;pdwCodeTargetLe!=NULL&&pdwCodeTargetLe[i]!=0;i++)
			{
				pbtmp = (unsigned char *)&pdwCodeTargetLe[i];
				pbtmp[0] ^= pbtmp[1];
				pbtmp[1] ^= pbtmp[0];
				pbtmp[0] ^= pbtmp[1];
			}
			break;
		case CODE_UTF16_LE :
			/* 清除第0位 因为Unicode编码总是2字节对齐的*/
			ulCodeLen &= ~(1<<0);
			count = ulCodeLen;
			/* 对前置编码进行检查     		如果有前置编码的话  			需要滤过 */
			if(strSrcCode[0] == 0xff && strSrcCode[1] == 0xfe)
			{
				strSrcCode += 2;
				count -= 2;
			}
			pwtmp = (unsigned short *)strSrcCode;
			/* 需要分配\0的空间 */
			pdwCodeTargetLe = CodeAllocCodeData( ( count/2 + 1) * sizeof(wchar_t), CODE_UTF16_BE);
			
			for(i=0;pdwCodeTargetLe&&i<count/2;i++)
				pdwCodeTargetLe[i]= (pwtmp[i] >> 8) | (pwtmp[i] << 8);

			/* 核心层的内存分配函数 			会自动清除空间为0      		所以可以不添加\0*/			
			*pSuccessedNum = ulCodeLen;
			break;
		default :
			printf(MODULE_NAME": This code is not supported (Does not support ID:%lu)\n",
						ulSrcCodeFormat);
			return NULL;
	}
	return pdwCodeTargetLe;
}


/********************************************************
 * 编码识别函数
 ********************************************************/
static int ThisCodeIdentify(unsigned const char *strSrcCode, 
									unsigned long ulCodeLen)
{	
	int err;
	/* utf8编码识别 */
	err=Utf8CodeIdentify(strSrcCode, ulCodeLen);
	if(err)
		return CODE_UTF8;

	if(ulCodeLen<2)
		return -1;
	/* utf16-le 编码识别 */
	if(strSrcCode[0]==0xFF && strSrcCode[1]==0xFE)
		return CODE_UTF16_LE;
	if(strSrcCode[0]==0xFE && strSrcCode[1]==0xFF)
		return CODE_UTF16_BE;
	
	return -1;
}



/********************************************************
 * 释放转化编码时所产生的数据
 ********************************************************/
static void  ThisTargetCodeFree(wchar_t *pdwStr)
{
	CodeFreeCodeData(pdwStr);
}




static unsigned long uaUnicodeLESupportID[] = {
	CODE_UTF8,CODE_UTF16_BE,CODE_UTF16_LE
};

static unsigned long uaUnicodeBESupportID[] = {
	CODE_UTF8,CODE_UTF16_LE
};

static struct CodeOpr tUnicodeLEOpr ={
	.CodeGoalConversion = UnicodeLECodeGoalConversion,
	.CodeIdentify = ThisCodeIdentify,
	.TargetCodeFree = ThisTargetCodeFree,
};

static struct CodeOpr tUnicodeBEOpr ={
	.CodeGoalConversion = UnicodeBECodeGoalConversion,
	.CodeIdentify = ThisCodeIdentify,
	.TargetCodeFree = ThisTargetCodeFree,
};


static struct CodeModule  UnicodeLEModule = {
	.name = "utf16-le",
	.ulID = CODE_UTF16_LE,
	.puSupportID = uaUnicodeLESupportID,
	.uNum = ARRY_SIZE(uaUnicodeLESupportID),
	.pt_opr	=	&tUnicodeLEOpr,
};

static struct CodeModule  UnicodeBEModube = {
	.name = "utf16-be",
	.ulID = CODE_UTF16_BE,
	.puSupportID = uaUnicodeBESupportID,
	.uNum = ARRY_SIZE(uaUnicodeBESupportID),
	.pt_opr	=	&tUnicodeBEOpr,
};


/********************************************************
 * 模块初始化函数
 ********************************************************/

int UnicodeModuleInit(void)
{
	if(RegisterCodeModule(&UnicodeLEModule)<0)
	{
		printf(MODULE_NAME": Registration failed\n (name = %s , ID = %lu)\n",
						UnicodeLEModule.name,UnicodeLEModule.ulID);
		return -1;
	}
	if(RegisterCodeModule(&UnicodeBEModube)<0)
	{
		printf(MODULE_NAME": Registration failed\n (name = %s , ID = %lu)\n",
						UnicodeBEModube.name,UnicodeBEModube.ulID);
		return -1;
	}
	return 0;

}

/********************************************************
 * 模块退出函数
 ********************************************************/

void UnicodeModuleExit(void)
{
	UnregisterCodeModule(&UnicodeLEModule);
	UnregisterCodeModule(&UnicodeBEModube);
}

