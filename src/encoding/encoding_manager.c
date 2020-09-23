/********************************************************
 *	管理编码的转化
 *	支持多种编码的转化
 *	
 *	
 *	
 ********************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>

#include <ulist.h>
#include <encoding_manager.h>

#define CODEING_FORMAT
#include <code.h>


#define THIS_NAME "endcoing-core"

#ifndef offsetof
# define offsetof(type,ident) ((size_t)&(((type*)0)->ident))
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({          \
	const __typeof__(((type *)0)->member)*__mptr =  (ptr);    \
		     (type *)((char *)__mptr - offsetof(type, member)); })
#endif


/* 存储转换完后编码的字符串 */
struct CodeDate{
	unsigned long ulID;			/* 创建它时的ID */
	wchar_t  pdwCodeTarget[0];	/* 目标编码    	*/
};


static  struct list_head  CodeHead;


/* 是否支持该源编码的转化 */
static inline int IsSupport(unsigned long ulSrcCodeFormat, struct CodeModule* ptCodeModule)
{
	int i;
	for(i=0;i<ptCodeModule->uNum;i++)
	{
		if(ptCodeModule->puSupportID[i] == ulSrcCodeFormat)
			return 1;
	}
	return 0;
}

/* 通过ID号来寻找模块 */
static inline struct CodeModule* LookingFor(unsigned long ulTargetCodeFormat)
{
	struct CodeModule* pos;
	list_for_each_entry(pos, &CodeHead, CodeNode)
	{
		if(pos->ulID == ulTargetCodeFormat)
			return pos;
	}
	return NULL;
}



/********************************************************
 *	编码转化函数
 *	首先通过目标编码ID去寻找CodeModule
 *	然后查看该CodeModule是否支持源ID
 *	参数：
 *		ulSrcCodeFormat					:源编码格式
 *		ulTargetCodeFormat				:目标编码格式
 *		strSrcCode						:源编码数据
 *		ulCodeLen						:编码长度
 *	返回值：
 *		成功：目标编码数据
 *		失败：NULL
 ********************************************************/
wchar_t * CodeConversion(unsigned long ulSrcCodeFormat, unsigned long ulTargetCodeFormat, 
				const char *strSrcCode, unsigned long ulCodeLen,int *pSuccessedNum)
{
	struct CodeModule* ptCodeModule;
	int SuccessedNum;
	wchar_t *strTarget;
	ptCodeModule = LookingFor(ulTargetCodeFormat);
	if(ptCodeModule && IsSupport(ulSrcCodeFormat,ptCodeModule))
	{
		strTarget = ptCodeModule->pt_opr->CodeGoalConversion(ulSrcCodeFormat,(unsigned const char*)strSrcCode,ulCodeLen,&SuccessedNum);
		if(pSuccessedNum)
			*pSuccessedNum = SuccessedNum;
		return strTarget;
	}
	
	//一般情况下，还有其他解决方案，比如u-f8 --> unicode     				--> GB2312
	//可以在这里去实现
	return NULL;
}



/********************************************************
 *	测试是否可以进行编码转化
 *	首先通过目标编码ID去寻找CodeModule
 *	然后查看该CodeModule是否支持源ID
 *	参数：
 *		ulSrcCodeFormat					:源编码格式
 *		ulTargetCodeFormat				:目标编码格式
 *	返回值：
 *		可行：1
 *		不行：0
 ********************************************************/

int CodeConversionTest(unsigned long ulSrcCodeFormat, unsigned long ulTargetCodeFormat)
{
	struct CodeModule* ptCodeModule;
	ptCodeModule = LookingFor(ulTargetCodeFormat);
	if(ptCodeModule && IsSupport(ulSrcCodeFormat,ptCodeModule))
	{
		return 1;
	}
	return 0;
}




/********************************************************
 *	自动识别源编码 
 *	参数：
 *		strSrcCode						:源编码数据 
 *		ulCodeLen						:编码长度
 *	返回值：
 *		成功：目标编码代码值
 *		失败：-1
 ********************************************************/
int CodeGuess(const char *strSrcCode,unsigned long ulCodeLen)
{
	int err;
	struct CodeModule* pos;
	list_for_each_entry(pos, &CodeHead, CodeNode)
	{
		if(pos->pt_opr->CodeIdentify==NULL)
			continue;
		err = pos->pt_opr->CodeIdentify((unsigned const char*) strSrcCode,ulCodeLen);
		if(err>=0 && err <64)
			return err;
	}
	return -1;
}



/********************************************************
 *	自动识别源编码，然后转化为目标编码
 *	参数：
 *		ulTargetCodeFormat				:目标编码格式
 *		strSrcCode						:源编码数据
 *		ulCodeLen						:编码长度
 *	返回值：
 *		成功：目标编码数据
 *		失败：NULL
 ********************************************************/
