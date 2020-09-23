


#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <encoding_manager.h>
#include <stdint.h>
#include <code.h>

#ifndef ARRY_SIZE
# define ARRY_SIZE(A) (sizeof(A)/sizeof(A[0]))
#endif
#define MODULE_NAME "gb2312"


#define TARGET_TMP_SIZE 1024

static wchar_t * Gb2312BToGb2312DW(unsigned const char *strSrcCode,
					unsigned long ulCodeLen, int *pSuccessedNum)
{
	wchar_t TargetTmp[TARGET_TMP_SIZE];
	wchar_t *TargetPos;
	wchar_t *Gb2312Coding;
	unsigned const char * SrcPos;
	unsigned const char * SrcEnd;
	unsigned long ProcessLen;
	
	
	ProcessLen = ulCodeLen < (TARGET_TMP_SIZE-1) ? ulCodeLen : (TARGET_TMP_SIZE-1);
	SrcPos     = strSrcCode;
	SrcEnd     = strSrcCode + ProcessLen;
	TargetPos  = TargetTmp;
	while(SrcPos < SrcEnd)
	{

		if(*SrcPos >= 0xA1)
		{
			
			/* 可能是GB2312 */
			SrcPos++;
			if(SrcPos < SrcEnd)
			{
				*TargetPos++ = ((*SrcPos) << 8) |(*(SrcPos-1)); /* 确定为GB2312,如果第二个字节小于A1就让他去乱码吧*/				
				//printf("TargetPos = 0x%08x\n",*(TargetPos-1));
			}
			else
			{
				/* 表示GB2312被打断了，回退一步然后退出把*/
				SrcPos--;
				break;
			}
		}
		else
		{
			*TargetPos++ = *SrcPos;	/* ascii */			
			//printf("TargetPos = 0x%08x\n",*(TargetPos-1));
		}
	
		SrcPos++;
	}
	*TargetPos++ = '\0';
	Gb2312Coding = CodeAllocCodeData((TargetPos - TargetTmp) * sizeof(wchar_t),CODE_GB2312);
	if(Gb2312Coding == NULL)
	{
		printf(MODULE_NAME": Memory application failed\n");
		return NULL;
	}
	
	
	wcscpy(Gb2312Coding, TargetTmp);
	*pSuccessedNum = SrcPos - strSrcCode;
	
	//printf("pSuccessedNum = %d\n",*pSuccessedNum);
	return Gb2312Coding;
	
}



static wchar_t * Gb2312CodeGoalConversion(unsigned long ulSrcCodeFormat, 
				unsigned const char *strSrcCode,unsigned long ulCodeLen,int *pSuccessedNum)	
{
	switch (ulSrcCodeFormat)
	{
		case CODE_GB2312:
			return Gb2312BToGb2312DW(strSrcCode, ulCodeLen, pSuccessedNum);
		break;
		
		default:
			return NULL;
		break;
	}
	return NULL;
	
}


/********************************************************
 * 释放转化编码时所产生的数据
 ********************************************************/
static void  ThisTargetCodeFree(wchar_t *pdwStr)
{
	CodeFreeCodeData(pdwStr);
}




static struct CodeOpr Gb2312Opr ={
	.CodeGoalConversion = Gb2312CodeGoalConversion,
	.TargetCodeFree = ThisTargetCodeFree,
};


static unsigned long Gb2312SupportID[] = {
	CODE_GB2312,
};


static struct CodeModule  Gb2312Module = {
	.name = "gb2312",
	.ulID = CODE_GB2312,
	.puSupportID = Gb2312SupportID,
	.uNum = ARRY_SIZE(Gb2312SupportID),
	.pt_opr	=	&Gb2312Opr,
};



/********************************************************
 * 模块初始化函数
 ********************************************************/

int Gb2312ModuleInit(void)
{
	if(RegisterCodeModule(&Gb2312Module)<0)
	{
		printf(MODULE_NAME": Registration failed\n (name = %s , ID = %lu)\n",
						Gb2312Module.name,Gb2312Module.ulID);
		return -1;
	}

	return 0;

}

/********************************************************
 * 模块退出函数
 ********************************************************/

void Gb2312ModuleExit(void)
{
	UnregisterCodeModule(&Gb2312Module);
}





