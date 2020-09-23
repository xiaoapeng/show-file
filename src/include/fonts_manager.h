
#ifndef __NTS_MANAGER__H_
#define __NTS_MANAGER__H_
#include <stdint.h>
#include <ulist.h>
#include <wchar.h>

/* 描述符的总数 */
#define SCREEN_NUM 		4


/**********************************************************	
 *根据屏幕信息来推算出一个点等于多少个像素
 *		iUseFlag			使用标准			1为使用
 *		dwXres:				x方向像素数量
 *		dwYres				y方向像素数量
 *		udwPhysWidth		x方向屏幕大小 以毫米为单位
 *		udwPhysHeight		y方向屏幕大小
 *		udwPT				字体大小 以点为单位 
 *		iAngle				角度（顺时针方向）
 *		CodingFormat		编码格式
 *		FontType			字体样式
 **********************************************************/
struct OpenInfo{
	int 					iUseFlag;
	/* 像素信息 */
	unsigned long  			udwXres;
	unsigned long  			udwYres;
	/* 物理尺寸 */
	unsigned long  			udwPhysWidth;
	unsigned long  			udwPhysHeight;
	unsigned long			udwWDip;
	unsigned long			udwHDip;
	int  					idwPT;	
	int 					iAngle;
	char 					*FontType;
	char 					*CodingFormat;
	void					*PrivateData;
	struct FontsChannel* 	ptFontsChannel;
};

 extern struct OpenInfo ScreenDev[];

#define GetInfoXres(Desc)				(ScreenDev[(Desc)].udwXres)
#define GetInfoYres(Desc)				(ScreenDev[(Desc)].udwYres)
#define GetInfoWidth(Desc)				(ScreenDev[(Desc)].udwPhysWidth)
#define GetInfoHeight(Desc)				(ScreenDev[(Desc)].udwPhysHeight)
#define GetInfoPT(Desc)					(ScreenDev[(Desc)].idwPT)
#define GetInfoFontType(Desc)			(ScreenDev[(Desc)].FontType)
#define GetInfoCodingFormat(Desc)		(ScreenDev[(Desc)].CodingFormat)
#define GetInfoWDip(Desc)				(ScreenDev[(Desc)].udwHDip)
#define GetInfoHDip(Desc)				(ScreenDev[(Desc)].udwWDip)
#define GetInfoAngle(Desc)				(ScreenDev[(Desc)].iAngle)
#define SetInfoPrivateData(Desc,Date)	(ScreenDev[(Desc)].PrivateData = (void*)(Date))
#define GetInfoPrivateData(Desc)		(ScreenDev[(Desc)].PrivateData)




/* 
 *	ALL_SIZE	支持所有的字号
 *	ALL_FONT	支持所有的字体
 *
 */
#define ALL_SIZE 	(-1)
#define ALL_FONT   "All font"
/**************************************************************	
 * OPEN时需要设置的结构体
 *		dwXres:				x方向像素数量
 *		dwYres				y方向像素数量
 *		udwPhysWidth		x方向屏幕大小 以毫米为单位
 *		udwPhysHeight		y方向屏幕大小
 *		udwPT				字体大小 以点为单位
 *		CodingFormat		编码格式
 *		FontType			字体样式
 **************************************************************/

struct RequirInfo{
	unsigned long  	udwXres;
	unsigned long  	udwYres;
	/* 物理尺寸 */
	unsigned long  	udwPhysWidth;
	unsigned long  	udwPhysHeight;
	
	int  			idwPT;
	int				iAngle;
	char* 			CodingFormat;
	char* 			FontType;
};

/* 
 *	Fonts_ctrl函数的控制宏
 *	CMD_CTRL_CODE	改变该描述符的编码格式
 *	CMD_CTRL_PT		改变该描述符的字体大小
 *	CMD_CTRL_FONT	改变该描述符的字体
 */

#define CMD_CTRL_CODE	(1)
#define CMD_CTRL_PT		(2)
#define CMD_CTRL_FONT	(3)

#define ctrl_pt(WordSize)	(WordSize)
#define ctrl_code(CodePtr)	((intptr_t)(CodePtr))
#define ctrl_font(FontPtr) 	((intptr_t)(FontPtr))

#define ctrl_to_pt(WordSize)	(WordSize)
#define ctrl_to_code(CodePtr)	((char*)(CodePtr))
#define ctrl_to_font(FontPtr) 	((char*)(FontPtr))


struct FontOps {
	struct ImageMap* (*FontsGetmap)(int Desc,wchar_t Code);
	void (*FontsPutmap)(struct ImageMap* ptImageMap);
	int (*FontsConfig)(int Desc);
	int (*FontsCleanConfig)(int Desc);
};



/***************************************************************
 *	模块注册通道时使用的结构体
 *  成员：
 *		SupportFontTypeS:	支持的字体类型	"songti" or "heiti"
 											or "weiruanyahei"
 *		SupportPixel	:	支持多少的像素		为了	（为了支持字库文件）
 *		SupportPT		:	支持多大的字体
 *		CodingFormatS	:	支持哪些编码的转化
 *  	Ops				:	下层提供的函数接口
 ***************************************************************/

