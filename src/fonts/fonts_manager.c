/*	根据文字大小、字体、
 *	来获取位图
 *	PT	:字体大小单位	72个点等于一英寸
 *	DPI	:每英寸能打印出多少像素
 *	
 *
 */
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <fonts_manager.h>
#include <ulist.h>

#define SCREEN_USE		1
#define SCREEN_NO_USE	0
#define MODULE_NAME		"fonts-core"
#define SCREEN_NUM 		4






struct OpenInfo ScreenDev[SCREEN_NUM];
static  struct list_head  FonsChannelHead;


static int PixelToPt(int Pixe, struct RequirInfo * ptRequirInfo)
{
	double	Height_mm;
	double	Height_inch; 
	double	Yres;
	double	H_DPI;		
	double  Pixe_Height_inch;
	double  Pt;
	Height_mm	= (double)ptRequirInfo->udwPhysHeight;
	Height_inch = Height_mm/25.4;			// 屏幕高多少英寸
	Yres		= (double)ptRequirInfo->udwYres;
	H_DPI		= Yres/Height_inch;			//计算DPI		像素/英寸
	Pixe_Height_inch = Pixe/H_DPI;			//计算我们的像素 有多少英寸
	Pt = Pixe_Height_inch*72;				//计算出 有多少个点
	//进行4舍5入
	return (int)(Pt>((int)Pt+0.5)?Pt+1:Pt);
}

static int ctrl_PixelToPt(int Pixe, struct OpenInfo * ptOpenInfo)
{
	double	Height_mm;
	double	Height_inch; 
	double	Yres;
	double	H_DPI;		
	double  Pixe_Height_inch;
	double  Pt;
	Height_mm	= (double)ptOpenInfo->udwPhysHeight;
	Height_inch = Height_mm/25.4;			// 屏幕高多少英寸
	Yres		= (double)ptOpenInfo->udwXres;
	H_DPI		= Yres/Height_inch;			//计算DPI		像素/英寸
	Pixe_Height_inch = Pixe/H_DPI;			//计算我们的像素 有多少英寸
	Pt = Pixe_Height_inch*72;				//计算出 有多少个点
	//进行4舍5入
	return (int)(Pt>((int)Pt+0.5)?Pt+1:Pt);
}


/* 查看是否支持这种编码 */
static inline int IsSupportCodingFormat(struct FontsChannel* ptFontsChannel,char* CodingFormat)
{
	char **aCodingFormat = ptFontsChannel->CodingFormatS;
	char *temCode;
	temCode = *aCodingFormat;
	while(temCode != NULL)
	{
		if(strcmp(temCode,CodingFormat)==0)
			return 1;
		temCode=*(++aCodingFormat);
	}
	return 0;
}
/* open时匹配时查看是否支持该字号 */
static int inline IsSupportFontSize(struct FontsChannel* ptFontsChannel, 
								struct RequirInfo * ptRequirInfo)
{
	int idwPt = ptRequirInfo->idwPT;
	int SupportPT = ptFontsChannel->SupportPT;
	int SupportPixe	= ptFontsChannel->SupportPixel;
	if(SupportPT == ALL_SIZE)
		return 1;
	if(SupportPT == idwPt)
		return 1;
		
	if(idwPt == PixelToPt(SupportPixe,ptRequirInfo))
		return 1;
	return 0;
}

/* open时匹配通道 */
static int inline MatchChannel(struct RequirInfo * ptRequirInfo,
								struct FontsChannel* ptFontsChannel)
{
	char* FontType = ptRequirInfo->FontType;
	char* CodingFormat = ptRequirInfo->CodingFormat;
		
	/* 支持这种字体 */
	if(strcmp(ptFontsChannel->SupportFontTypeS,FontType)&&
			strcmp(ptFontsChannel->SupportFontTypeS,ALL_FONT))
		return 0;
	/* 支持这种编码 */
	if(!IsSupportCodingFormat(ptFontsChannel, CodingFormat))
		return 0;
	/* 支持这种字号吗 */
	if(!IsSupportFontSize(ptFontsChannel, ptRequirInfo))
	{	
		printf(MODULE_NAME": This font type is not supported\n");
		return 0;
	}
	
	return 1;
}

