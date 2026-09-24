# Flags
OUR_CFLAGS:=-O3 -g0
OUR_LDFLAGS:=

# Compiler
CROSS_COMPILE?=
CXX:=$(CROSS_COMPILE)g++
CC:=$(CROSS_COMPILE)gcc

# Same as: ar/nm/ranlib --plugin=<liblto_plugin.so>
BINUTILS_PREFIX?=
AR:=$(CROSS_COMPILE)$(BINUTILS_PREFIX)ar
NM:=$(CROSS_COMPILE)$(BINUTILS_PREFIX)nm
RANLIB:=$(CROSS_COMPILE)$(BINUTILS_PREFIX)ranlib

AS:=$(CROSS_COMPILE)as
LD:=$(CROSS_COMPILE)ld
OBJDUMP:=$(CROSS_COMPILE)objdump
OBJCOPY:=$(CROSS_COMPILE)objcopy
STRIP:=$(CROSS_COMPILE)strip
SIZE:=$(CROSS_COMPILE)size

TARGET:=$(shell $(CC) -dumpmachine)

ifeq ($(findstring linux,$(TARGET)),linux)
  OUR_CFLAGS+=-pthread
  ifeq ($(ENABLE_STATIC), 1)
	  OUR_LDFLAGS+=-Wl,--whole-archive -pthread -Wl,--no-whole-archive
  else
	  OUR_LDFLAGS+=-pthread
  endif
endif

THEIR_CFLAGS:=${CFLAGS}
THEIR_LDFLAGS:=${LDFLAGS}

CFLAGS:=${OUR_CFLAGS} ${THEIR_CFLAGS}
LDFLAGS:=${OUR_LDFLAGS} ${THEIR_LDFLAGS}

SCM_REV_SW:=-DSCM_REV_SW=\"$(shell git rev-parse HEAD 2> /dev/null || echo 0)\"
SCM_BRANCH=-DSCM_BRANCH=\"$(shell git rev-parse --abbrev-ref HEAD 2> /dev/null || echo unknown)\"

REQUIRED_MAKE_VERSION:=4.0
ifneq ($(REQUIRED_MAKE_VERSION), $(firstword $(sort $(MAKE_VERSION) $(REQUIRED_MAKE_VERSION))))
  $(error Bad 'make' version $(MAKE_VERSION), required a version $(REQUIRED_MAKE_VERSION) or higher)
endif

