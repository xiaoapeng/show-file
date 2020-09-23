
#ifndef __TEXT__DISPLAY_H__
#define __TEXT__DISPLAY_H__ 1


/*
 *	HORIZONTAL 	横屏
 *	VERTICAL	竖屏
 *
 */
#define HORIZONTAL 	(0x1)
#define VERTICAL  	(0x2)



/*
 *	构造时需要的设置的值
 *	word_spacing		字距			单位为pt
 *	line_spacing		行距			单位为pt
 *	word_size			字号			单位为pt
 *	direction			显示方向
 *	space_code			空格的编码值
 *	font				字体
 *	CodingFormat		编码
 */
struct text_formatting{
	int 		word_spacing;
	int 		line_spacing;
	int 		word_size;
	int 		direction;
	wchar_t		space_code;
	char*		font;
	char* 		CodingFormat;
	unsigned int 	word_colour;
	unsigned int	backg_colour;
};

/* 
 *TextAddWord 的返回值 
 *	TEXT_EOF		表示到达屏幕结尾
 *	TEXT_ERROR		表示出错
 */
#define TEXT_EOF		0x1
#define TEXT_ERROR		0x2



/****************************************************
 *	获取一个屏幕位图描述符
 *	参数：
 *		info:		获取描述符所必要的信息
 *	返回值：
 *		成功返回描述符
 *		失败返回-1
 ****************************************************/
int TextDispNew(struct text_formatting* info);


/****************************************************
 *	释放一个屏幕位图描述符
 *	参数：
 *		iDesc:		描述符
 *	返回值：
 *		无
 ****************************************************/

void TextDispDel(int iDesc);

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
int TextAddWord(int iDesc,wchar_t coding);

/******************************************************
 *	控制字符专用函数
 *	参数：
 *		iDesc		文本显示描述符
 *		ctrl		控制字符的Ascii码
 *	返回值:
 *		0 			表示成功
 *		TEXT_ERROR	表示错误
 *		TEXT_EOF	表示要换页了
 ******************************************************/
int TextAddCtrlWord(int iDesc,char ctrl);


/*****************************************************
 *	在屏幕上将现阶段的Text打印出来
 *	参数：
 *		iDesc			描述符
 *	返回值：
 *		成功返回0
 *		失败返回-1
 *****************************************************/

int TextDisplay(int iDesc);

/*****************************************************
 *	清除这一页，且将画笔放在开头
 *	参数：
 *		iDesc			描述符
 *****************************************************/
void TextClean(int iDesc);




/* 
 *	初始化函数/退出函数
 *		对下层进行 初始化/退出
 *		并通过下层接口获取屏幕的lcd屏幕的信息
 */
int TextDisplayInit(void);

void TextDisplayExit(void);




#endif /* __TEXT__DISPLAY_H__ */







