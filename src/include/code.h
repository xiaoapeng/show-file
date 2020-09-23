#ifndef __CODE_H___
#define __CODE_H___ 1

#include <string.h>

#define CODE_GB2312 		0
#define CODE_UTF8			1
#define CODE_UTF16_LE		2
#define CODE_UTF16_BE		3

#define CodeingFormatSize	64
extern char *CodeingFormat[CodeingFormatSize];



#ifdef CODEING_FORMAT
char *CodeingFormat[64] = {
	"gb2312",
	"utf8",
	"utf16-le",
	"utf16-be",
	NULL,
};
#endif /* CODEING_FORMAT */





#define SetCodeingFormat(code_num,str)	CodeingFormat[(code_num)] = (str)
#define GetCodeingFormatStr(code_num)			CodeingFormat[(code_num)]

static inline int GetCodeingFormatNum(const char *strCode)
{
	int i;
	for(i=0;GetCodeingFormatStr(i) != NULL;i++)
	{
		if(strcmp(GetCodeingFormatStr(i),strCode ) == 0)
			return i;
	}
	return -1;
}

#endif 

