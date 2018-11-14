THIS_EXE_ENCODER:=$(call get-my-dir)

PARSER_SRCS:=\
  $(THIS_EXE_ENCODER)/CfgParser.cpp\
  $(THIS_EXE_ENCODER)/Parser.cpp\
  $(THIS_EXE_ENCODER)/Tokenizer.cpp\

EXE_ENCODER_SRCS:=\
  $(THIS_EXE_ENCODER)/CodecUtils.cpp\
  $(THIS_EXE_ENCODER)/FileUtils.cpp\
  $(THIS_EXE_ENCODER)/IpDevice.cpp\
  $(THIS_EXE_ENCODER)/container.cpp\
  $(THIS_EXE_ENCODER)/main.cpp\
  $(THIS_EXE_ENCODER)/sink_bitstream_writer.cpp\
  $(THIS_EXE_ENCODER)/sink_frame_writer.cpp\
  $(THIS_EXE_ENCODER)/sink_md5.cpp\
  $(THIS_EXE_ENCODER)/MD5.cpp\
  $(THIS_EXE_ENCODER)/ROIMngr.cpp\
  $(THIS_EXE_ENCODER)/EncCmdMngr.cpp\
  $(THIS_EXE_ENCODER)/QPGenerator.cpp\
  $(THIS_EXE_ENCODER)/CommandsSender.cpp\
  $(PARSER_SRCS)\
  $(LIB_CONV_SRC)\
  $(LIB_APP_SRC)\

ifneq ($(ENABLE_BYPASS),0)
endif

ifneq ($(ENABLE_TWOPASS),0)
  EXE_ENCODER_SRCS+=$(THIS_EXE_ENCODER)/TwoPassMngr.cpp
endif

-include $(THIS_EXE_ENCODER)/site.mk

ifneq ($(ENABLE_UNITTESTS),0)
UNITTEST+=$(shell find $(THIS_EXE_ENCODER)/unittests -name "*.cpp")
UNITTEST+=$(THIS_EXE_ENCODER)/ROIMngr.cpp
UNITTEST+=$(THIS_EXE_ENCODER)/FileUtils.cpp
UNITTEST+=$(THIS_EXE_ENCODER)/QPGenerator.cpp
UNITTEST+=$(THIS_EXE_ENCODER)/EncCmdMngr.cpp
UNITTEST+=$(PARSER_SRCS)
endif

EXE_ENCODER_OBJ:=$(EXE_ENCODER_SRCS:%=$(BIN)/%.o)

##############################################################
# get svn revision for debug message
##############################################################
SVNDEV:=-D'SVN_REV="$(shell svnversion -n . | grep -v Unversioned || echo 0)"'

$(BIN)/$(THIS_EXE_ENCODER)/main.cpp.o: CFLAGS+=$(SVNDEV)

$(BIN)/$(THIS_EXE_ENCODER)/main.cpp.o: INTROSPECT_FLAGS=-DAL_COMPIL_FLAGS='"$(CFLAGS)"'

$(BIN)/$(THIS_EXE_ENCODER)/main.cpp.o: INTROSPECT_FLAGS+=-DHAS_COMPIL_FLAGS=1

ifneq ($(ENABLE_INTROSPECTION),0)
$(BIN)/$(THIS_EXE_ENCODER)/main.cpp.o: generated/Printer.h

$(BIN)/$(THIS_EXE_ENCODER)/ParserMain.cpp.o: generated/Printer.h

generated/Printer.h:
	@echo "Generate introspection files"
	./scripts/allegro_introspection.sh
endif

$(BIN)/AL_Encoder.exe: $(EXE_ENCODER_OBJ) $(LIB_REFENC_A) $(LIB_ENCODER_A)

TARGETS+=$(BIN)/AL_Encoder.exe

ifndef ($(ENABLE_LIB_ISCHEDULER),0)
$(BIN)/AL_Encoder.sh: $(BIN)/AL_Encoder.exe
	@echo "Generate script $@"
	$(shell echo 'LD_LIBRARY_PATH=$(BIN) $(BIN)/AL_Encoder.exe "$$@"' > $@ && chmod a+x $@)

TARGETS+=$(BIN)/AL_Encoder.sh
endif

# for compilation time reduction (we don't need this to be optimized)
$(BIN)/$(THIS_EXE_ENCODER)/CfgParser.cpp.o: CFLAGS+=-O0

EXE_CFG_PARSER_SRCS:=\
  $(THIS_EXE_ENCODER)/ParserMain.cpp \
  $(PARSER_SRCS)\

EXE_CFG_PARSER_OBJ:=$(EXE_CFG_PARSER_SRCS:%=$(BIN)/%.o)

$(BIN)/AL_CfgParser.exe: $(EXE_CFG_PARSER_OBJ) $(LIB_ENCODER_A)

TARGETS+=$(BIN)/AL_CfgParser.exe

exe_encoder_src: $(EXE_ENCODER_SRCS) $(EXE_CFG_PARSER_SRCS)
	@echo $(EXE_ENCODER_SRCS) $(EXE_CFG_PARSER_SRCS)

$(BIN)/$(THIS_EXE_ENCODER)/unittests/commandsparser.cpp.o: CFLAGS+=-Wno-missing-field-initializers

.PHONY: exe_encoder_src
