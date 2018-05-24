LIB_COMMON_ENC_SRC:=\
	lib_common_enc/L2PrefetchParam.c\
	lib_common_enc/EncBuffers.c\
	lib_common_enc/EncRecBuffer.c\
	lib_common_enc/IpEncFourCC.c\
	lib_common_enc/EncSize.c\
	lib_common_enc/ChooseLda.c\
	lib_common_enc/EncHwScalingList.c\
	lib_common_enc/Settings.c\

UNITTEST+=$(shell find lib_common_enc/unittests -name "*.cpp")
UNITTEST+=$(LIB_COMMON_ENC_SRC)
