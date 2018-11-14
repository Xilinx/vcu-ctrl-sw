LIB_CONV_SRC:=\
  lib_conv_yuv/AL_NvxConvert.cpp\

ifneq ($(ENABLE_SRC_ENDIANESS),0)
endif

ifneq ($(ENABLE_LG_COMP),0)
endif

ifneq ($(ENABLE_LIBREF),0)
  ifneq ($(ENABLE_TILE_SRC),0)
    LIB_CONV_SRC+=$(LIB_FBCSTANDALONE_SRC)
    LIB_CONV_SRC+=$(LIB_REFFBC_SRC)
  endif
endif
