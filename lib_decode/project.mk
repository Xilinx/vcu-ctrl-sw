LIB_DECODER_A=$(BIN)/liballegro_decode.a
LIB_DECODER_DLL=$(BIN)/liballegro_decode.so

-include lib_decode/project_mcu.mk

ifneq ($(findstring mingw,$(TARGET)),mingw)
	CFLAGS+=-fPIC
endif

LIB_DECODE_SRC+=\
		lib_decode/NalUnitParser.c\
		lib_decode/HevcDecoder.c\
		lib_decode/AvcDecoder.c\
		lib_decode/FrameParam.c\
		lib_decode/SliceDataParsing.c\
		lib_decode/DefaultDecoder.c\
		lib_decode/lib_decode.c\
		lib_decode/BufferFeeder.c\
		lib_decode/Patchworker.c\
		lib_decode/DecoderFeeder.c\

LIB_DECODER_SRC:=\
  $(LIB_RTOS_SRC)\
  $(LIB_COMMON_SRC)\
  $(LIB_COMMON_DEC_SRC)\
  $(LIB_FPGA_SRC)\
  $(LIB_PARSING_SRC)\
  $(LIB_DECODE_SRC)\
  $(LIB_SCHEDULER_DEC_SRC)\

LIB_DECODER_OBJ:=$(LIB_DECODER_SRC:%=$(BIN)/%.o)

$(LIB_DECODER_A): $(LIB_DECODER_OBJ)

$(LIB_DECODER_DLL): $(LIB_DECODER_OBJ)

liballegro_decode: liballegro_decode_dll liballegro_decode_a

liballegro_decode_dll: $(LIB_DECODER_DLL)

$(LIB_DECODER_DLL):
	$(Q)$(CC) $(CFLAGS) -shared -Wl,-soname,$@.$(MAJOR) -o "$@.$(VERSION)" $^ $(LDFLAGS)
	@echo "LD $@"
	@rm -f $@
	@ln -s "$(@:$(BIN)/%=%).$(VERSION)" $@

liballegro_decode_a: $(LIB_DECODER_A)

TARGETS+=$(LIB_DECODER_DLL)

.PHONY: liballegro_decode liballegro_decode_dll liballegro_decode_a

UNITTEST+=$(shell find lib_decode/unittests -name "*.cpp")
UNITTEST+=$(LIB_DECODE_SRC)

