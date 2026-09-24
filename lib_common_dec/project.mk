LIB_COMMON_DEC_SRC:=\
  lib_common_dec/DecBuffers.c\
  lib_common_dec/DecInfo.c\
  lib_common_dec/DecHardwareConfig.c\
  lib_common_dec/RbspParser.c\
  lib_common_dec/IpDecFourCC.c\
  lib_common_dec/StreamSettings.c\
  lib_common_dec/DecOutputSettings.c\
  lib_common_dec/DecHwScalingList.c\
	lib_common_dec/BufPool.c\


# TODO: add `ifeq ($(findstring linux,$(TARGET)),linux)` when implementation is fully split
LIB_COMMON_DEC_SRC+=lib_common_dec/DecLinuxDriverCommunication.c

ifneq ($(ENABLE_DEC_SW_HIGH_DYNAMIC_RANGE),0)
  LIB_COMMON_DEC_SRC+=lib_common_dec/HDRMeta.c
endif
