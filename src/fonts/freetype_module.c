

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <config.h>
#include <fonts_manager.h>
#include <ft2build.h>
#include <freetype/ftglyph.h>


#define MODULE_NAME "freetype"


struct freetype_module{
	FT_Library 		g_tLibrary;
	FT_Face 		g_tFace;
	FT_GlyphSlot 	g_tSlot;
	double	  		g_dAngle;
	FT_Matrix  		gt_matrix;				/* 变换矩阵 */ 
	FT_Vector 		pen; 					/* 非变换原点 */
};

/* 渲染到位图结构 */
static void FreetypeDraw_bitmap( FT_Bitmap*  bitmap,
             struct ImageMap* ptImageMap)
{
  FT_Int  x, y;
  FT_Int  x_max = GetImageWidth(ptImageMap);
  FT_Int  y_max = GetImageHeight(ptImageMap);
	for ( x=0; x < x_max; x++ )
	{
		for ( y=0; y < y_max; y++ )
		{
			SetImageBit(ptImageMap, x, y_max-y-1,bitmap->buffer[y*bitmap->width+x] );
		}
	}
}



/*****************************************
 *	清除配置函数	提供给核心层来释放配置函
 *	数中分配的内存
 *	
 *	参数：
 *		Desc: 通道描述符 通过描述符来获取信息
 *	返回值：
 *		成功返回0 失败返回 -1
 *****************************************/
static int FreetypeFontsCleanConfig(int Desc)
{
	int error;
	/* 如果配置使用标志没有置位 			 那么就返回 */
	struct freetype_module *FreetypeConfig;
	
	/* 取出关于本描述符的配置 */
	FreetypeConfig = (struct freetype_module *)GetInfoPrivateData(Desc);
	if(FreetypeConfig == NULL)
	{
		printf(MODULE_NAME": No configuration items\n");
		return -1;
	}
	error=FT_Done_Face(FreetypeConfig->g_tFace);
	if(error)
		return -1;
	FT_Done_FreeType(FreetypeConfig->g_tLibrary);
	if(error)
		return -1;
	free(FreetypeConfig);
	SetInfoPrivateData(Desc,NULL);
	return 0;
	
}


/*****************************************
 *	配置函数
 *	参数：
 *		Desc: 通道描述符 通过描述符来获取信息
 *	返回值：
 *		成功返回0 失败返回 -1
 *****************************************/
static int FreetypeFontsConfig(int Desc)
{
	int iError;
	char filename[20]; 
	struct freetype_module *FreetypeConfig;

	
	FreetypeConfig = (struct freetype_module *)malloc(sizeof(struct freetype_module));
	if(FreetypeConfig == NULL)
	{
		printf(MODULE_NAME": Failed to allocate space\n");
		return -1;
	}
	SetInfoPrivateData(Desc,FreetypeConfig);
	/* 显示矢量字体 */
	iError = FT_Init_FreeType(&FreetypeConfig->g_tLibrary );			   /* initialize library */
	/* error handling omitted */
	if (iError)
	{
		printf(MODULE_NAME": FreetypeFontsConfig failed\n");
		return -1;
	}
	strcpy(filename,GetInfoFontType(Desc));
	strcat(filename, ".ttf");
	iError = FT_New_Face(FreetypeConfig->g_tLibrary, filename, 0, &FreetypeConfig->g_tFace); /* create face object */
	if (iError)
	{
		printf(MODULE_NAME": FT_Init_FreeType failed\n");
		return -1;
	}
	FreetypeConfig->g_tSlot = FreetypeConfig->g_tFace->glyph;
	iError = FT_Set_Char_Size(FreetypeConfig->g_tFace,GetInfoPT(Desc)*64,GetInfoPT(Desc)*64, 
									GetInfoWDip(Desc),GetInfoHDip(Desc));
	if (iError)
	{
		printf(MODULE_NAME": FT_Set_Pixel_Sizes failed \n");
		return -1;
	}
	/* 注意	  这是笛卡尔坐标系   	    */
	FreetypeConfig->pen.x = 0*64;
	FreetypeConfig->pen.y = 0*64;	
	FreetypeConfig->g_dAngle = ( ((double)GetInfoAngle(Desc)) / 360 ) * 3.14159 * 2;	  /* use 25 degrees 	*/
	FreetypeConfig->gt_matrix.xx = (FT_Fixed)( cos( FreetypeConfig->g_dAngle ) * 0x10000L );
	FreetypeConfig->gt_matrix.xy = (FT_Fixed)(-sin( FreetypeConfig->g_dAngle ) * 0x10000L );
	FreetypeConfig->gt_matrix.yx = (FT_Fixed)( sin( FreetypeConfig->g_dAngle ) * 0x10000L );
	FreetypeConfig->gt_matrix.yy = (FT_Fixed)( cos( FreetypeConfig->g_dAngle ) * 0x10000L );
	FT_Set_Transform( FreetypeConfig->g_tFace, &FreetypeConfig->gt_matrix, &FreetypeConfig->pen );
	return 0;
}



