LIB_COMMON_DEC_SRC:=\
  lib_common_dec/DecBuffers.c\
  lib_common_dec/DecInfo.c\
  lib_common_dec/DecHardwareConfig.c\
  lib_common_dec/RbspParser.c\
  lib_common_dec/IpDecFourCC.c\
  lib_common_dec/StreamSettings.c

ifneq ($(ENABLE_DECODER),0)
  LIB_COMMON_DEC_SRC+=lib_common_dec/DecHwScalingList.c
ifneq ($(ENABLE_HIGH_DYNAMIC_RANGE),0)
  LIB_COMMON_DEC_SRC+=lib_common_dec/HDRMeta.c
endif
endif

