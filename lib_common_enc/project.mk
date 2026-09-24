LIB_COMMON_ENC_GENERATED_SRC:=

LIB_COMMON_ENC_SRC:=\
	lib_common_enc/EncBuffers.c\
	lib_common_enc/IpEncFourCC.c\
	lib_common_enc/EncSize.c\
	lib_common_enc/Settings.c\
	lib_common_enc/DPBConstraints.c\
	lib_common_enc/ParamConstraints.c\
	lib_common_enc/RateCtrlMeta.c\
	lib_common_enc/QPTable.c\
	lib_common_enc/QpTableMeta.c\
	lib_common_enc/EncHardwareConfig.c\


# TODO: add `ifeq ($(findstring linux,$(TARGET)),linux)` when implementation is fully split
LIB_COMMON_ENC_SRC+=lib_common_enc/EncLinuxDriverCommunication.c



ifneq ($(ENABLE_ENC_ITU),0)
  LIB_COMMON_ENC_SRC+=lib_common_enc/Itu_Utils.c
endif
