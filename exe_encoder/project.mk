THIS_EXE_ENCODER:=$(call get-my-dir)

EXE_ENCODER_SRCS:=\
  $(THIS_EXE_ENCODER)/CodecUtils.cpp\
  $(THIS_EXE_ENCODER)/IpDevice.cpp\
  $(THIS_EXE_ENCODER)/main.cpp\
  $(THIS_EXE_ENCODER)/sink_bitstream_writer.cpp\
  $(THIS_EXE_ENCODER)/sink_frame_writer.cpp\
  $(THIS_EXE_ENCODER)/sink_md5.cpp\
  $(THIS_EXE_ENCODER)/MD5.cpp\
  $(THIS_EXE_ENCODER)/QPGenerator.cpp\
  $(LIB_CFG_SRC)\
  $(LIB_CONV_SRC)\
  $(LIB_APP_SRC)\

-include $(THIS_EXE_ENCODER)/site.mk

UNITTEST+=$(shell find $(THIS_EXE_ENCODER)/unittests -name "*.cpp")
UNITTEST+=$(THIS_EXE_ENCODER)/QPGenerator.cpp

EXE_ENCODER_OBJ:=$(EXE_ENCODER_SRCS:%=$(BIN)/%.o)

##############################################################
# get svn revision for debug message
##############################################################
SVNDEV:=-D'SVN_REV="$(shell svnversion -n . || echo 0)"'

$(BIN)/$(THIS_EXE_ENCODER)/main.cpp.o: CFLAGS+=$(SVNDEV)

$(BIN)/AL_Encoder.exe: $(EXE_ENCODER_OBJ) $(LIB_REFENC_A) $(LIB_ENCODER_A)

TARGETS+=$(BIN)/AL_Encoder.exe

