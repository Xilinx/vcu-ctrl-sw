LIB_DECODER_A=$(BIN)/liballegro_decode.a
LIB_DECODER_DLL=$(BIN)/liballegro_decode.so

ifneq ($(findstring mingw,$(TARGET)),mingw)
	CFLAGS+=-fPIC
endif

LIB_DECODE_SRC+=\
		lib_decode/NalUnitParser.c\
		lib_decode/NalDecoder.c\
		lib_decode/HevcDecoder.c\
		lib_decode/AvcDecoder.c\
		lib_decode/FrameParam.c\
		lib_decode/SliceDataParsing.c\
		lib_decode/DefaultDecoder.c\
		lib_decode/lib_decode.c\
		lib_decode/BufferFeeder.c\
		lib_decode/Patchworker.c\
		lib_decode/DecoderFeeder.c\

ifneq ($(ENABLE_BYPASS),0)
endif

ifneq ($(ENABLE_MCU),0)
  LIB_DECODE_SRC+=lib_decode/DecChannelMcu.c
endif

ifneq ($(ENABLE_JPEG),0)
endif

LIB_DECODER_SRC:=\
  $(LIB_RTOS_SRC)\
  $(LIB_COMMON_SRC)\
  $(LIB_COMMON_DEC_SRC)\
  $(LIB_FPGA_SRC)\
  $(LIB_PARSING_SRC)\
  $(LIB_DECODE_SRC)\
  $(LIB_SCHEDULER_DEC_SRC)\
  $(LIB_SCHEDULER_SRC)\
  $(LIB_PERFS_SRC)\

ifneq ($(ENABLE_TRACES),0)
  LIB_DECODER_SRC+=\
    $(LIB_TRACE_SRC_DEC)
endif

LIB_DECODER_OBJ:=$(LIB_DECODER_SRC:%=$(BIN)/%.o)

$(LIB_DECODER_A): $(LIB_DECODER_OBJ)

$(LIB_DECODER_DLL): $(LIB_DECODER_OBJ)

liballegro_decode: liballegro_decode_dll liballegro_decode_a

liballegro_decode_dll: $(LIB_DECODER_DLL)

liballegro_decode_a: $(LIB_DECODER_A)

TARGETS+=$(LIB_DECODER_DLL)

liballegro_decode_src: $(LIB_DECODER_SRC)
	@echo $(LIB_DECODER_SRC)

ifneq ($(ENABLE_UNITTESTS),0)
UNITTEST+=$(shell find lib_decode/unittests -name "*.cpp")
UNITTEST+=$(LIB_DECODE_SRC)
UNITTEST+=$(LIB_TRACE_SRC_DEC)
endif

.PHONY: liballegro_decode liballegro_decode_dll liballegro_decode_a liballegro_decode_src

