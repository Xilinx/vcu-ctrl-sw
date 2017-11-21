LIB_CONV_SRC:=\
  lib_conv_yuv/AL_NvxConvert.cpp\

ifneq ($(ENABLE_LG_COMP),0)
  LIB_CONV_SRC+=\

endif

ifneq ($(ENABLE_LIBREF),0)
  LIB_CONV_SRC+=\
  $(LIB_REFFBC_SRC)\

endif
