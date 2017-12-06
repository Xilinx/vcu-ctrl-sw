LIB_COMMON_ENC_BASE_SRC:=\
	lib_common_enc/L2PrefetchParam.c\
	lib_common_enc/EncBuffers.c\
	lib_common_enc/EncRecBuffer.c\
	lib_common_enc/IpEncFourCC.c\
	lib_common_enc/EncSize.c\

LIB_COMMON_ENC_SRC:=\
	$(LIB_COMMON_ENC_BASE_SRC)\
	lib_common_enc/EncHwScalingList.c\
	lib_common_enc/Settings.c\

LIB_COMMON_ENC_MCU_SRC:=\
	$(LIB_COMMON_ENC_BASE_SRC)

UNITTEST+=$(shell find lib_common_enc/unittests -name "*.cpp")