static struct ImageMap* FreetypeFontsGetmap(int Desc,wchar_t Code)
{
	struct ImageMap*	ptImageMap;
	int 				error;
	struct freetype_module *FreetypeConfig;
	int ulBaseLinex;
	int ulBaseLiney;
	int Increasingx;
	/* 取出关于本描述符的配置 */
	FreetypeConfig = (struct freetype_module *)GetInfoPrivateData(Desc);
	if(FreetypeConfig == NULL)
	{
		printf(MODULE_NAME": No configuration items\n");
		return NULL;
	}
	
	/* 装载字形图像到字形槽（将会抹掉先前的字形图像） */
	error = FT_Load_Char( FreetypeConfig->g_tFace,Code, FT_LOAD_RENDER );
	if(error)
		return NULL;
	ulBaseLinex = FreetypeConfig->g_tSlot->bitmap_left;
	ulBaseLiney = FreetypeConfig->g_tSlot->bitmap_top - FreetypeConfig->g_tSlot->bitmap.rows ;
	Increasingx = FreetypeConfig->g_tSlot->advance.x/64;
	//printf("advance.x = %d ,advance.y = %d\n",FreetypeConfig->g_tSlot->advance.x,
	//											FreetypeConfig->g_tSlot->advance.y);
	
	ptImageMap = FontsAllocMap(FreetypeConfig->g_tSlot->bitmap.width, 
					FreetypeConfig->g_tSlot->bitmap.rows,ulBaseLinex,ulBaseLiney,Increasingx);
	if(ptImageMap == NULL)
		return NULL;
	FreetypeDraw_bitmap(&FreetypeConfig->g_tSlot->bitmap,ptImageMap);				
	return  ptImageMap;
}

static void FreetypeFontsPutmap(struct ImageMap* ptImageMap)
{
	FontsFreeMap(ptImageMap);
}


static struct	FontOps FreetypeOps = {
	.FontsGetmap = FreetypeFontsGetmap,
	.FontsPutmap = FreetypeFontsPutmap,
	.FontsConfig = FreetypeFontsConfig,
	.FontsCleanConfig = FreetypeFontsCleanConfig,
};

static char	*FreetypeCodingFormatS[]={
	"utf16-le",NULL,
};

static struct FontsChannel FontsChannel_songti = {
	.name 			  = "All font",
	.SupportFontTypeS = ALL_FONT,		/* 支持全部字体 */
	.SupportPT		  =	ALL_SIZE,		/* 支持全部大小 */
	.CodingFormatS	  = FreetypeCodingFormatS,
	.Ops = &FreetypeOps,
};





int FreetypeInit(void)
{
	if(RegisteredFontsChannel(&FontsChannel_songti))
	{
		printf(MODULE_NAME": Registration failed\n");
		return -1;
	}
	return 0;
}

void FreetypeExit(void)
{
	UnregisteredFontsChannel(&FontsChannel_songti);
}


