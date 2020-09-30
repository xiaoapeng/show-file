/*
 *	text_display.c
 *	本文件用于在屏幕上排列文字
 * 	本文件依赖于 display
 *
 *
 *
 *
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <disp-manager-core.h>
#include <fonts_manager.h>
#include <text_display.h>

#define MODULE_NAME "text-display"

static struct DispDeviceInfo* g_tScreenInfo;



/*
 *	描述结构体
 *	screen_map			屏幕位图文件
 *	text_formatting		字体的格式等信息
 *	font_desc			打开的字体描述符
 *	xdpi				x方向的dpi
 *	ydpi				y方向的dpi
 *	increasing_lines	y方向的基本递减大小
 *		对不同的方向x和y的值要调换
 *	VirtX				虚拟X方向长度
 *	VirtY				虚拟Y方向长度
 *	virt_xdpi			虚拟x方向的dpi
 *	virt_ydpi			虚拟y方向的dpi
 *	pen_x				笔的x坐标
 *	pen_y				笔的y坐标
 * 	word_spacing		字距			单位像素
 *	line_spacing		行距			单位像素
 *	
 */
struct text_desc
{
	struct 	screen_map 		*ptMap;
	struct 	text_formatting  ptTextFormatting;
	int 					font_desc;
	int						xdpi;
	int 					ydpi;
	int						increasing_lines;
	int 					virt_xdpi;
	int 					virt_ydpi;
	int 					virt_x;
	int 					virt_y;
	int						pen_x;
	int						pen_y;	
	int 					word_spacing;
	int 					line_spacing;
};

#define DESC_MAX_MUN (10)
static struct text_desc *g_aptTextDesc[DESC_MAX_MUN];

#define GetTextDesc(desc)	(g_aptTextDesc[(desc)])


/**************************************************************
 *	点阵描述结构体
 *	image		点阵位图
 *	Width		点阵宽
 *	Height		点阵高
 **************************************************************/

//typedef uint32_t mapU32_t;
struct screen_map {
	mapU32_t 	  	*image;
	unsigned long  	Width;		/* 这里的宽度总是说的像素数 */	
	unsigned long  	Height;		/*  */
	unsigned long	ImageSize;	/* image的大小 */
};


static inline int __GetScreenBit(mapU32_t *image, int pos)
{	
	mapU32_t Base = image[pos>>5];
	/* 去除高位 */	
	pos &= 0x1F;
	Base = Base << (31-pos);
	Base = Base >> 31;
	return Base;
}	


static inline void __SetScreenBit(mapU32_t *image, int pos,int var)
{	
	int index;
	mapU32_t dwValue;
	index = pos >> 5;
	dwValue = image[index];
	pos &= 0x1F;
	image[index] = var ? dwValue | (1 << pos)
					   : dwValue & (~ (1 << pos)) ;
}


#define GetScreenBit(ptImageMap,x,y)  	__GetScreenBit((ptImageMap)->image\
										,(y)*(ptImageMap)->Width+(x))
#define SetScreenBit(ptImageMap,x,y,var)	__SetScreenBit((ptImageMap)->image\
										,(y)*(ptImageMap)->Width+(x),var)
#define GetScreenWidth(ptImageMap)	((ptImageMap)->Width)
#define GetScreenHeight(ptImageMap)	((ptImageMap)->Height)


/*
 *	动态分配一个字体位图结构体
 *	返回值:
 *		成功返回指针
 *		失败返回NULL
*/
static struct screen_map* ScreenAllocMap(void)
{
	struct screen_map* ptImageMap;
	mapU32_t *image;
	unsigned long iImageSize = 0;
	ptImageMap = (struct screen_map*)malloc(sizeof(struct screen_map));
	if(ptImageMap == NULL)
	{
		printf(MODULE_NAME": Space allocation failure\n");
		return NULL;
	}
	ptImageMap->Width = g_tScreenInfo->dwXres;
	ptImageMap->Height = g_tScreenInfo->dwYres;
	iImageSize = (ptImageMap->Width*ptImageMap->Height + 31) >> 5;
	ptImageMap->ImageSize = sizeof(mapU32_t)*iImageSize; 
	image = (mapU32_t*)malloc(ptImageMap->ImageSize);
	if(image == NULL)
	{
		free(ptImageMap);
		printf(MODULE_NAME": Space allocation failure\n");
		return NULL;
	}
	ptImageMap->image = image;
	return ptImageMap;
}

