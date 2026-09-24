LIB_ENCODER_A=$(BIN)/liballegro_encode.a
LIB_ENCODER_DLL=$(BIN)/liballegro_encode.so

include lib_encode/project_src.mk


LIB_ENCODER_SRC:=\
  $(LIB_FPGA_SRC)\
  $(LIB_COMMON_SRC)\
  $(LIB_COMMON_ENC_SRC)\
  $(LIB_RTOS_SRC)\
  $(LIB_ENCODE_SRC)\
  $(LIB_LOG_SRC)\
  $(LIB_RATECTRL_SRC)\
  $(LIB_BUF_MNGT_SRC)\
  $(LIB_SCHEDULER_ENC_SRC)\
  $(LIB_BITSTREAM_SRC)\

LIB_ENCODER_OBJ+=$(LIB_ENCODER_SRC:%=$(BIN)/%.o)


$(LIB_ENCODER_DLL): $(LIB_ENCODER_OBJ)

$(LIB_ENCODER_A): $(LIB_ENCODER_OBJ)

liballegro_encode: liballegro_encode_dll liballegro_encode_a

liballegro_encode_dll: $(LIB_ENCODER_DLL)

liballegro_encode_a: $(LIB_ENCODER_A)

TARGETS+=liballegro_encode_dll


liballegro_encode_src: $(LIB_ENCODER_SRC)
	@echo $(LIB_ENCODER_SRC)

.PHONY: liballegro_encode liballegro_encode_dll liballegro_encode_a liballegro_encoder_src
