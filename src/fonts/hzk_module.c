
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>



#include <config.h>
#include <fonts_manager.h>


#define MODULE_NAME "hzk_module"


/*
 *私有数据 
 *	file_desc	打开的汉字库的文件描述符
 *	hzk_mem		汉字库在内存中的映射
 */
struct Hzk_module{
	int file_desc;
	unsigned char* hzk_mem;
	unsigned long mmap_len;
};




extern const unsigned char fontdata_8x16[];




static void set_ascii_map(struct ImageMap* ptImageMap, wchar_t Code)
{
	unsigned char *dots = (unsigned char *)&fontdata_8x16[Code*16];
	int x,y;
	unsigned char byte;
	for(y=0;y < 16;y++)
	{
		byte = dots[y];
		/* 每行 */
		for(x=7;x>=0;x--)
		{
			/* 每列 */
			if(byte & (1 << x))
			{
				SetImageBit(ptImageMap,7-x,15 - y, 1);
			}
		}
	}


}

static void set_gb2312_map(struct ImageMap* ptImageMap, wchar_t Code , unsigned char* hzk_mem)
{
	unsigned int area  = (Code - 0xA1) & 0xff;
	unsigned int where = ((Code >> 8) - 0xA1) & 0xff;
	unsigned short *dots = (unsigned short *) (hzk_mem + (area * 94 + where)*32);
	int y, x;
	unsigned short word;
	//printf("area = 0x%08x\nwhere = 0x%08x\n",area,where);
	for(y=0;y<16;y++)
	{
		//每行
		word = dots[y];
		word = (word << 8) | (word >> 8) ;
		for(x=0;x<16;x++)
		{
			//每列
			if(word & 1<<(15-x))
			{
				SetImageBit(ptImageMap,x,15-y,1);
			}
		}
	}
	

}

static struct ImageMap* HzkFontsGetmap(int Desc,wchar_t Code)
{
	struct ImageMap*	ptImageMap;
	struct Hzk_module * ptHzkModule;
	ptHzkModule = (struct Hzk_module *)GetInfoPrivateData(Desc);
	if(Code <= 0x7F)
	{
		/* ascii码 */
		ptImageMap = FontsAllocMap(8,16,0,0,8);
		if(ptImageMap == NULL)
			return NULL;
		set_ascii_map(ptImageMap,Code);
	}
	else
	{
		/* gb2312 */
		ptImageMap = FontsAllocMap(16,16,0,0,16);
		if(ptImageMap == NULL)
			return NULL;
		//printf("Code = 0x%x\n",Code);
		//printf("ptHzkModule->hzk_mem = 0x%p\n",ptHzkModule->hzk_mem);
		set_gb2312_map(ptImageMap,Code,ptHzkModule->hzk_mem);
	}
	
	return  ptImageMap;
}

static void HzkFontsPutmap(struct ImageMap* ptImageMap)
{
	FontsFreeMap(ptImageMap);
}





static int HzkFontsConfig(int Desc)
{
	int FdHzk16;
	struct stat tHzkStat;
	unsigned char * ucpHzkMem;
	struct Hzk_module * ptHzkModule;
	int error = 0;

	ptHzkModule = (struct Hzk_module *)malloc(sizeof(struct Hzk_module));
	if(ptHzkModule == NULL )
	{
		printf(MODULE_NAME": Failed to allocate ptHzkModule space\n");
		return -1;

	}
	
	//printf("%d :555555555555\n",getpid());
	FdHzk16 = open("HZK16", O_RDONLY);
	if (FdHzk16 < 0)
	{
		printf(MODULE_NAME": can't open HZK16\n");
		error = -1;
		goto open_error;
	}
	if(fstat(FdHzk16, &tHzkStat))
	{
		printf(MODULE_NAME": can't get fstat\n");
		error = -1;
		goto fstat_error;
	}
	ucpHzkMem = (unsigned char *)mmap(NULL , tHzkStat.st_size, PROT_READ, MAP_SHARED, FdHzk16, 0);
	if((unsigned char *)-1 == ucpHzkMem)
	{
		printf(MODULE_NAME":can't mmap for hzk16\n");
		error = -1;
		goto mmap_error;
	}
	ptHzkModule->file_desc = FdHzk16;
	ptHzkModule->hzk_mem = ucpHzkMem;
	ptHzkModule->mmap_len = tHzkStat.st_size;
	SetInfoPrivateData(Desc, ptHzkModule);

	return error;
	mmap_error:
	fstat_error:
	close(FdHzk16);
	open_error:
	free(ptHzkModule);
	
	return 0;
}

static int HzkCleanConfig(int Desc)
{
	struct Hzk_module * ptHzkModule;
	ptHzkModule = (struct Hzk_module *)GetInfoPrivateData(Desc);
	munmap(ptHzkModule->hzk_mem, ptHzkModule->mmap_len);
	close(ptHzkModule->file_desc);	
	SetInfoPrivateData(Desc, NULL);
	free(ptHzkModule);
	return 0;
}

static struct	FontOps HzkOps = {
	.FontsGetmap = HzkFontsGetmap,
	.FontsPutmap = HzkFontsPutmap,
	.FontsConfig = HzkFontsConfig,
	.FontsCleanConfig = HzkCleanConfig,
};

static char	*HzkCodingFormatS[]={
	"gb2312",NULL,
};



static struct FontsChannel HzkFontsChannel = {
	.name 			  = "Hzk",
	.SupportFontTypeS = ALL_FONT,		/* 支持全部字体 */
	.SupportPT		  = ALL_SIZE,
	.CodingFormatS	  = HzkCodingFormatS,
	.Ops = &HzkOps,
};





int HzkInit(void)
{
	if(RegisteredFontsChannel(&HzkFontsChannel))
	{
		printf(MODULE_NAME": Registration failed\n");
		return -1;
	}
	return 0;
}

void HzkExit(void)
{
	UnregisteredFontsChannel(&HzkFontsChannel);
}





















