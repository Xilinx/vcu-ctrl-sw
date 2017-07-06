# build configuration
ENABLE_64BIT?=1
ENABLE_ANDROID?=0
ENABLE_COMP?=1
ENABLE_STATIC?=0
ENABLE_DECODER?=1
ENABLE_UNITTESTS?=1
BIN?=bin

CONFIG?=config.h
ifneq ($(CONFIG),)
CFLAGS+=-include $(CONFIG)
endif

INCLUDES+=-I.
INCLUDES+=-Iinclude
INCLUDES+=-I./extra/include

#CFLAGS+=-Werror
CFLAGS+=-Wall -Wextra

ifeq ($(ENABLE_64BIT),0)
  # force 32 bit compilation
  ifneq (,$(findstring x86_64,$(TARGET)))
    CFLAGS+=-m32
    LDFLAGS+=-m32
    TARGET:=i686-linux-gnu
  endif
endif


##############################################################
# Reference model specifics
##############################################################
# warning derogations. Ongoing task: make this list empty
$(BIN)/lib_parsing/common_syntax.c.o: CFLAGS+=-Wno-type-limits
$(BIN)/lib_parsing/common_syntax.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_parsing/DPB.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_parsing/PpsParsing.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_parsing/SeiParsing.c.o: CFLAGS+=-Wno-type-limits
$(BIN)/lib_parsing/SliceHdrParsing.c.o: CFLAGS+=-Wno-type-limits
$(BIN)/lib_parsing/SliceHdrParsing.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_parsing/SpsParsing.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_parsing/VpsParsing.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_rate_ctrl/GopMngrCustom.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_rate_ctrl/GopMngrDefault.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_rate_ctrl/GopMngrDefaultVp9.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_rate_ctrl/InitHwRC.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_rate_ctrl/RateCtrlCBR.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_rate_ctrl/RateCtrlLowLat.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_rate_ctrl/RateCtrlTest.c.o: CFLAGS+=-Wno-unused-parameter
$(BIN)/lib_rate_ctrl/RateCtrlVBR.c.o: CFLAGS+=-Wno-unused-parameter

# microblaze implementation of the muldiv. No tests yet on this, so not changing the code yet.
$(BIN)/lib_rate_ctrl/RateCtrlUtils.c.o:CFLAGS+=-Wno-sign-compare
