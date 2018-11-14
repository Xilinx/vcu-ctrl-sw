LIB_APP_SRC:=

LIB_APP_SRC+=lib_app/utils.cpp\
	     lib_app/convert.cpp\
	     lib_app/BufPool.cpp\
	     lib_app/BufferMetaFactory.c\
       lib_app/AllocatorTracker.cpp\

ifneq ($(ENABLE_FBC),0)
endif

ifeq ($(findstring mingw,$(TARGET)),mingw)
else
  LIB_APP_SRC+=lib_app/console_linux.cpp
endif

ifneq ($(ENABLE_UNITTESTS),0)
UNITTEST+=$(shell find lib_app/unittests -name "*.cpp")
UNITTEST+=$(LIB_APP_SRC)
endif
