#hilink mcu sdk 根目录
PROJ_DIR := $(shell pwd)

#编译生成.o文件目录
OUT_PATH := $(PROJ_DIR)/build/out

#指定编译器(根据实际情况指定)
CC = gcc
#AR = ar

#引用头文件目录
INCLUDE_DIR := -I $(PROJ_DIR)/system/include \
			   -I $(PROJ_DIR)/include

#编译选项(根据实际情况配置)
BUILD_CFLAGS = $(INCLUDE_DIR)
#BUILD_CFLAGS := $(INCLUDE_DIR) -Wall -O2 -fno-builtin -Wstrict-prototypes -fomit-frame-pointer -DKBUILD_NO_NLS -Wno-unused-but-set-variable -Wno-main -Wno-missing-braces

#目标生成.a文件
TARGET_A := lib_mcu.a

#源文件.c
sources := $(wildcard ./*.c)
sources += $(wildcard ./system/*.c)

#中间文件.o
objects := $(patsubst %.c, $(OUT_PATH)/%.o, $(sources))

#==============================================================================
.PHONY: all
all: clean $(OUT_PATH) $(TARGET_A)

#编译.a
$(TARGET_A):$(objects)
	$(AR) -crv $@ $(objects)
	mv ./*.a $(OUT_PATH)/../liba

#编译.o
$(objects): $(OUT_PATH)/%.o: %.c
	$(CC) $(BUILD_CFLAGS) -c $< -o $@

#输出文件路径
$(OUT_PATH):
	mkdir -p $(OUT_PATH)/../liba
	mkdir -p $(OUT_PATH)/system

clean:
	rm -rf ./build