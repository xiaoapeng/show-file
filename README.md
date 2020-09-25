# 小说阅读器V1.0-linux最小系统版
## 概述
	平台:s3c2440
	编译器：arm-linux-gcc 4.3.2
	语言：C语言
	依赖库：freetype库
	功能： 支持4种编码：gb2312、utf8、utf16-le、utf16-be；
	支持所有字体；
		   支持屏幕方向的变化：横屏、竖屏；
		   支持不同的字体大小；
		   支持字距、行距的变化；
		   支持改变颜色：字体颜色、背景颜色
		   支持扩展：编码模块的扩展、显示模块的扩展、位图获取模块的扩展；
		   支持字库和字体文件；
	本程序参考过韦东山教学的部分框架图，代码未参考！
	总计7000-8000行代码，除去asicc.c从内核拷贝，其余原创代码3000-4000多行
	   效果图：
[![](https://raw.githubusercontent.com/xiaoapeng/show-file/master/res/%E6%95%88%E6%9E%9C%E5%9B%BE.jpg)](https://raw.githubusercontent.com/xiaoapeng/show-file/master/res/%E6%95%88%E6%9E%9C%E5%9B%BE.jpg)
## 编译
	切换至src目录，vim Makefile设置编译器或者配置编译器前缀，:wq保存后，执行make。

## 使用
#### 准备：
	在此之前，请将freetype构建到目标开发板上；
	在任意目录下新建目录，将编译过的二进制文件showfile cp在该目录；
	将Fonts_or_HZK16 和 test_files 下的所有文件复制到该目录；
#### 	命令行的使用：
			showfile  [option]  <filename>
			-p  竖屏
			-s	<字体大小>
			-w	<字距>
			-l	<行距>
			-c	<编码格式>
			-o	<字体颜色>
			-b	<背景颜色>
			-f	<字体样式>
			-z	打印支持的编码
			-h	打印帮助信息
	注意：对于gb2312编码的文件，一定要指定编码格式，对于这种编码，因为没有使用编码库此程序暂时无法识别，且gb2312编码的文件暂时无法变化大小，因为还未实现2值位图的放大与缩小。
#### 		翻页操作：
  对于有按键功能的开发板，请配置内核设备树，使之映射为KEY_UP、KEY_DOWN、KEY_ENTER按键。
	KEY_ENTER为唤醒，当内核10分钟熄屏后会按该键会唤醒屏幕，且重新显示。
	KEY_UP为翻到上一页。
	KEY_DOWN为翻到下一页

## 移植
	完善开发板fb驱动的平台设备结构体，填写正确的屏幕信息，在应用层会获取使用，填写错误的可能会造成堆栈溢出。
	没有按键的可以更改main.c中的start函数，使用其他方法进行翻页等操作。

## 程序的扩展性与结构
	程序以面向对象的思想进行编程，完成了面向对象的封装、继承、多态
	程序的解耦性特别好，可以添加更多的模块以支持更多的编码和字库
	吸收linux子系统的思想，抽象出一层一层的通用管理核心层。
	程序抽象出三大通用层：分别为display、encoding、fonts
#### 		display：
	由disp-manarger-core.c 进行管理，暂时只有fb-dev.c一个子模块，主要提供通过xy坐标显示像素点的函数，可以写出更多的子模块去支持更多的屏幕。
#### 		encoding：
	由encoding_manager.c进行管理，此时有gb2312.c、unicode.c 两个子模块、主要提供编码的转化，编码的识别等功能，可添加更多子模块，来完成更多编码的识别与转化。
#### 		fonts：
	由fonts_manager.c进行管理，主要提供位图的获取，此时有freetype_module.c、hzk_module.c等模块，也可以进行扩展。
#### 其他：
##### text_display：
	fonts和display向text_display提供接口，完成小说页的构建。
##### text_ctrl：
	text_display、text_stack、encoding为text_ctrl提供接口，完成对小说的各项操作。
##### 层次图
[![](https://raw.githubusercontent.com/xiaoapeng/show-file/master/res/%E5%B1%82%E6%AC%A1%E5%9B%BE.jpg)](https://raw.githubusercontent.com/xiaoapeng/show-file/master/res/%E5%B1%82%E6%AC%A1%E5%9B%BE.jpg)
