CROSS_COMPILE  = arm-linux-
AS  =  $(CROSS_COMPILE)as
LD  =  $(CROSS_COMPILE)ld
CC  =  $(CROSS_COMPILE)gcc
CPP  =  $(CC)  -E 
AR  =  $(CROSS_COMPILE)ar 
NM  =  $(CROSS_COMPILE)nm 
STRIP  =  $(CROSS_COMPILE)strip 
OBJCOPY  =  $(CROSS_COMPILE)objcopy 
OBJDUMP  =  $(CROSS_COMPILE)objdump 


export  AS  LD  CC  CPP  AR  NM 
export  STRIP  OBJCOPY  OBJDUMP 

CFLAGS  :=  -Wall  -O0  -g    
CFLAGS 	+=	-I $(TOPDIR)/include/ 
#指定库
LDFLAGS  := -lm -lfreetype

export  CFLAGS  LDFLAGS     

TOPDIR  :=  $(shell  pwd)
export  TOPDIR 

TARGET := showfile

obj-y += display/
obj-y += encoding/
obj-y += fonts/
obj-y += text/
obj-y += key_events.o
obj-y += main.o

all  : 
	sudo chmod a+wr ./* -R
	make  -C  ./  -f  $(TOPDIR)/Makefile.build
	$(CC)  $(LDFLAGS)  -o  $(TARGET)  built-in.o


clean: 
	rm  -f  $(shell  find  -name  "*.o") 
	rm  -f	$(TARGET) 

distclean: 
	rm  -f  $(shell  find  -name  "*.o") 
	rm  -f  $(shell  find  -name  "*.d") 
	rm  -f  $(TARGET)

install:
	cp $(TARGET)  /home/plz/nfs/fs_linux_3.4.2/drv/
 
