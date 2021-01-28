#todo: remove lib_app dependency
LIB_CONV_YUV_A=$(BIN)/liballegro_conv_yuv.a $(LIB_APP_A)
LIB_CONV_YUV_DLL=$(BIN)/liballegro_conv_yuv.so $(LIB_APP_DLL)

#todo: remove lib_common{,enc} dependency
LIB_CONV_YUV_SRC:=\
  lib_conv_yuv/AL_RasterConvert.cpp\
  $(LIB_COMMON_SRC)\
  $(LIB_COMMON_ENC_SRC)\



LIB_CONV_YUV_OBJ:=$(LIB_CONV_YUV_SRC:%=$(BIN)/%.o)

$(BIN)/liballegro_conv_yuv.a: $(LIB_CONV_YUV_OBJ)

$(BIN)/liballegro_conv_yuv.so: $(LIB_CONV_YUV_OBJ)

liballegro_conv_yuv: liballegro_conv_yuv_dll liballegro_conv_yuv_a

liballegro_conv_yuv_dll: $(LIB_CONV_YUV_DLL)

liballegro_conv_yuv_a: $(LIB_CONV_YUV_A)

TARGETS+=liballegro_conv_yuv_dll

liballegro_conv_yuv_src: $(LIB_CONV_YUV_SRC)
	@echo $(LIB_CONV_YUV_SRC)

.PHONY: liballegro_conv_yuv liballegro_conv_yuv_dll liballegro_conv_yuv_a liballegro_conv_yuv_src
