LIB_DECODER_A=$(BIN)/liballegro_decode.a
LIB_DECODER_DLL=$(BIN)/liballegro_decode.so

include lib_decode/project_src.mk


LIB_DECODER_SRC:=\
  $(LIB_RTOS_SRC)\
  $(LIB_COMMON_SRC)\
  $(LIB_COMMON_DEC_SRC)\
  $(LIB_FPGA_SRC)\
  $(LIB_PARSING_SRC)\
  $(LIB_DECODE_SRC)\
  $(LIB_SCHEDULER_DEC_SRC)\
  $(LIB_PERFS_SRC)\



LIB_DECODER_OBJ:=$(LIB_DECODER_SRC:%=$(BIN)/%.o)

$(LIB_DECODER_A): $(LIB_DECODER_OBJ)

$(LIB_DECODER_DLL): $(LIB_DECODER_OBJ)

liballegro_decode: liballegro_decode_dll liballegro_decode_a

liballegro_decode_dll: $(LIB_DECODER_DLL)

liballegro_decode_a: $(LIB_DECODER_A)

TARGETS+=$(LIB_DECODER_DLL)

liballegro_decode_src: $(LIB_DECODER_SRC)
	@echo $(LIB_DECODER_SRC)


.PHONY: liballegro_decode liballegro_decode_dll liballegro_decode_a liballegro_decode_src