/* Ctrl匹配时查看是否支持该字号 */
static int inline ctrl_IsSupportFontSize(struct FontsChannel* ptFontsChannel, 
								struct OpenInfo * ptOpenInfo)
{
	int idwPt = ptOpenInfo->idwPT;
	int SupportPT = ptFontsChannel->SupportPT;
	int SupportPixe	= ptFontsChannel->SupportPixel;
	if(SupportPT == ALL_SIZE)
		return 1;
	if(SupportPT == idwPt)
		return 1;
	if(idwPt == ctrl_PixelToPt(SupportPixe,ptOpenInfo))
		return 1;
	return 0;
}
/* ctrl时匹配通道 */
static int inline ctrl_MatchChannel(struct OpenInfo * ptOpenInfo,
								struct FontsChannel* ptFontsChannel)
{
	char* FontType = ptOpenInfo->FontType;
	char* CodingFormat = ptOpenInfo->CodingFormat;
	
	
	/* 支持这种字体 */
	if(strcmp(ptFontsChannel->SupportFontTypeS,FontType)&&
			strcmp(ptFontsChannel->SupportFontTypeS,ALL_FONT))
		return 0;
	/* 支持这种编码 */
	if(!IsSupportCodingFormat(ptFontsChannel, CodingFormat))
		return 0;
	/* 支持这种字号吗 */
	if(ctrl_IsSupportFontSize(ptFontsChannel, ptOpenInfo))
		return 1;
	
	return 0;
}


/* 寻找可用的描述符 */
static int LookingForDesc(void)
{
	int i;
	for(i=0;i<SCREEN_NUM;i++)
	{
		if(ScreenDev[i].iUseFlag == SCREEN_NO_USE)
			return i;
	}
	return -1;
}

static int dip(int PhysNum_mm, int PixelNum)
{
	double	Phys_mm;
	double	Phys_inch; 
	double	Pixel;
	double	DPI;
	Phys_mm 	= (double)PhysNum_mm;
	Phys_inch  	= Phys_mm/25.4;
	Pixel	   	= (double)PixelNum;
	DPI		   	=	Pixel/Phys_inch;
	return (int)(DPI>((int)DPI+0.5)?DPI+1:DPI);
}


/*****************************************************
 *	打开一个字体获取通道
 *	参数: 
 *		ptOpenInfo	 关于打开通道的信息
 *	返回值：
 *		成功返回通道描述符
 *		失败返回 -1
 *****************************************************/
int Fonts_open(struct RequirInfo * ptRequirInfo)
{
	struct FontsChannel* pos;
	int iDesc,SupportFlag=0;
	if(ptRequirInfo == NULL)
	{
		printf(MODULE_NAME": Use a null pointer\n");
		return -1;
	}
	if(ptRequirInfo->CodingFormat == NULL || ptRequirInfo->FontType == NULL)
	{
		printf(MODULE_NAME": The encoding or font string is NULL\n");
		return -1;
	}
	/* 查看有哪些通道可以支持 */
	list_for_each_entry(pos, &FonsChannelHead, ChannelNode)
	{
		if(MatchChannel(ptRequirInfo, pos))
		{
			SupportFlag = 1;
			break;
		}
	}
	if(!SupportFlag)
	{
		printf(MODULE_NAME": There are no Channel to support\n");
		return -1;
	}
	
	iDesc = LookingForDesc();
	if(iDesc == -1)
	{
		printf(MODULE_NAME": There are no descriptors available\n");
		return -1;
	}
	
	ScreenDev[iDesc].CodingFormat	=  ptRequirInfo->CodingFormat;
	ScreenDev[iDesc].FontType 		=  ptRequirInfo->FontType;
	ScreenDev[iDesc].udwPhysHeight 	=  ptRequirInfo->udwPhysHeight;	
	ScreenDev[iDesc].udwPhysWidth 	=  ptRequirInfo->udwPhysWidth;
	ScreenDev[iDesc].idwPT 			=  ptRequirInfo->idwPT;
	ScreenDev[iDesc].udwXres 		=  ptRequirInfo->udwXres;
	ScreenDev[iDesc].udwYres 		=  ptRequirInfo->udwYres;
	ScreenDev[iDesc].iAngle			=  ptRequirInfo->iAngle;
	ScreenDev[iDesc].ptFontsChannel =  pos;
	ScreenDev[iDesc].udwWDip		=  dip(ptRequirInfo->udwPhysWidth, ptRequirInfo->udwXres);
	ScreenDev[iDesc].udwHDip		=  dip(ptRequirInfo->udwPhysHeight, ptRequirInfo->udwYres);
	ScreenDev[iDesc].iUseFlag = SCREEN_USE;
	if(pos->Ops->FontsConfig(iDesc))
	{
		printf(MODULE_NAME": Configure channel failure\n");
		ScreenDev[iDesc].iUseFlag = SCREEN_NO_USE;
		return -1;
	}
	return iDesc;
}
/*****************************************************
 *	关闭一个字体通道
 *	参数: 
 *		ptOpenInfo	 关于打开通道的信息
 *	返回值：
 *		成功返回通道描述符
 *		失败返回 -1
 *****************************************************/

