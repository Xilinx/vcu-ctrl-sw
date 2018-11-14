EXE_DECODER_SRC:=\
  exe_decoder/main.cpp\
  exe_decoder/crc.cpp\
  exe_decoder/IpDevice.cpp\
  exe_decoder/CodecUtils.cpp\
  exe_decoder/Conversion.cpp\
  $(LIB_APP_SRC)\

ifneq ($(ENABLE_BYPASS),0)
endif

-include exe_decoder/site.mk

EXE_DECODER_OBJ:=$(EXE_DECODER_SRC:%=$(BIN)/%.o)

ifneq ($(ENABLE_UNITTESTS),0)
UNITTEST+=$(shell find exe_decoder/unittests -name "*.cpp")
endif

$(BIN)/AL_Decoder.exe: $(EXE_DECODER_OBJ) $(LIB_REFDEC_A) $(LIB_DECODER_A)

$(BIN)/exe_decoder/%.o: CFLAGS+=$(SVNDEV)

$(BIN)/exe_decoder/%.o: INTROSPECT_FLAGS=-DAL_COMPIL_FLAGS='"$(CFLAGS)"'

$(BIN)/exe_decoder/%.o: INTROSPECT_FLAGS+=-DHAS_COMPIL_FLAGS=1

TARGETS+=$(BIN)/AL_Decoder.exe

exe_decoder_src: $(EXE_DECODER_SRC)
	@echo $(EXE_DECODER_SRC)

.PHONY: decoder exe_decoder_src


