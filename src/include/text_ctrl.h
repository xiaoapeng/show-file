#ifndef __TEXT__CTRL__H
#define __TEXT__CTRL__H




#define CODE_AUTO_DISCERN	0x88

/* 
 *	TextShow函数参数iFalg 取值
 *
 *	SHOW_NEXT_PAGE			显示下一页
 *	SHOW_PREV_PAGE			显示上一页
 *	SHOW_CURRENT_PAGE		显示当前页
 *
 *
 */
#define SHOW_NEXT_PAGE		0x0
#define	SHOW_PREV_PAGE		0x1
#define	SHOW_CURRENT_PAGE 	0x2



/*
 *	TextShow函数返回值
 *	TEXT_SUCCEED		成功
 *	TEXT_NO_NEXT		没有下一页了
 *	TEXT_NO_PREV		没有上一页
 *	TEXT_NO_CURRENT		没有当前页	这种情况是文件长度为0的情况
 *	TEXT_ERROR			发生错误
 */
#define TEXT_SUCCEED 			0
#define TEXT_NO_NEXT 			0x1
#define TEXT_NO_PREV			0x2
#define TEXT_NO_CURRENT			0x3
#define TEXT_ARISE_ERROR		0x4

struct text_ctrl;
typedef struct text_ctrl TEXT_CTRL;

#ifndef HORIZONTAL

#define HORIZONTAL 	(0x1)
#define VERTICAL  	(0x2)

#endif

/*	用户设置使用text_ctrl的结构体
 *		word_spacing		字距
 *		line_spacing		行距
 *		word_size			字号
 *		direction			方向
 *		font				字体
 *		word_colour			字体颜色
 *		backg_colour		背景颜色
 *		coding_format		编码格式
 *		filename			要打开的文件名
 */
struct text_set{
	int 			word_spacing;
	int 			line_spacing;
	int 			word_size;
	int 			direction;
	int				coding_format;
	unsigned int 	word_colour;
	unsigned int	backg_colour;
	char*			font;
	char*			filename;
};
typedef struct text_set TEXT_SET;

/**************************************************
 *	打开一个文本文件
 *	参数：
 *		ptTextSet	其他接口需要构造的结构 详情请看结构体成员
 *	返回值：
 *		返回一个控制对象
 *
 **************************************************/
extern struct text_ctrl* TextOpen(struct text_set* ptTextSet);


/**************************************************
 *	释放TextOpen 使用的内存
 *
 **************************************************/

extern void TextClose(struct text_ctrl* ptTextCtrl);


/*
 *	在屏幕上显示上一页，下一页
 *	参数：
 *		ptTextCtrl		控制对象
 *		iFlag			显示标志
 *			SHOW_NEXT_PAGE		0x0	
 *			SHOW_PREV_PAGE		0x1 
 *			SHOW_CURRENT_PAGE 	0x2
 *	返回值：
 *			TEXT_SUCCEED		成功
 *			TEXT_NO_NEXT		没有下一页了
 *			TEXT_NO_PREV		没有上一页
 *			TEXT_NO_CURRENT		没有当前页	这种情况是文件长度为0的情况
 *			TEXT_ERROR			发生错误
 */
extern int TextShow(struct text_ctrl* ptTextCtrl,int iFlag);



/* 初始化函数 */
extern int TextCtrlInit(void);

/* 结束退出函数 */
extern void TextCtrlExit(void);


#endif /* __TEXT__CTRL__H */

