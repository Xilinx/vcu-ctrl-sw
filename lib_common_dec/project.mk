LIB_COMMON_DEC_BASE_SRC:=\
	lib_common_dec/DecBuffers.c\

LIB_COMMON_DEC_SRC:=\
	$(LIB_COMMON_DEC_BASE_SRC)\
	lib_common_dec/DecHwScalingList.c\
	lib_common_dec/DecInfo.c\
	lib_common_dec/RbspParser.c\
	lib_common_dec/BufferDecodedPictureMeta.c\
	lib_common_dec/IpDecFourCC.c\

LIB_COMMON_DEC_MCU_SRC:=\
	$(LIB_COMMON_DEC_BASE_SRC)

UNITTEST+=$(shell find lib_common_dec/unittests -name "*.cpp")
UNITTEST+=$(LIB_COMMON_DEC_SRC)

