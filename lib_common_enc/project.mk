LIB_COMMON_ENC_SRC:=\
	lib_common_enc/EncBuffers.c\
	lib_common_enc/EncRecBuffer.c\
	lib_common_enc/IpEncFourCC.c\
	lib_common_enc/EncSize.c\
	lib_common_enc/EncHwScalingList.c\
	lib_common_enc/Settings.c\

ifneq ($(ENABLE_RESIZE),0)
endif

ifneq ($(ENABLE_UNITTESTS),0)
UNITTEST+=$(shell find lib_common_enc/unittests -name "*.cpp")
UNITTEST+=$(LIB_COMMON_ENC_SRC)
endif
