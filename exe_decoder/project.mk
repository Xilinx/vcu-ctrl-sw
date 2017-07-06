EXE_DECODER_SRC:=\
  exe_decoder/main.cpp\
  exe_decoder/crc.cpp\
  exe_decoder/IpDevice.cpp\
  exe_decoder/CodecUtils.cpp\
  exe_decoder/Conversion.cpp\
	$(LIB_APP_SRC)\

-include exe_decoder/site.mk

EXE_DECODER_OBJ:=$(EXE_DECODER_SRC:%=$(BIN)/%.o)

$(BIN)/AL_Decoder.exe: $(EXE_DECODER_OBJ) $(LIB_REFDEC_A) $(LIB_DECODER_A)

$(EXE_DECODER_OBJ): CFLAGS+=$(SVNDEV)

TARGETS+=$(BIN)/AL_Decoder.exe

.PHONY: decoder


