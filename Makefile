CFLAGS+=-O3
CFLAGS+=-g0

-include config.mk

##############################################################
# cross build
##############################################################
CROSS_COMPILE?=

CXX:=$(CROSS_COMPILE)g++
CC:=$(CROSS_COMPILE)gcc
AS:=$(CROSS_COMPILE)as
AR:=$(CROSS_COMPILE)ar
NM:=$(CROSS_COMPILE)nm
LD:=$(CROSS_COMPILE)ld
OBJDUMP:=$(CROSS_COMPILE)objdump
OBJCOPY:=$(CROSS_COMPILE)objcopy
RANLIB:=$(CROSS_COMPILE)ranlib
STRIP:=$(CROSS_COMPILE)strip
SIZE:=$(CROSS_COMPILE)size

TARGET:=$(shell $(CC) -dumpmachine)

all: true_all

-include compiler.mk

##############################################################
# basic build rules and external variables
##############################################################
include ctrlsw_version.mk
include encoder_defs.mk
include base.mk

##############################################################
# Libraries
##############################################################
-include lib_fpga/project.mk
include lib_app/project.mk

include lib_cfg/project.mk
-include lib_common/project.mk
-include lib_common_enc/project.mk
-include lib_rtos/project.mk
-include lib_preprocess/project.mk

-include lib_buf_mngt/project.mk
-include lib_rate_ctrl/project.mk
-include lib_bitstream/project.mk
-include lib_scheduler/project.mk
-include lib_scheduler_enc/project.mk
-include lib_perfs/project.mk
-include lib_encode/project.mk

-include lib_conv_yuv/project.mk

ifneq ($(ENABLE_DECODER),0)
-include lib_common_dec/project.mk
endif

-include ref.mk

##############################################################
# AL_Decoder
##############################################################
ifneq ($(ENABLE_DECODER),0)
  -include lib_parsing/project.mk
  -include lib_scheduler_dec/project.mk
  -include lib_decode/project.mk
  include exe_decoder/project.mk
endif

##############################################################
# AL_Encoder
##############################################################
-include exe_encoder/project.mk

##############################################################
# AL_Compress
##############################################################
ifneq ($(ENABLE_COMP),0)
  -include exe_compress/project.mk
endif

##############################################################
# AL_Decompress
##############################################################
ifneq ($(ENABLE_COMP),0)
  -include exe_decompress/project.mk
endif

##############################################################
# AL_PerfMonitor
##############################################################
ifneq ($(ENABLE_PERF),0)
  -include exe_perf_monitor/project.mk
endif

##############################################################
# Unit tests
##############################################################
-include test/test.mk

##############################################################
# Environment tests
##############################################################
-include exe_test_env/project.mk

##############################################################
# tools
##############################################################
-include app_mcu/integration_tests.mk
-include exe_vip/project.mk

true_all: $(TARGETS)

.PHONY: true_all clean all
