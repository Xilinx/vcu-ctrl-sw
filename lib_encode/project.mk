LIB_ENCODER_A=$(BIN)/liballegro_encode.a
LIB_ENCODER_DLL=$(BIN)/liballegro_encode.so

ifneq ($(findstring mingw,$(TARGET)),mingw)
	CFLAGS+=-fPIC
endif

LIB_ENCODE_SRC+=\
	lib_encode/Com_Encoder.c\
	lib_encode/HEVC_Encoder.c\
	lib_encode/HEVC_Sections.c\
	lib_encode/AVC_Encoder.c\
	lib_encode/AVC_Sections.c\
	lib_encode/Sections.c\
	lib_encode/NalWriters.c\
	lib_encode/IP_Utils.c\
	lib_encode/IP_Stream.c\
	lib_encode/lib_encoder.c\
	lib_encode/SourceBufferChecker.c\
	lib_encode/LoadLda.c\

ifneq ($(ENABLE_AOM),0)
endif

ifneq ($(ENABLE_AV1),0)
endif

ifneq ($(ENABLE_JPEG),0)
  LIB_ENCODE_SRC+=lib_encode/JpegTables.c
endif

ifneq ($(ENABLE_RESIZE),0)
endif

LIB_ISCHEDULER_ENC_A=$(BIN)/liballegro_encscheduler.a
LIB_ISCHEDULER_ENC_DLL=$(BIN)/liballegro_encscheduler.so

ISCHEDULER_SRC:=\
	lib_encode/ISchedulerCommon.c\
	lib_encode/IScheduler.c\

ifneq ($(ENABLE_BYPASS),0)
endif

ifneq ($(ENABLE_MCU),0)
  ISCHEDULER_SRC+=lib_encode/DriverDataConversions.c
  ISCHEDULER_SRC+=lib_encode/SchedulerMcu.c
endif

LIB_ISCHEDULER_ENC_SRC:=\
  $(ISCHEDULER_SRC)\
  $(LIB_RATECTRL_SRC)\
  $(LIB_BUF_MNGT_SRC)\
  $(LIB_SCHEDULER_SRC)\
  $(LIB_SCHEDULER_ENC_SRC)\
  $(LIB_BITSTREAM_SRC)\

# needed but user can modify these carefully
#$(LIB_PERFS_SRC)\
#$(LIB_RTOS_SRC)\
#$(LIB_COMMON_SRC)\
#$(LIB_COMMON_ENC_SRC)\

LIB_ISCHEDULER_ENC_OBJ:=$(LIB_ISCHEDULER_ENC_SRC:%=$(BIN)/%.o)

$(LIB_ISCHEDULER_ENC_DLL): $(LIB_ISCHEDULER_ENC_OBJ)

$(LIB_ISCHEDULER_ENC_A): $(LIB_ISCHEDULER_ENC_OBJ)

LIB_ENCODER_SRC:=\
  $(LIB_FPGA_SRC)\
  $(LIB_COMMON_SRC)\
  $(LIB_COMMON_ENC_SRC)\
  $(LIB_RTOS_SRC)\
  $(LIB_ENCODE_SRC)\
  $(LIB_PERFS_SRC)\


ifneq ($(ENABLE_TRACES),0)
  LIB_ENCODER_SRC+=$(LIB_TRACE_SRC_ENC)
endif

ifneq ($(ENABLE_STATIC),0)
  $(warning the lib_ischeduler will be compiled in instead of being compiled as a library)
  ENABLE_LIB_ISCHEDULER:=0
endif

ifneq ($(ENABLE_LIB_ISCHEDULER),0)
  LIB_ENCODER_OBJ+=$(LIB_ISCHEDULER_ENC_DLL)
else
  LIB_ENCODER_SRC+=$(LIB_ISCHEDULER_ENC_SRC)
endif

LIB_ENCODER_OBJ+=$(LIB_ENCODER_SRC:%=$(BIN)/%.o)

$(LIB_ENCODER_DLL): $(LIB_ENCODER_OBJ)

$(LIB_ENCODER_A): $(LIB_ENCODER_OBJ)

liballegro_encode: liballegro_encode_dll liballegro_encode_a

liballegro_encode_dll: $(LIB_ENCODER_DLL)

liballegro_encode_a: $(LIB_ENCODER_A)

TARGETS+=liballegro_encode_dll

ifneq ($(ENABLE_UNITTESTS),0)
UNITTEST+=$(shell find lib_encode/unittests -name "*.cpp")
UNITTEST+=$(LIB_ENCODE_SRC)
UNITTEST+=$(ISCHEDULER_SRC)
UNITTEST+=$(LIB_SCHEDULER_SRC)
UNITTEST+=$(LIB_SCHEDULER_ENC_SRC)
UNITTEST+=$(LIB_COMMON_SRC)
UNITTEST+=$(LIB_BITSTREAM_SRC)
UNITTEST+=$(LIB_COMMON_ENC_SRC)
UNITTEST+=$(LIB_RATECTRL_SRC)
UNITTEST+=$(LIB_TRACE_SRC_ENC)
endif

liballegro_encode_src: $(LIB_ENCODER_SRC)
	@echo $(LIB_ENCODER_SRC)

.PHONY: liballegro_encode liballegro_encode_dll liballegro_encode_a liballegro_encoder_src