void Fonts_close(int Desc)
{
	int error;
	if(ScreenDev[Desc].iUseFlag == SCREEN_NO_USE)
		return ;
	error=ScreenDev[Desc].ptFontsChannel->Ops->FontsCleanConfig(Desc);
	if(error)
		printf(MODULE_NAME": Data cleanup may fail\n");
	ScreenDev[Desc].iUseFlag = SCREEN_NO_USE;
}

/*****************************************************
 *	关闭一个字体通道
 *	参数: 
 *		ptOpenInfo	 关于打开通道的信息
 *	返回值：
 *		成功返回通道描述符
 *		失败返回 -1
 *****************************************************/

int Fonts_ctrl(int Desc,int CMD,intptr_t Var)
{
	struct OpenInfo * ptScreenDev = &ScreenDev[Desc];
	struct FontsChannel* ptFontsChannel = ptScreenDev->ptFontsChannel;
	struct FontsChannel* pos;
	struct OpenInfo	ScreenDev_bak;
	int SupportFlag=0;

	
	/* 先备份原字体与通道 */
	ScreenDev_bak = *ptScreenDev;
	
	switch (CMD)
	{
		case CMD_CTRL_CODE:
			//更改编码
			if(!IsSupportCodingFormat(ptFontsChannel, ctrl_to_code(Var)))
			{
				printf(MODULE_NAME": This coding is not supported :%s \n",ctrl_to_code(Var));
				return -1;
			}
			ptScreenDev->CodingFormat = ctrl_to_code(Var);
			break;
		case CMD_CTRL_PT:
			//更改字号 有且只有支持所有字号的可以更改字号
			if(ptFontsChannel->SupportPT != ALL_SIZE)
			{
				printf(MODULE_NAME": Changing font size is not supported :%d\n",ctrl_to_pt(Var));
				return -1;
			}
			ptScreenDev->idwPT = ctrl_to_pt(Var);
			break;
		case CMD_CTRL_FONT:
			
			/* 更改字体时最复杂,需要重新匹配一个通道 */

			/* 更改成新字体*/
			ptScreenDev->CodingFormat = ctrl_to_font(Var);
			list_for_each_entry(pos, &FonsChannelHead, ChannelNode)
			{
				if(ctrl_MatchChannel(ptScreenDev, pos))
				{
					SupportFlag = 1;
					break;
				}
			}
			if(!SupportFlag)
			{
				*ptScreenDev = ScreenDev_bak;
				printf(MODULE_NAME": Changing fonts is not supported :%s\n",ctrl_to_font(Var));
				return -1;
			}
			/* 成功匹配到新通道 */
			ptScreenDev->ptFontsChannel = pos;
			break;
		default:
			printf(MODULE_NAME": Use the correct command\n");
			return -1;
	}
	/* 先清除原通道的配置 */
	if(ScreenDev_bak.ptFontsChannel->Ops->FontsCleanConfig(Desc))
	{
		printf(MODULE_NAME": Data cleanup may fail\n	It may cause abnormal operation\n");
		*ptScreenDev = ScreenDev_bak;
		return -1;
	}
	/* 重新配置新通道 */
	if(ptScreenDev->ptFontsChannel->Ops->FontsConfig(Desc))
	{
		printf(MODULE_NAME": Configure channel failure\n	It may cause abnormal operation\n");
		*ptScreenDev = ScreenDev_bak;
		return -1;
	}
	return 0;
}
/*****************************************************
 *	获取位图
 *	参数: 
 *		Desc	 描述符
 *		Code	 编码
 *	返回值：
 *		成功返回位图指针
 *		失败返回 NULL
 *****************************************************/
 
