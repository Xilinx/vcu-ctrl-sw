LIB_ENCODER_A=$(BIN)/liballegro_encode.a
LIB_ENCODER_DLL=$(BIN)/liballegro_encode.so

include lib_encode/project_src.mk

LIB_ISCHEDULER_ENC_A=$(BIN)/liballegro_encscheduler.a
LIB_ISCHEDULER_ENC_DLL=$(BIN)/liballegro_encscheduler.so


ifneq ($(ENABLE_MCU),0)
  ISCHEDULER_SRC+=lib_encode/DriverDataConversions.c
  ISCHEDULER_SRC+=lib_encode/EncSchedulerMcu.c
endif

LIB_ISCHEDULER_ENC_SRC:=\
  $(ISCHEDULER_SRC)\
  $(LIB_RATECTRL_SRC)\
  $(LIB_BUF_MNGT_SRC)\
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



ifneq ($(ENABLE_LIB_ISCHEDULER),0)
  ifneq ($(ENABLE_STATIC),0)
    $(warning the lib_ischeduler will be compiled in instead of being compiled as a library)
    ENABLE_LIB_ISCHEDULER:=0
  endif
endif

ifneq ($(ENABLE_LIB_ISCHEDULER),0)
  LIB_ENCODER_OBJ+=$(LIB_ISCHEDULER_ENC_DLL)
  # because we still want to show these as the lib_encoder_src
  LIB_ISCHEDULER_ENC_SRC_SHOW=$(LIB_ISCHEDULER_ENC_SRC)
else
  LIB_ENCODER_SRC+=$(LIB_ISCHEDULER_ENC_SRC)
  LIB_ISCHEDULER_ENC_SRC_SHOW=
endif

LIB_ENCODER_OBJ+=$(LIB_ENCODER_SRC:%=$(BIN)/%.o)

$(LIB_ENCODER_DLL): $(LIB_ENCODER_OBJ)

$(LIB_ENCODER_A): $(LIB_ENCODER_OBJ)

liballegro_encode: liballegro_encode_dll liballegro_encode_a

liballegro_encode_dll: $(LIB_ENCODER_DLL)

liballegro_encode_a: $(LIB_ENCODER_A)

TARGETS+=liballegro_encode_dll


liballegro_encode_src: $(LIB_ENCODER_SRC)
	@echo $(LIB_ENCODER_SRC) $(LIB_ISCHEDULER_ENC_SRC_SHOW)

.PHONY: liballegro_encode liballegro_encode_dll liballegro_encode_a liballegro_encoder_src
