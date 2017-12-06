LIB_COMMON_BASE_SRC:=\
	lib_common/Utils.c\
	lib_common/UtilsQp.c\
	lib_common/BufCommon.c\
	lib_common/AllocatorDefault.c\
	lib_common/ChannelResources.c\

LIB_COMMON_SRC:=\
	$(LIB_COMMON_BASE_SRC)\
	lib_common/MemDesc.c\
	lib_common/HwScalingList.c\
	lib_common/BufferAPI.c\
	lib_common/BufferSrcMeta.c\
	lib_common/BufferCircMeta.c\
	lib_common/BufferStreamMeta.c\
	lib_common/BufferAccess.c\
	lib_common/Fifo.c\
	lib_common/AvcLevelsLimit.c\
	lib_common/StreamBuffer.c\
	lib_common/FourCC.c\

LIB_COMMON_MCU_SRC:=\
	$(LIB_COMMON_BASE_SRC)

UNITTEST+=$(shell find lib_common/unittests -name "*.cpp")
UNITTEST+=$(LIB_COMMON_SRC)