wchar_t * CodeAutomaticConversion(unsigned long ulTargetCodeFormat,  const char *strSrcCode, 
				unsigned long ulCodeLen,int *pSuccessedNum)
{
	int iID;
	wchar_t *pdwCodeTarget;
	iID = CodeGuess(strSrcCode, ulCodeLen);
	if(iID == -1)
	{
		printf(THIS_NAME": Try some other code. It won't recognize you automatically \n");
		return NULL;
	}
	pdwCodeTarget = CodeConversion(iID, ulTargetCodeFormat, strSrcCode, ulCodeLen,pSuccessedNum);
	if(pdwCodeTarget == NULL)
		printf( THIS_NAME": Convert defeat" );
	return pdwCodeTarget;
}

/********************************************************
 *	当一个编码不再使用，我们应该把空间给释放掉：
 *	我们将交给模块自己去释放，因为我不知道模块在
 *	转化时有没有使用其他空间。
 *	参数：
 *		pdwStr: 要释放的指针
 *	返回值：
 *		这个函数一般不会出错，希望模块的编写者
 *		能写出足够健壮的代码
 ********************************************************/

void CodeDWFree(wchar_t *pdwStr)
{
	struct CodeModule* ptCodeModule;
	struct CodeDate *ptCodeDate;
	unsigned long ulID;
	ptCodeDate = container_of(pdwStr, struct CodeDate, pdwCodeTarget[0]);
	ulID = ptCodeDate->ulID;
	ptCodeModule = LookingFor(ulID);
	ptCodeModule->pt_opr->TargetCodeFree(pdwStr);
}


/* 以上是给用户层调用的 */

//#####################################################################################

/* 以下全部是给模块代码用的 ，上层请勿调用*/




/********************************************************
 *	申请一个CodeDate对象
 *	给下层提供接口：
 *	请务必使用该函数来分配内存，如果使用Malloc来分配内存
 *	你必须明白你在做什么
 *	
 *	参数：
 *		ulLen 	:		额外申请的长度
 *		ulId	:		模块编码ID
 ********************************************************/
wchar_t* CodeAllocCodeData(unsigned long ulLen,unsigned long ulId)
{
	struct CodeDate* ptCodeDate;
	
	ptCodeDate=malloc(sizeof(struct CodeDate)+ulLen);
	if( ptCodeDate==NULL)
		return NULL;
	memset(ptCodeDate,0,sizeof(struct CodeDate)+ulLen);
	ptCodeDate->ulID = ulId;
	return ptCodeDate->pdwCodeTarget;
}


/********************************************************
 *	释放一个CodeDate对象
 *	给下层提供接口：
 *	参数：
 *		ulLen :		额外申请的长度
 ********************************************************/
void CodeFreeCodeData(wchar_t* ptCodeDateStr)
{
	struct CodeDate *ptCodeDate;
	ptCodeDate = container_of(ptCodeDateStr, struct CodeDate, pdwCodeTarget[0]);
	free(ptCodeDate);
}



/********************************************************
 *	给下层提供接口：
 *	注册一个编码模块	
 *	1.申请一个结构体
 *	2.设置它
 *	3.注册
 ********************************************************/
int RegisterCodeModule(struct CodeModule* pt_codeing)
{
	struct CodeModule* pos;
	if(pt_codeing->pt_opr == NULL)
	{
		printf(THIS_NAME":No operating function\n");
		return -1;
	}
	list_for_each_entry(pos, &CodeHead, CodeNode)
	{
		if( pos->ulID == pt_codeing->ulID)
		{
			printf(THIS_NAME": The same ID exists : %lu\n",pt_codeing->ulID);
			return -1;
		}
		if(pt_codeing->ulID>=64)	
		{
			printf(THIS_NAME":The ID is too big %lu\n",pt_codeing->ulID);
			return -1;
		}
	}
	//正常情况下
	list_add_tail(&pt_codeing->CodeNode, &CodeHead);
	return 0;
}



/********************************************************
 *	给下层提供接口：
 *	注销一个编码模块	
 *	1.申请一个结构体
 *	2.设置它
 *	3.注册
 ********************************************************/
void UnregisterCodeModule(struct CodeModule* pt_codeing)
{
	list_del(&pt_codeing->CodeNode);
}


extern int UnicodeModuleInit(void);
extern void UnicodeModuleExit(void);

extern int Gb2312ModuleInit(void);
extern void Gb2312ModuleExit(void);



/* 初始化函数 */
int CodeInit(void)
{
	int err;	
	INIT_LIST_HEAD(&CodeHead);
	//下面填写子模块的init函数
	err=UnicodeModuleInit();
	if(err)
		return err;
	err=Gb2312ModuleInit();
	if(err)
		return err;
	return 0;
}
/* 退出函数 */
void CodeExit(void)
{
	
	//下面填写子模块的exit函数
	UnicodeModuleExit();
	Gb2312ModuleExit();
}