static void  ScreenFreeMap(struct screen_map* ptImageMap)
{
	if(ptImageMap == NULL)
		return ;
	free(ptImageMap->image);
	free(ptImageMap);
}

static void ScreenClean(struct screen_map* ptImageMap)
{	
	if(ptImageMap == NULL)
		return ;
	memset(ptImageMap->image ,0, ptImageMap->ImageSize);
}


/* 寻找一个能用的描述符 */
static int LookForDesc(void)
{
	int i;
	for(i=0;i<DESC_MAX_MUN;i++)
	{
		if(g_aptTextDesc[i] == NULL)
			return i;
	}
	return -1;
}


/* 从单位点 	转化为单位像素 */
static int PtToPixeNum(int dpi, int pt)
{
	double dPt_inch = ((double)pt) / 72.0;
	double	dPixeNum = dPt_inch * dpi;
	int iPixeNum = (int)(dPixeNum > ((double)(((int)dPixeNum)+0.5))? dPixeNum +1 : dPixeNum);
	return iPixeNum;
}

/* 填充为文字的点到位图 */
static inline void print_image(struct text_desc *ptTextDesc,int x,int y)
{
	int direction ;
	struct 	screen_map 		*ptMap;
	struct 	text_formatting  *ptTextFormatting;
	if( x<0 || x > ptTextDesc->virt_x)
	{
		printf(MODULE_NAME":x = %d cross the border\n",x);
		return ;
	}


	
	if( y<0 || y > ptTextDesc->virt_y)
	{
		printf(MODULE_NAME":y = %d cross the border\n",y);
		return ;
	}
	ptTextFormatting = &ptTextDesc->ptTextFormatting;
	ptMap = ptTextDesc->ptMap;
	direction = ptTextDesc->ptTextFormatting.direction;
	switch (direction)
	{
		case HORIZONTAL:
			SetScreenBit(ptMap, x, y,1);
		break;

		case VERTICAL:
			SetScreenBit(ptMap,y,GetScreenHeight(ptMap)-x-1,1);
		break;
	}
	
}



