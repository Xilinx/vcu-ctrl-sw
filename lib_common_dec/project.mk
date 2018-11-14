LIB_COMMON_DEC_SRC:=\
	lib_common_dec/DecBuffers.c\
	lib_common_dec/DecHwScalingList.c\
	lib_common_dec/DecInfo.c\
	lib_common_dec/RbspParser.c\
	lib_common_dec/IpDecFourCC.c\

ifneq ($(ENABLE_UNITTESTS),0)
UNITTEST+=$(shell find lib_common_dec/unittests -name "*.cpp")
UNITTEST+=$(LIB_COMMON_DEC_SRC)
endif

