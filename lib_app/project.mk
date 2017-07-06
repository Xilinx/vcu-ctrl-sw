LIB_APP_SRC:=

LIB_APP_SRC+=lib_app/utils.cpp\
	     lib_app/convert.cpp\
	     lib_app/BufPool.c\
	     lib_app/BufferMetaFactory.c\


ifeq ($(findstring mingw,$(TARGET)),mingw)
	LIB_APP_SRC+=lib_app/console_windows.cpp
else
	LIB_APP_SRC+=lib_app/console_linux.cpp
endif

UNITTEST+=$(shell find lib_app/unittests -name "*.cpp")
UNITTEST+=$(LIB_APP_SRC)