/* 将编码打印到画布位图上去 */
static int FontgRidding(struct text_desc *ptTextDesc,wchar_t coding)
{
	struct ImageMap* ptImageMap;
	int pen_x,pen_y;
	int x,y;
	int x_max,y_max;
	/* use_width指在这个位图结束所占用的宽度 并不是位图的宽度			 */
	int use_width;
	
	pen_x = ptTextDesc->pen_x;
	pen_y = ptTextDesc->pen_y;
	/* 先获取位图 */
	ptImageMap = Fonts_getmap(ptTextDesc->font_desc, coding);
	if(ptImageMap == NULL)
	{
		printf(MODULE_NAME": Failed to get bitmap\n");
		return TEXT_ERROR;
	}
	if(pen_x == 0 && GetImageBaseLinex(ptImageMap) < 0)
	{
		pen_x -= GetImageBaseLinex(ptImageMap);
	}
	
	use_width = GetImageWidth(ptImageMap) + GetImageBaseLinex(ptImageMap);
	x_max = GetImageWidth(ptImageMap);
	y_max = GetImageHeight(ptImageMap);
	/* 判断是否需要换行,是否已经没有位置可以打印了 */
	if( (pen_x + use_width) > ptTextDesc->virt_x )
	{
		/* 需要换行 */
		/* 先计算下一行的pen_y  		减去基本换行的大小 再减去行距*/
		pen_x = 0;
		pen_y = pen_y - ptTextDesc->increasing_lines - ptTextDesc->line_spacing;
	}
	/* 判断是否需要换页 */
	if((pen_y + GetImageBaseLiney(ptImageMap)) < 0)
	{
		/* 已经没有空间可以打印				需要换页 */
		/* 且重置画笔 */
		pen_x = 0;
		pen_y = ptTextDesc->virt_y - ptTextDesc->increasing_lines;
		ptTextDesc->pen_x = pen_x;
		ptTextDesc->pen_y = pen_y;
		Fonts_putmap(ptImageMap);
		return TEXT_EOF;
	}
	
	
	/* 先执行打印函数 */
	//printf("pen_x = %d, pen_y = %d ,coding = 0x%08x , use_width = %d\n" , pen_x,pen_y,coding, use_width);
	//printf("x_max = %d , y_max = %d \n",x_max,y_max);
	for(x=0;x<x_max;x++)
	{
		for(y=0;y<y_max;y++)
		{
			if(GetImageBit(ptImageMap,x,y))
			{
				print_image(ptTextDesc, x+pen_x+GetImageBaseLinex(ptImageMap)
					, y+pen_y+GetImageBaseLiney(ptImageMap));
			}
		}
	}

	
	/* 配置pen_x到下一个字符 		，不需要考虑越界，因为函数的开头会检查 */
	pen_x += GetImageIncreasingx(ptImageMap) + ptTextDesc->word_spacing;
	ptTextDesc->pen_x = pen_x;
	ptTextDesc->pen_y = pen_y;
	Fonts_putmap(ptImageMap);
	return 0;
}





/****************************************************
 *	获取一个屏幕位图描述符
 *	参数：
 *		info:		获取描述符所必要的信息
 *	返回值：
 *		成功返回描述符
 *		失败返回-1
 ****************************************************/
