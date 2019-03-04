LIB_APP_SRC:=lib_app/utils.cpp\
	     lib_app/convert.cpp\
	     lib_app/BufPool.cpp\
       lib_app/AllocatorTracker.cpp\



ifeq ($(findstring mingw,$(TARGET)),mingw)
else
  LIB_APP_SRC+=lib_app/console_linux.cpp
endif