struct FontsChannel{
	char 	*name;
	char 	*SupportFontTypeS;
	int 	SupportPixel;
	int 	SupportPT;
	char 	** CodingFormatS;
	struct 	FontOps *Ops;
	struct list_head  ChannelNode;
};


typedef uint32_t mapU32_t;


/**************************************************************
 *	点阵描述结构体
 *	image			点阵位图
 *	Width			点阵宽
 *	Height			点阵高
 *	BaseLinex		打印起点X坐标
 *	BaseLiney		打印起点y坐标
 *	Increasingx		下一次打印x方向建议的增值
 *	ptFontsChannel	该位图属于的通道
 *	基点坐标默认为 0,0
 *		点阵的大小 = ( Width * Height + 31) & 0xFFFF FFE0 / 32
 **************************************************************/
struct ImageMap{
	mapU32_t 	  	*image;
	unsigned long  	Width;
	unsigned long  	Height;
	int	BaseLinex;
	int	BaseLiney;
	int	Increasingx; 
	struct FontsChannel *ptFontsChannel;
	int Desc;
};

static inline int __GetImageBit(mapU32_t *image, int pos)
{	
	mapU32_t Base = image[pos>>5];
	/* 去除高位 */	
	pos &= 0x1F;
	Base = Base << (31-pos);
	Base = Base >> 31;
	return Base;
}	


static inline void __SetImageBit(mapU32_t *image, int pos,int var)
{	
	int index;
	mapU32_t dwValue;
	index = pos >> 5;
	dwValue = image[index];
	pos &= 0x1F;
	image[index] = var ? dwValue | (1 << pos)
					   : dwValue & (~ (1 << pos)) ;
}


#define GetImageBit(ptImageMap,x,y)  	__GetImageBit((ptImageMap)->image\
										,(y)*(ptImageMap)->Width+(x))
#define SetImageBit(ptImageMap,x,y,var)	__SetImageBit((ptImageMap)->image\
										,(y)*(ptImageMap)->Width+(x),var)
#define GetImageWidth(ptImageMap)	((ptImageMap)->Width)
#define GetImageHeight(ptImageMap)	((ptImageMap)->Height)
#define GetImageBaseLinex(ptImageMap) ((ptImageMap)->BaseLinex)
#define GetImageBaseLiney(ptImageMap) ((ptImageMap)->BaseLiney)
#define GetImageIncreasingx(ptImageMap) ((ptImageMap)->Increasingx)

/*****************************************************
 *	打开一个字体获取通道
 *	参数: 
 *		ptOpenInfo	 关于打开通道的信息
 *	返回值：
 *		成功返回通道描述符
 *		失败返回 -1
 *****************************************************/
extern int Fonts_open(struct RequirInfo * ptRequirInfo);

/*****************************************************
 *	关闭一个字体通道
 *	参数: 
 *		ptOpenInfo	 关于打开通道的信息
 *	返回值：
 *		成功返回通道描述符
 *		失败返回 -1
 *****************************************************/
extern void Fonts_close(int Desc);

/*****************************************************
 *	关闭一个字体通道
 *	参数: 
 *		ptOpenInfo	 关于打开通道的信息
 *	返回值：
 *		成功返回通道描述符
 *		失败返回 -1
 *****************************************************/
extern int Fonts_ctrl(int Desc,int CMD,intptr_t Var);

/*****************************************************
 *	获取位图
 *	参数: 
 *		Desc	 描述符
 *		Code	 编码
 *	返回值：
 *		成功返回位图指针
 *		失败返回 NULL
 *****************************************************/
extern struct ImageMap* Fonts_getmap(int Desc,wchar_t Code);

/*****************************************************
 *	释放位图
 *	参数: 
 *		ptImageMap	 位图指针
 *****************************************************/
extern void Fonts_putmap(struct ImageMap* ptImageMap);



/* 以下是提供给模块的函数 */



/*****************************************
 *	注册一个字体通道
 *	参数：
 *		ptFontsChannel:			通道结构体
 *	返回值:
 *		成功返回0
 *		失败返回-1
 *****************************************/

extern int  RegisteredFontsChannel(struct FontsChannel *ptFontsChannel);

/*****************************************
 *	注销一个字体通道
 *	参数：
 *		ptFontsChannel:			通道结构体
 *****************************************/
extern void  UnregisteredFontsChannel(struct FontsChannel *ptFontsChannel);


/*****************************************
 *	动态分配一个字体位图结构体
 *	参数：
 *		iw:			字体的宽度
 *		ih:			字体的高度
 *	返回值:
 *		成功返回指针
 *		失败返回NULL
 *****************************************/
extern struct ImageMap* FontsAllocMap(unsigned long iw,unsigned long ih,int ulBaseLinex,int ulBaseLiney,int Increasingx);

/*****************************************
 *	释放一个字体位图结构体
 *	参数：
 *		ptImageMap:			字体位图结构体
 *****************************************/
extern void  FontsFreeMap(struct ImageMap* ptImageMap);



/* 初始化函数 */
int FontsInit(void);


/* 退出函数 */
void FontsExit(void);









#endif	/* __NTS_MANAGER__H_ */