int TextDispNew(struct text_formatting* info)
{
	
	struct text_desc *ptTextDesc;
	int iDesc;
	struct RequirInfo  tRequirInfo;
	int font_desc;

	/* 1.寻找一个可以使用的描述符 */
	iDesc = LookForDesc();
	
	if(iDesc == -1)
	{
		printf(MODULE_NAME": The appropriate descriptor could not be found\n");
		return -1;
	}
	/* 2.分配一个描述符结构体 text_desc */
	ptTextDesc = (struct text_desc *)malloc(sizeof(struct text_desc));
	if(ptTextDesc ==NULL)
	{
		printf(MODULE_NAME": TextDispNew->malloc(Memory allocation failure)\n");
		return -1;
	}
	/* 3. 用传进来的变量 和获取到的信息设置它 */
	/* 3.1 分配且设置位图结构体 */
	ptTextDesc->ptMap = ScreenAllocMap();
	if(ptTextDesc->ptMap == NULL)
	{
		free(ptTextDesc);
		printf(MODULE_NAME": TextDispNew->ScreenAllocMap(Memory allocation failure)\n");
		return -1;
	}
	/* 3.2设置字型的各项参数 */
	ptTextDesc->ptTextFormatting = *info;

	/* 3.3 获取字体描述符 ，并设置 */
	tRequirInfo.CodingFormat = info->CodingFormat;
	tRequirInfo.FontType = info->font;	/*宋体*/
	tRequirInfo.iAngle = 0 ;
	tRequirInfo.idwPT  = info->word_size;
	tRequirInfo.udwPhysHeight = g_tScreenInfo->dwHeight;
	tRequirInfo.udwPhysWidth  = g_tScreenInfo->dwWidth;
	tRequirInfo.udwXres	   = g_tScreenInfo->dwXres;
	tRequirInfo.udwYres	   = g_tScreenInfo->dwYres;
	font_desc=Fonts_open(&tRequirInfo);
	
	if(font_desc == -1)
	{
		ScreenFreeMap(ptTextDesc->ptMap);
		free(ptTextDesc);
		printf(MODULE_NAME": Fonts_open defeated\n");
		return -1;
	}
	ptTextDesc->font_desc = font_desc;
	/* 3.4 计算一些必要的属性 	注意设置的顺序 后面依赖于前面的设置*/
	ptTextDesc->xdpi = (int)(((double)g_tScreenInfo->dwXres) / (((double)g_tScreenInfo->dwWidth) / 25.4));
	ptTextDesc->ydpi = (int)(((double)g_tScreenInfo->dwYres) / (((double)g_tScreenInfo->dwHeight) / 25.4));
	ptTextDesc->increasing_lines = PtToPixeNum(info->direction == HORIZONTAL ? ptTextDesc->ydpi:ptTextDesc->xdpi, info->word_size);
	ptTextDesc->virt_x = info->direction == HORIZONTAL ? g_tScreenInfo->dwXres :g_tScreenInfo->dwYres ;
	ptTextDesc->virt_y = info->direction == HORIZONTAL ? g_tScreenInfo->dwYres :g_tScreenInfo->dwXres ;
	ptTextDesc->virt_xdpi	=	info->direction == HORIZONTAL ? ptTextDesc->xdpi : ptTextDesc->ydpi;
	ptTextDesc->virt_ydpi	=	info->direction == HORIZONTAL ? ptTextDesc->ydpi : ptTextDesc->xdpi;
	ptTextDesc->pen_x	=	0;
	ptTextDesc->pen_y	=	ptTextDesc->virt_y - ptTextDesc->increasing_lines ;
	ptTextDesc->word_spacing = PtToPixeNum(info->direction == HORIZONTAL ? ptTextDesc->ydpi:ptTextDesc->xdpi, info->word_spacing);
	ptTextDesc->line_spacing = PtToPixeNum(info->direction == HORIZONTAL ? ptTextDesc->ydpi:ptTextDesc->xdpi, info->line_spacing);
	/* 4.将它加到描述符数组 */
	g_aptTextDesc[iDesc] = ptTextDesc;

	return iDesc;
}


/****************************************************
 *	释放一个屏幕位图描述符
 *	参数：
 *		iDesc:		描述符
 *	返回值：
 *		无
 ****************************************************/

void TextDispDel(int iDesc)
{
	struct text_desc *ptTextDesc;
	ptTextDesc = GetTextDesc(iDesc);
	if(ptTextDesc == NULL)
		return;
	g_aptTextDesc[iDesc] = NULL;
	ScreenFreeMap(ptTextDesc->ptMap);
	Fonts_close(ptTextDesc->font_desc);
	free(ptTextDesc);
}



/*****************************************************
 *	给该描述符的位图添加一个字
 *	注意 :此函数静止传入任何控制字符,因为本模块不识别编码
 *	参数：
 *		iDesc			描述符
 *		coding			编码
 *	返回值：
 *		成功表示0
 *		
 *****************************************************/
int TextAddWord(int iDesc,wchar_t coding)
{
	struct text_desc *ptTextDesc;
	int error;
	ptTextDesc = GetTextDesc(iDesc);
	if(ptTextDesc == NULL)
	{
		printf(MODULE_NAME": Invalid descriptor\n");
		return TEXT_ERROR;
	}
	error = FontgRidding(ptTextDesc,coding);
	return error;
}

/*
 *	控制字符专用函数
 *	参数：
 *		iDesc		文本显示描述符
 *		ctrl		控制字符的Ascii码
 *	返回值:
 *		0 			表示成功
 *		TEXT_ERROR	表示错误
 *		TEXT_EOF	表示要换页了
 */
