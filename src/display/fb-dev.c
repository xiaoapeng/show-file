
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include <linux/fb.h>

#include <disp-manager-core.h>
#include <config.h>





static struct fb_var_screeninfo st_Var;	/* Current var */
static struct fb_fix_screeninfo st_Fix;	/* Current fix */
static int s_iScreenSize;
static int s_iFbFd;

static unsigned char *sp_FbMem;		//显存的地址
static unsigned int s_iLineWidth;	//x方向的宽度 以字节为单位
static unsigned int s_PixelWidth;	//每一个像素的宽度 以字节为单位 


static int FbShowPixel(int iPenX, int iPenY, unsigned int dwColor)
{
	unsigned char *pucFB8bpp = sp_FbMem+iPenX*s_PixelWidth+iPenY*s_iLineWidth;
	unsigned short *puwFB16bbp;	
	unsigned int *pudwFB24bbp;	
	puwFB16bbp = (unsigned short *) pucFB8bpp;
	pudwFB24bbp = (unsigned int *) pucFB8bpp;
	unsigned int rad,green,blue;
	switch (st_Var.bits_per_pixel)
	{
		case 8:
			*pucFB8bpp = (unsigned char)dwColor;
			break;
		case 16:
			rad = (dwColor >> 16) & 0xff;
			green = (dwColor >> 8) & 0xff;
			blue = (dwColor >> 0) & 0xff;
			*puwFB16bbp = ((rad>>3)<<(5+6))|((green>>2)<<5) | ( blue >> 3);
			break;
		case 24:
		case 32:
			*pudwFB24bbp = (unsigned int)dwColor;
			break;
		default:
			printf("can't surport  %dbpp \n",st_Var.bits_per_pixel);
			break;
	}
	return 0;
}

static int FbCleanScreen(unsigned int dwBackColor)
{
	unsigned char *pucFB8bpp = sp_FbMem;
	unsigned short *puwFB16bbp,uwFB16bbp;	
	unsigned int *pudwFB24bbp,i,udwFB24bbp;	
	puwFB16bbp = (unsigned short *) pucFB8bpp;
	pudwFB24bbp = (unsigned int *) pucFB8bpp;
	unsigned int uiRad,uiGreen,uiBlue;
	switch (st_Var.bits_per_pixel)
	{
		case 8:
			memset(pucFB8bpp, dwBackColor, s_iScreenSize);
			break;
		case 16:
			uiRad = (dwBackColor >> 16) & 0xff;
			uiGreen = (dwBackColor >> 8) & 0xff;
			uiBlue = (dwBackColor >> 0) & 0xff;
			uwFB16bbp = ((uiRad>>3)<<(5+6))|((uiGreen>>2)<<5) | ( uiBlue >> 3);
			i=0;
			while(i<s_iScreenSize)
			{
				*puwFB16bbp = uwFB16bbp;
				puwFB16bbp++;
				i += 2;
			}
			break; 
		case 24:
		case 32:
			udwFB24bbp = (unsigned int)dwBackColor;
			i=0;
			while(i<s_iScreenSize)
			{
				*pudwFB24bbp = udwFB24bbp;
				pudwFB24bbp++;
				i += 4;
			}
			break;
		default:
			printf("can't surport  %dbpp \n",st_Var.bits_per_pixel);
			return -1;
			break;
	}
	return 0;
}



static struct DispDevOpr  t_Opr ={
	.ShowPixel		=	FbShowPixel,
	.CleanScreen	=	FbCleanScreen,
};




static struct DispDevice t_FbDispDevice ={
	.name = "fb-dev",
	.pt_Opr = &t_Opr,

};

int FbDev_init(void)
{
	s_iFbFd=open("/dev/fb0",O_RDWR);
	if( s_iFbFd < 0 )
	{
		perror("can't open /dev/fb0\n/dev/fb0:");
		return -1;
	}
	if( ioctl(s_iFbFd,FBIOGET_VSCREENINFO,&st_Var) )
	{
		perror("can't get var\n/dev/fb0 ioctl:");
		return -1;
	}
	if( ioctl(s_iFbFd,FBIOGET_FSCREENINFO,&st_Fix) )
	{
		perror("can't get var\n/dev/fb0 ioctl:");
		return -1;
	}
	/* 设置设备结构体 */
	t_FbDispDevice.t_Info.dwBPP 	= st_Var.bits_per_pixel;
	t_FbDispDevice.t_Info.dwXres 	= st_Var.xres; 
	t_FbDispDevice.t_Info.dwYres 	= st_Var.yres; 
	t_FbDispDevice.t_Info.dwWidth	= st_Var.width;
	t_FbDispDevice.t_Info.dwHeight	= st_Var.height;
	
	s_iLineWidth = st_Var.xres * st_Var.bits_per_pixel / 8;
	s_PixelWidth = st_Var.bits_per_pixel / 8;
	s_iScreenSize = st_Var.xres * st_Var.yres * st_Var.bits_per_pixel / 8;
	sp_FbMem = (unsigned char *)mmap(NULL , s_iScreenSize, PROT_READ | PROT_WRITE, MAP_SHARED, s_iFbFd, 0);
	if(sp_FbMem == (unsigned char*)-1)
	{
		perror("can't mmap sp_FbMem\n/dev/fb0 mmap");
		close(s_iFbFd);
		return -1;
	}
	memset(sp_FbMem, 0, s_iScreenSize);

	if (RegisterDispDev(&t_FbDispDevice) == -1)
	{
		munmap(sp_FbMem, s_iScreenSize);
		close(s_iFbFd);
	}
	return 0;
}



void FbDev_exit(void)
{
	UnregisterDispDev(&t_FbDispDevice);
	munmap(sp_FbMem, s_iScreenSize);
	close(s_iFbFd);
}