define get-my-dir
$(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
endef

ifneq ($(MAKECMDGOALS), clean)
  ifeq ($(wildcard include/config.h),)
    $(error config.h does not exist, cannot compile)
  endif
  include config.mk
else
  -include config.mk
endif


-include delivery.mk

DELIVERY_BUILD_NUMBER?=-DDELIVERY_BUILD_NUMBER=0
DELIVERY_SCM_REV?=-DDELIVERY_SCM_REV=\"unknown\"
DELIVERY_DATE?=-DDELIVERY_DATE=\"unknown\"


all: true_all

# Basic build rules and external variables
include ctrlsw_version.mk
include codec_defs.mk
-include compiler.mk

# Libraries
-include lib_fpga/project.mk
-include lib_common/project.mk
-include lib_rtos/project.mk
-include lib_ip_ctrl/project.mk
-include lib_log/project.mk

BUILD_LIB_A2P=0

ifneq ($(BUILD_LIB_A2P), 0)
  -include lib_a2p/project.mk
endif


-include lib_app/project.mk #lib_common, lib_log and lib_fbc_standalone dependency



BUILD_LIB_BITSTREAM=0
ifneq ($(ENABLE_ENCODER),0)
  BUILD_LIB_BITSTREAM=1
endif
ifneq ($(BUILD_LIB_BITSTREAM),0)
  -include lib_bitstream/project.mk
endif

ifneq ($(ENABLE_ENCODER),0)
  -include lib_common_enc/project.mk
  -include lib_buf_mngt/project.mk
  -include lib_rate_ctrl/project.mk
  -include lib_scheduler_enc/project.mk
  -include lib_encode/project.mk
endif

BUILD_LIB_COM_DEC=0
ifneq ($(ENABLE_DECODER),0)
  BUILD_LIB_COM_DEC=1
endif
ifneq ($(BUILD_LIB_COM_DEC),0)
  -include lib_common_dec/project.mk
endif



ifneq ($(ENABLE_CLIENT_FLAG),0)
-include lib_ref_customer/project.mk
$(shell mkdir -p $(BIN))
BIN_REF = $(shell readlink --canonicalize --verbose $(BIN))
ref_target = $(LIB_REFENC_A) \
            $(LIB_REFENC_DLL) \
            $(LIB_REFDEC_A) \
            $(LIB_REFDEC_DLL) \
            $(LIB_REF_LCEVC_ENC_QUANT_A) \
            $(LIB_REF_LCEVC_ENC_QUANT_DLL) \
            $(LIB_REF_LCEVC_ENC_A) \
            $(LIB_REF_LCEVC_ENC_DLL) \
            $(LIB_REF_LCEVC_DEC_A) \
            $(LIB_REF_LCEVC_DEC_DLL) \
            $(LIB_REFALLOC_A) \
            $(LIB_REFALLOC_DLL) \
            $(LIB_REFFBC_A) \
            $(LIB_REFFBC_DLL) \
            $(LIB_REFPOSTPROC_A) \
            $(LIB_REFPIXELPROC_A) \
            $(LIB_REFPIXELPROC_DLL) \
            $(LIB_REFPOSTPROC_DLL) \
            $(LIB_REFALLOC_SRC) \
            $(LIB_REFFBC_SRC)
lib_ref_goals:= $(shell echo $(MAKECMDGOALS) | sed -e "s/ /\n/g" | grep lib_ref | xargs)
$(ref_target): .submake ;
.submake:
	$(MAKE) $(lib_ref_goals) -C lib_ref_customer \
	ENABLE_64BIT=$(ENABLE_64BIT) \
	CROSS_COMPILE=$(CROSS_COMPILE) \
  CFLAGS_BASE="$(CFLAGS)" \
  LDFLAGS_BASE="$(LDFLAGS)" \
	BIN=$(BIN_REF)
else
-include ref.mk
endif

ifneq ($(ENABLE_DECODER),0)
  -include lib_parsing/project.mk

  -include lib_scheduler_dec/project.mk
  -include lib_decode/project.mk
endif

ifneq ($(ENABLE_EXE_DECODER),0)
  # AL_Decoder
  -include exe_decoder/project.mk

endif

ifneq ($(ENABLE_EXE_DECODER),0)
endif



ifneq ($(ENABLE_EXE_ENCODER),0)
  # AL_Encoder
  -include exe_encoder/project.mk
endif











ifneq ($(ENABLE_EXE_ENCODER),0)
ifneq ($(ENABLE_SYNC_IP),0)
ifeq ($(findstring linux,$(TARGET)),linux)
  -include exe_sync_ip/project.mk
endif
endif
endif




include base.mk

INSTALL ?= install -c
PREFIX ?= /usr
HDR_INSTALL_OPT = -m 0644

INCLUDE_DIR := include
HEADER_DIRS_TMP := $(sort $(dir $(wildcard $(INCLUDE_DIR)/*/)))
HEADER_DIRS := $(HEADER_DIRS_TMP:$(INCLUDE_DIR)/%=%)
INSTALL_PATH ?= ${PREFIX}
INSTALL_HDR_PATH := ${INSTALL_PATH}/include

install: all install_headers
	mkdir -p ${INSTALL_PATH}
	find ${BIN} -maxdepth 1 -name '*.exe' -exec $(INSTALL) {} ${INSTALL_PATH} -m 755  ';'
	find ${BIN} -maxdepth 1 -name '*.a' -exec $(INSTALL) {} ${INSTALL_PATH} -m 644 ';'
	find ${BIN} -maxdepth 1 -type f -name '*.so*' -exec cp -d {} ${INSTALL_PATH} ';'
	find ${BIN} -maxdepth 1 -type f -name "*.so*" -exec $(INSTALL) {} ${INSTALL_PATH} -m 755 ';'
	find ${BIN} -maxdepth 1 -type l -name '*.so*' -exec cp -d {} ${INSTALL_PATH} ';'

install_headers:
	@echo $(HEADER_DIRS)
	for dirname in $(HEADER_DIRS); do \
		$(INSTALL) -d "$(INCLUDE_DIR)/$$dirname" "$(INSTALL_HDR_PATH)/$$dirname"; \
		$(INSTALL) $(HDR_INSTALL_OPT) "$(INCLUDE_DIR)/$$dirname"/*.h* "$(INSTALL_HDR_PATH)/$$dirname"; \
	done; \
	$(INSTALL) $(HDR_INSTALL_OPT) "$(INCLUDE_DIR)"/*.h* "$(INSTALL_HDR_PATH)/";
	install -Dm 0755 bin/AL_Encoder.exe ${INSTALL_PATH}/ctrlsw_encoder
	install -Dm 0755 bin/AL_Decoder.exe ${INSTALL_PATH}/ctrlsw_decoder



pack_includes:
	@echo $(PACK_INCLUDES)

pack_defines:
	@echo $(PACK_DEFINES)

coverage: true_all
coverage: CFLAGS+=--coverage
coverage: LDFLAGS+=-lgcov

true_all: $(TARGETS)

print_flags:
	@echo $(CFLAGS) $(INCLUDES)

.PHONY: true_all clean all install