int TextAddCtrlWord(int iDesc,char ctrl)
{
	struct text_desc *ptTextDesc;
	int error;
	int pen_x,pen_y;
	
	
	ptTextDesc = GetTextDesc(iDesc);
	pen_x = ptTextDesc->pen_x;
	pen_y = ptTextDesc->pen_y;
	if(ptTextDesc == NULL)
	{
		printf(MODULE_NAME": Invalid descriptor\n");
		return TEXT_ERROR;
	}
	if((unsigned char)ctrl > 0x1f && ctrl != 0x7f)
	{
		printf(MODULE_NAME": Not a control character\n");
		return TEXT_ERROR;
	}
	switch (ctrl)
	{
		case '\n':
				
			/* 需要换行 */
			/* 先计算下一行的pen_y  		减去基本换行的大小 再减去行距*/
			pen_x = 0;
			pen_y = pen_y - ptTextDesc->increasing_lines - ptTextDesc->line_spacing;
			ptTextDesc->pen_x = pen_x;
			ptTextDesc->pen_y = pen_y;
			return 0;
		break;
		
		case '\t':
			/* 简单的打印2   个空格 */
			error = FontgRidding(ptTextDesc,ptTextDesc->ptTextFormatting.space_code);
			if(error == 0)
				error = FontgRidding(ptTextDesc,ptTextDesc->ptTextFormatting.space_code);
			return error;
		break;

		default:
			return 0;
		break;
	}
	return 0;
}


/*****************************************************
 *	在屏幕上将现阶段的Text打印出来
 *	参数：
 *		iDesc			描述符
 *	返回值：
 *		成功返回0
 *		失败返回-1
 *****************************************************/

int TextDisplay(int iDesc)
{
	struct text_desc *ptTextDesc;
	struct 	screen_map* ptScreenMap;
	int error;
	ptTextDesc = GetTextDesc(iDesc);
	ptScreenMap = ptTextDesc->ptMap;
	int x,y;
	/* 先清屏 */
	CleanScreen(ptTextDesc->ptTextFormatting.backg_colour);
	for(x=0;x<GetScreenWidth(ptScreenMap);x++)
	{
		for(y=0;y<GetScreenHeight(ptScreenMap);y++)
		{
			int var;
			var=GetScreenBit(ptScreenMap,x,y);
			if(var)
			{
				error=PixelDisplay(x, y, ptTextDesc->ptTextFormatting.word_colour  ,CARTESIAN_COORDINATE );
				if(error)
					return -1;
			}
		}
	}
	return 0;
}

/*****************************************************
 *	清除这一页，且将画笔放在开头
 *	参数：
 *		iDesc			描述符
 *****************************************************/

void TextClean(int iDesc)
{
	struct text_desc *ptTextDesc;
	struct 	screen_map* ptScreenMap;
	ptTextDesc = GetTextDesc(iDesc);
	ptScreenMap = ptTextDesc->ptMap;
	ScreenClean(ptScreenMap);
	ptTextDesc->pen_x = 0;
	ptTextDesc->pen_y = ptTextDesc->virt_y - ptTextDesc->increasing_lines;
}






/* 
 *	初始化函数
 *		对下层进行初始化
 *		并通过下层接口获取屏幕的lcd屏幕的信息
 */

 
int TextDisplayInit(void)
{
	int error;
	/* 进行下层的初始化 */
	error = DisplayInit();
	if(error)
	{
		printf(MODULE_NAME": DisplayInit defeated\n");
		return -1;
	}


	g_tScreenInfo = GetDispInfo("fb-dev");
	if(g_tScreenInfo == NULL)
	{		
		printf(MODULE_NAME": GetDispInfo defeated\n");
		return -1;
	}
	printf("dwXres,dwYres = %lu,%lu",g_tScreenInfo->dwXres,g_tScreenInfo->dwYres);
	printf("dwWidth,dwHeight = %lu,%lu",g_tScreenInfo->dwWidth,g_tScreenInfo->dwHeight);
	
	error = FontsInit();
	if(error)
	{
		printf(MODULE_NAME": FontsInit() Initialization failure\n");
		return -1;
	}
	return 0;
}


void TextDisplayExit(void)
{
	FontsExit();
	DisplayExit();
	
}




