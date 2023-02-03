THIS_EXE_DECODER:=$(call get-my-dir)

EXE_DECODER_SRC:=\
  $(THIS_EXE_DECODER)/main.cpp\
  $(THIS_EXE_DECODER)/crc.cpp\
  $(THIS_EXE_DECODER)/IpDevice.cpp\
  $(THIS_EXE_DECODER)/CodecUtils.cpp\
  $(THIS_EXE_DECODER)/Conversion.cpp\
  $(THIS_EXE_DECODER)/InputLoader.cpp\



ifneq ($(ENABLE_HIGH_DYNAMIC_RANGE),0)
  EXE_DECODER_SRC+=$(THIS_EXE_DECODER)/HDRWriter.cpp
endif

-include $(THIS_EXE_DECODER)/site.mk

EXE_DECODER_OBJ:=$(EXE_DECODER_SRC:%=$(BIN)/%.o)


$(BIN)/AL_Decoder.exe: $(EXE_DECODER_OBJ) $(LIB_REFDEC_A) $(LIB_REFALLOC_A) $(LIB_DECODER_A) $(LIB_APP_A)

$(BIN)/$(THIS_EXE_DECODER)/%.o: CFLAGS+=-pthread

$(BIN)/$(THIS_EXE_DECODER)/CodecUtils.cpp.o: CFLAGS+=$(SCM_REV)
$(BIN)/$(THIS_EXE_DECODER)/CodecUtils.cpp.o: CFLAGS+=$(SCM_BRANCH)
$(BIN)/$(THIS_EXE_DECODER)/CodecUtils.cpp.o: CFLAGS+=$(DELIVERY_BUILD_NUMBER)
$(BIN)/$(THIS_EXE_DECODER)/CodecUtils.cpp.o: CFLAGS+=$(DELIVERY_SCM_REV)
$(BIN)/$(THIS_EXE_DECODER)/CodecUtils.cpp.o: CFLAGS+=$(DELIVERY_DATE)
$(BIN)/$(THIS_EXE_DECODER)/CodecUtils.cpp.o: INTROSPECT_FLAGS=-DAL_COMPIL_FLAGS='"$(CFLAGS)"'
$(BIN)/$(THIS_EXE_DECODER)/CodecUtils.cpp.o: INTROSPECT_FLAGS+=-DHAS_COMPIL_FLAGS=1

AL_Decoder.exe: $(BIN)/AL_Decoder.exe
TARGETS+=AL_Decoder.exe

exe_decoder_src: $(EXE_DECODER_SRC)
	@echo $(EXE_DECODER_SRC)

.PHONY: exe_decoder_src
