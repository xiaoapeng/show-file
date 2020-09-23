
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>
#include <string.h>
#include <linux/input.h>  
#include <getopt.h>

#include <text_ctrl.h>
#include <code.h>

#ifdef CONFIG
#include <config.h>

#endif


extern int EventInit(void);
extern int EventExit(void);
extern int EventRead(void);



#ifndef CONFIG
#define DEFAULT_WORD_SIZE  		9					/* 字高为8pt */
#define DEFAULT_LINE_SPACING	5					/* 行距为5pt */
#define DEFAULT_WORD_SPACING	3					/* 字距为3pt */
#define DEFAULT_WORD_COLOUR		0x000000			/* 黑色 */
#define DEFAULT_BACKG_COLOUR	0xFFFA8F			/* 淡黄色 */
#define DEFAULT_FONT			"simsun"			/* 宋体 */
#define DEFAULT_FORMAT			CODE_AUTO_DISCERN	/* 默认自动识别编码，暂时不能识别gb2312,使用gb2312请指定编码 */
#define DEFAULT_DIRECTION		HORIZONTAL			/* 默认使用横屏 */
#endif




static struct text_set tTextSet = {
	.line_spacing 	= DEFAULT_LINE_SPACING,
	.word_size 		= DEFAULT_WORD_SPACING,
	.word_colour 	= DEFAULT_WORD_COLOUR,
	.backg_colour 	= DEFAULT_BACKG_COLOUR,
	.word_spacing 	= DEFAULT_WORD_SPACING,
	.font	 		= DEFAULT_FONT,
	.coding_format	= DEFAULT_FORMAT,
	.direction		= DEFAULT_DIRECTION,
};



int Start(void)
{
	int ctrl;
	struct text_ctrl * ptTextCtrl;
	if(TextCtrlInit() == -1)
	{
		printf("Error initializing text ctrl\n");
		return -1 ;
	}
	if(EventInit() == -1)
	{
		printf("Error initializing event management\n");
		TextCtrlExit();
		return -1;
	}
	
	ptTextCtrl = TextOpen(&tTextSet);
	if(ptTextCtrl == NULL)
	{
		printf("error\n");
		return -1;
	}
	TextShow(ptTextCtrl,SHOW_CURRENT_PAGE);

	while(1)
	{
		ctrl = EventRead();
		if(ctrl == -1)
		{
			break;
		}
		
		switch (ctrl)
		{
			case KEY_DOWN:
			
			if(TextShow(ptTextCtrl,SHOW_NEXT_PAGE) == TEXT_NO_NEXT)
			{
				printf("not's next\n");
			}
			break;
			case KEY_UP:
			if(TextShow(ptTextCtrl,SHOW_PREV_PAGE) == TEXT_NO_PREV)
			{
				printf("not's prev\n");
			}
			break;
			case KEY_ENTER:
			if(TextShow(ptTextCtrl,SHOW_CURRENT_PAGE) == TEXT_NO_CURRENT)
			{
				printf("not's current\n");
			}
			break;
			default:
			goto quit;
			break;
		}
	}
	quit:
	TextClose(ptTextCtrl);
	EventExit();
	TextCtrlExit();
	return 0;
}




char *g_Help = 	"-p	横屏                     			\n"
				"-s	<字号>	                     	\n"
				"-w	<字距>	                       	\n"
				"-l	<行距>                          	\n"
				"-c	<编码格式>             			    \n"
				"-o	<字体颜色>                    		\n"
				"-b	<背景颜色>              			\n"
				"-f	<字体样式>                    		\n"
				"-z	打印支持的编码 						\n"
				"-h	打印帮助信息							\n";






int main(int argc, char **argv)
{
	int iError;
	int iErrVar;
	char  **CodeingPos = NULL;
	/*
     *	-p  竖屏
     *	-s	<字体大小>
     *	-w	<字距>
     * 	-l	<行距>
     *	-c	<编码格式>
     *	-o	<字体颜色>
     *	-b	<背景颜色>
     *	-f	<字体样式>
     *	-z	打印支持的编码
     *	-h	打印帮助信息
     */
	while ((iError = getopt(argc, argv, "ps:w:l:c:o:b:f:zh")) != -1)
	{
		switch (iError)
		{
			case 'p':
				tTextSet.direction = VERTICAL;
				break;
			case 's':
				iErrVar = sscanf(optarg,"%d",&tTextSet.word_size);
				if(iErrVar != 1)
				{
					fprintf(stderr,"error : -s Specifies the font size,need a number\n"
					               "\"-s 13\"\n");
					return -1;
				}
				if(tTextSet.word_size <9)
				{
					fprintf(stderr,	"error : Please enter a font size greater than 9\n"
									"		 -s <Font_size> satisfied with:Font_size>=9\n");
					return -1;
				}
				break;			
			case 'w':
				iErrVar = sscanf(optarg,"%d",&tTextSet.word_spacing);
				if(iErrVar != 1)
				{
					fprintf(stderr,"error : -w Specify kerning,need a number\n"
					               "\"-w 4\"\n");
					return -1;
				}
				break;
				
			case 'l':
				iErrVar = sscanf(optarg,"%d",&tTextSet.line_spacing);
				if(iErrVar != 1)
				{
					fprintf(stderr,"error : -l Specify line spacing,need a number\n"
					               "\"-l 4\"\n");
					return -1;
				}
				break;
				
			case 'c':
				tTextSet.coding_format = GetCodeingFormatNum(optarg);
				if(tTextSet.coding_format == -1)
				{
					fprintf(stderr,"error : -c Specify encoding\n"
					               "		Use -z to query encoding\n");
					return -1;
				}
				break;
				
			case 'o':
				iErrVar = sscanf(optarg,"0x%x",&tTextSet.word_colour);
				if(iErrVar != 1)
				{
					fprintf(stderr,"error : -o Specify font color,Please enter hexadecimal\n"
					               "\"-o  0x0feecc\"\n");
					return -1;
				}
				break;
				
			case 'b':
				iErrVar = sscanf(optarg,"0x%x",&tTextSet.backg_colour);
				if(iErrVar != 1)
				{
					fprintf(stderr,"error : -b Specify background color,Please enter hexadecimal\n"
					               "\"-b  0x0feecc\"\n");
					return -1;
				}
				break;
				
			case 'z':
				CodeingPos = CodeingFormat;
				printf("Supported encoding:\n");
				while(*CodeingPos)
				{
					printf("%s\n",*CodeingPos);
					CodeingPos++;
				}
				return 0;
				break;				
			case 'f':
				tTextSet.font = optarg;
				break;

			case '?':
				printf("error:\n");
			case 'h':
				printf("%s",g_Help);
				return -1;
				break;
		}


	}
	if(optind == argc)
	{
		fprintf(stderr,"error : Operand not found,Please specify the name of the file to be opened\n");
		return -1;
	}
	tTextSet.filename = argv[optind];

	return Start();
}