struct ImageMap* Fonts_getmap(int Desc,wchar_t Code)
{
	struct FontsChannel* ptFontsChannel = ScreenDev[Desc].ptFontsChannel;
	struct ImageMap* ptImageMap;

	ptImageMap=ptFontsChannel->Ops->FontsGetmap(Desc,Code);
	if(ptImageMap == NULL)
	{
		printf(MODULE_NAME": Bitmap acquisition failed\n");
		return NULL;
	}
	ptImageMap->ptFontsChannel = ptFontsChannel;
	ptImageMap->Desc = Desc;
	return ptImageMap;
}

void Fonts_putmap(struct ImageMap* ptImageMap)
{
	struct FontsChannel* ptFontsChannel = ptImageMap->ptFontsChannel;
	ptFontsChannel->Ops->FontsPutmap(ptImageMap);
}





/* 以下是提供给模块的函数 */

/*****************************************
 *	注册一个字体通道
 *	参数：
 *		ptFontsChannel:			通道结构体
 *	返回值:
 *		成功返回0
 *		失败返回-1
 *****************************************/

int  RegisteredFontsChannel(struct FontsChannel *ptFontsChannel)
{	
	if(!ptFontsChannel || !ptFontsChannel->Ops)
	{
		printf(MODULE_NAME": Invalid parameter\n");
		return -1;
	}
	if(ptFontsChannel->SupportPT == 0 && ptFontsChannel->SupportPixel == 0)
	{
		printf(MODULE_NAME": Don't support any sizes");
		return -1;
	}
	list_add_tail(&ptFontsChannel->ChannelNode , &FonsChannelHead);
	return 0;
}


/*****************************************
 *	注销一个字体通道
 *	参数：
 *		ptFontsChannel:			通道结构体
 *****************************************/
void  UnregisteredFontsChannel(struct FontsChannel *ptFontsChannel)
{
	list_del(&ptFontsChannel->ChannelNode);
}


/*****************************************
 *	动态分配一个字体位图结构体
 *	参数：
 *		iw:			字体的宽度
 *		ih:			字体的高度
 *	返回值:
 *		成功返回指针
 *		失败返回NULL
 *****************************************/
struct ImageMap* FontsAllocMap(unsigned long iw,unsigned long ih,int ulBaseLinex,
						int ulBaseLiney,int Increasingx)
{
	struct ImageMap* ptImageMap;
	mapU32_t *image;
	int iImageSize = 0;
	ptImageMap = (struct ImageMap*)malloc(sizeof(struct ImageMap));
	if(ptImageMap == NULL)
	{
		printf(MODULE_NAME": Space allocation failure\n");
		return NULL;
	}
	ptImageMap->Width = iw;
	ptImageMap->Height = ih;
	ptImageMap->BaseLiney = ulBaseLiney;
	ptImageMap->BaseLinex = ulBaseLinex;	
	ptImageMap->Increasingx = Increasingx;
	iImageSize = (iw*ih + 31) >> 5;
	image = (mapU32_t*)malloc(sizeof(mapU32_t)*iImageSize);
	if(image == NULL)
	{
		free(ptImageMap);
		printf(MODULE_NAME": Space allocation failure\n");
		return NULL;
	}
	memset(image,0 , sizeof(mapU32_t)*iImageSize);
	ptImageMap->image = image;
	return ptImageMap;
}



/*****************************************
 *	释放一个字体位图结构体
 *	参数：
 *		ptImageMap:			字体位图结构体
 *****************************************/
 void  FontsFreeMap(struct ImageMap* ptImageMap)
{
	if(ptImageMap == NULL)
		return ;
	free(ptImageMap->image);
	free(ptImageMap);
}





extern int FreetypeInit(void);
extern void FreetypeExit(void);

extern int HzkInit(void);
extern void HzkExit(void);

int FontsInit(void)
{
	int error;
	INIT_LIST_HEAD(&FonsChannelHead);
	/* 下面填写模块初始化代码  */
	error = FreetypeInit();
	if(error)
	{
		printf(MODULE_NAME":FreetypeInit Initialization failure\n");
		return -1;
	}
	error = HzkInit();
	if(error)
	{
		printf(MODULE_NAME":FreetypeInit Initialization failure\n");
		return -1;
	}
	return 0;
}

void FontsExit(void)
{
	FreetypeExit();
	HzkExit();
}





