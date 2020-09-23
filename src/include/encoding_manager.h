
#ifndef __CODEING_MANAGER__H_
#define __CODEING_MANAGER__H_
#include <ulist.h>




#define CODEDEF_TO_STR(NUM)	 CodeingFormat[NUM]


/********************************************************
 *###################子模块操作函数####################
 *
 *#CodeGoalConversion：
 *	编码转化函数	<子模块必须提供>
 *	参数 ：
 *		ulSrcCodeFormat	：需要转化的源编码格式
 *		strSrcCode		：需要转化的源编码数据
 *		ulCodeLen		：需要转化的编码长度
 *
 *#CodeIdentify
 *	编码识别函数 <可以不提供>
 *	简介：		主要对编码进行自动识别,识别源编码的格式，
 *		后期随着该核心层的完善会去掉该函数，去使用识
 *		别子模块识别
 *	参数:
 *		strSrcCode		: 需要识别的编码数据
 *		ulCodeLen		：需要识别的编码长度
 *	
 *#TargetCodeFree
 *	编码转化后,在不使用之后要进行释放操作 <必须提供>
 ********************************************************/

struct CodeOpr{
	wchar_t * (*CodeGoalConversion)(unsigned long ulSrcCodeFormat, 
				unsigned const char *strSrcCode,unsigned long ulCodeLen,int *pSuccessedNum);
	int (*CodeIdentify)(unsigned const char *strSrcCode, unsigned long ulCodeLen);	
	void  (*TargetCodeFree)(wchar_t *pdwStr);
};


struct CodeModule{
	const char *name;
	struct CodeOpr * pt_opr;			/* 操作函数 */
	unsigned long ulID;					/* 转化后的编码ID*/
	unsigned long *puSupportID; 		/* 支持哪些编码的转化*/	
	unsigned long uNum; 				/* 支持多少种编码的转化 */
	struct list_head  CodeNode;	
	
};




	  


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
extern wchar_t * CodeConversion(unsigned long ulSrcCodeFormat, unsigned long ulTargetCodeFormat, 
				const char *strSrcCode, unsigned long ulCodeLen,int *pSuccessedNum);


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
extern int CodeConversionTest(unsigned long ulSrcCodeFormat, unsigned long ulTargetCodeFormat);



/********************************************************
 *	自动识别源编码 
 *	参数：
 *		strSrcCode						:源编码数据 
 *		ulCodeLen						:编码长度
 *	返回值：
 *		成功：目标编码代码值
 *		失败：
 ********************************************************/
extern int CodeGuess(const char *strSrcCode,unsigned long ulCodeLen);



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
extern wchar_t * CodeAutomaticConversion(unsigned long ulTargetCodeFormat,  const char *strSrcCode, 
				unsigned long ulCodeLen,int *pSuccessedNum);

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
extern void CodeDWFree(wchar_t *pdwStr);




/********************************************************
 *	给下层提供接口：
 *	注册一个编码模块	
 *	1.申请一个结构体
 *	2.设置它
 *	3.注册
 ********************************************************/
int RegisterCodeModule(struct CodeModule* pt_codeing);



/********************************************************
 *	给下层提供接口：
 *	注销一个编码模块	
 *	1.申请一个结构体
 *	2.设置它
 *	3.注册
 ********************************************************/
void UnregisterCodeModule(struct CodeModule* pt_codeing);



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
wchar_t* CodeAllocCodeData(unsigned long ulLen,unsigned long ulId);


/********************************************************
 *	释放一个CodeDate对象
 *	给下层提供接口：
 *	参数：
 *		ulLen :		额外申请的长度
 ********************************************************/
void CodeFreeCodeData(wchar_t* ptCodeDateStr);





/* 初始化函数 */
extern int CodeInit(void);


/* 退出函数 */
extern void CodeExit(void);




#endif /* __CODEING_MANAGER__H_ */

