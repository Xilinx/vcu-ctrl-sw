LIB_BITSTREAM_SRC:=\
	lib_bitstream/BitStreamLite.c\
	lib_bitstream/RbspEncod.c\
	lib_bitstream/HEVC_RbspEncod.c\
	lib_bitstream/HEVC_SkippedPict.c\
	lib_bitstream/AVC_RbspEncod.c\
	lib_bitstream/AVC_SkippedPict.c\

ifneq ($(ENABLE_AOM),0)
endif

ifneq ($(ENABLE_AV1),0)
endif

ifneq ($(ENABLE_UNITTESTS),0)
UNITTEST+=$(shell find lib_bitstream/unittests -name "*.cpp")
endif
