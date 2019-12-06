LIB_BITSTREAM_SRC:=\
	lib_bitstream/BitStreamLite.c\
	lib_bitstream/RbspEncod.c\
  lib_bitstream/Cabac.c\

ifneq ($(ENABLE_AVC),0)
	LIB_BITSTREAM_SRC+=lib_bitstream/AVC_RbspEncod.c
	LIB_BITSTREAM_SRC+=lib_bitstream/AVC_SkippedPict.c
endif

ifneq ($(ENABLE_HEVC),0)
	LIB_BITSTREAM_SRC+=lib_bitstream/HEVC_RbspEncod.c
	LIB_BITSTREAM_SRC+=lib_bitstream/HEVC_SkippedPict.c
endif



