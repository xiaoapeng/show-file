/**********************************************
 *	display/disp-manarger-core.c
 *  显示器的核心层代码:
 *	给上层提供像素显示接口
 *	给下层提供设备注册接口
 *  可以同时在多个设备上显示（如果有的话）
 **********************************************/

#include <stdio.h>
#include <ulist.h>


#include <config.h>
#include <disp-manager-core.h>
#include <string.h>


#define MODULE_NAME "display core"

/* 静态全局变量 */
static  struct list_head  DisplayDeviceHead;


/* 静态函数声明 */
static int 
__PixelDisplay(struct DispDevice *ptDispOpr,unsigned long x, unsigned long y, unsigned int dwColor, int flag);




static int 
__PixelDisplay(struct DispDevice *ptDispOpr,unsigned long x, unsigned long y, unsigned int dwColor, int flag)
{

	struct DispDeviceInfo *tp_Info = &ptDispOpr->t_Info;
	y = (flag == CARTESIAN_COORDINATE ? tp_Info->dwYres - y -1 : y);
	if (x >= tp_Info->dwXres || y >= tp_Info->dwYres)
	{
		//printf(MODULE_NAME": (x or y)Pointer to the cross-border\n x=%lu; y=%lu\n",x,y);
		return 0;
	}
	if(ptDispOpr->pt_Opr->ShowPixel(x, y, dwColor))
		return -1;
	return 0;
}


/*******************************************
 * 获取屏幕的信息
 *	参数： 
 *		传入注册屏幕设备时所使用的名字
 *	返回值：
 *		返回屏幕信息的指针
 *		失败返回 NULL
 *******************************************/
struct DispDeviceInfo* GetDispInfo(char* str_Name)
{
	struct list_head *pt_Pos;
	struct DispDevice *pt_PosDev;
	list_for_each(pt_Pos, &DisplayDeviceHead)
	{
		pt_PosDev=list_entry(pt_Pos, struct DispDevice, node);
		if ( !strcmp(pt_PosDev->name, str_Name) )
			return &pt_PosDev->t_Info;
	}
	printf(MODULE_NAME": GetDispInfo: %s couldn't be found\n",str_Name);
	return NULL;
}


/*******************************************
 *  注册一个屏幕驱动
 *	参数：
 *		屏幕的描述结构体
 *	返回值：
 *		正常情况下返回0
 *		失败返回 -1
 *******************************************/
int RegisterDispDev(struct DispDevice *ptDispOpr)
{
	struct list_head *pt_Pos;
	struct DispDevice *pt_PosDev;	
	struct DispDevOpr * pt_Opr;
	pt_Opr = ptDispOpr->pt_Opr;
	//错误检查
	if (pt_Opr == NULL || pt_Opr->CleanScreen == NULL || pt_Opr ->ShowPixel == NULL)
	{
		printf(MODULE_NAME": %s does not provide pt_Opr or member\n",ptDispOpr->name ? \
															ptDispOpr->name : " ");
		return -1;
	}
	list_for_each(pt_Pos, &DisplayDeviceHead)
	{
		pt_PosDev=list_entry(pt_Pos, struct DispDevice, node);
		if ( !strcmp(pt_PosDev->name, ptDispOpr->name) )
			printf(MODULE_NAME": %s seems to have been registered\n",ptDispOpr->name);
		return -1;
	}
	//加入链表
	list_add_tail(&ptDispOpr->node, &DisplayDeviceHead);
	return 0;
}


/*******************************************
 *  删除一个屏幕驱动
 *	参数：
 *		屏幕的描述结构体
 *******************************************/

void UnregisterDispDev(struct DispDevice *ptDispOpr)
{
	list_del(&ptDispOpr->node);
}

/*******************************************
 *	在所有已经注册的屏幕上进行打印
 *	参数：
 *		x: 			x方向
 *		y: 			y方向
 *		dwColor:	颜色
 *		flag		坐标系
 *	返回值：
 *		成功返回0
 *		越界打印返回-1
 *******************************************/

int PixelDisplay(unsigned long x, unsigned long y, unsigned int dwColor,int flag)
{
	struct list_head *pt_Pos;
	struct DispDevice *pt_PosDev;	
	int err = 0;
	list_for_each(pt_Pos, &DisplayDeviceHead)
	{
		pt_PosDev=list_entry(pt_Pos, struct DispDevice, node);	
		err = __PixelDisplay(pt_PosDev,x,y,dwColor,flag);
		if(err)
			return err;
	}
	return err;
}

/*******************************************
 *	将屏幕设置为某一个颜色
 *	参数：
 *		dwColor:	颜色
 *	返回值：
 *		成功返回0
 *		失败 -1
 *******************************************/

int CleanScreen(unsigned int dwColor)
{
	struct list_head *pt_Pos;
	struct DispDevice *pt_PosDev;	
	int err =0;
	list_for_each(pt_Pos, &DisplayDeviceHead)
	{
		pt_PosDev=list_entry(pt_Pos, struct DispDevice, node);	
		err = pt_PosDev->pt_Opr->CleanScreen(dwColor);
		if(err)
			return err;
	}
	return err;
}

extern int FbDev_init(void);
extern void FbDev_exit(void);

/*******************************************
 *	初始化函数
 *******************************************/
int DisplayInit(void)
{
	int err;
	//初始化核心层
	INIT_LIST_HEAD(&DisplayDeviceHead);
	
	//以下可填写显示设备的初始化函数
	err = FbDev_init();

	
	return err;
}
/*******************************************
 *	析构函数
 *******************************************/
void DisplayExit(void)
{
	
	
	//以下可填写显示设备的退出函数
	FbDev_exit();
	
}


