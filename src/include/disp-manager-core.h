#ifndef __DISP__MANAGER__CORE__H
#define __DISP__MANAGER__CORE__H
#include <ulist.h>


/*
 *	CARTESIAN_COORDINATE 笛卡尔坐标系
 *	GENETAL_COORDINATE	一般屏幕坐标系
 *
 */
#define CARTESIAN_COORDINATE 	1
#define GENETAL_COORDINATE 		0

struct DispDevOpr{

	int (*ShowPixel)(int iPenX, int iPenY, unsigned int dwColor);
	int (*CleanScreen)(unsigned int dwBackColor);

};

struct DispDeviceInfo{
	/* 像素信息 */
	unsigned long  dwXres;
	unsigned long  dwYres;
	unsigned long  dwBPP;
	
	/* 物理尺寸 */
	unsigned long  dwWidth;
	unsigned long  dwHeight;
};

struct DispDevice{
	char *name;
	struct list_head  node;
	struct DispDeviceInfo t_Info;
	struct DispDevOpr * pt_Opr;
};


/*******************************************
 * 获取屏幕的信息
 *	参数： 
 *		传入注册屏幕设备时所使用的名字
 *	返回值：
 *		返回屏幕信息的指针
 *		失败返回 -1
 *******************************************/
struct DispDeviceInfo* GetDispInfo(char* str_Name);

/*******************************************
 *  注册一个屏幕驱动
 *	参数：
 *		屏幕的描述结构体
 *	返回值：
 *		正常情况下返回0
 *		失败返回 -1
 *******************************************/
int RegisterDispDev(struct DispDevice *ptDispOpr);


/*******************************************
 *  删除一个屏幕驱动
 *	参数：
 *		屏幕的描述结构体
 *******************************************/
void UnregisterDispDev(struct DispDevice *ptDispOpr);

/*******************************************
 *	在所有已经注册的屏幕上进行打印
 *	参数：
 *		x: 			x方向
 *		y: 			y方向
 *		dwColor:	颜色
 *	返回值：
 *		成功返回0
 *		越界打印返回-1
 *******************************************/
int PixelDisplay(unsigned long x, unsigned long y, unsigned int dwColor,int flag);

/*******************************************
 *	将屏幕设置为某一个颜色
 *	参数：
 *		dwColor:	颜色
 *	返回值：
 *		成功返回0
 *		失败 -1
 *******************************************/
int CleanScreen(unsigned int dwColor);

/*******************************************
 *	初始化函数
 *******************************************/
int DisplayInit(void);

/*******************************************
 *	析构函数
 *******************************************/
void DisplayExit(void);


#endif /* __DISP__MANAGER__CORE__H */

