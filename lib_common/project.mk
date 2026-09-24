LIB_COMMON_SRC:=\
  lib_common/Utils.c\
  lib_common/BufCommon.c\
  lib_common/AllocatorDefault.c\
  lib_common/AlignedAllocator.c\
  lib_common/MemDesc.c\
  lib_common/BufferAPI.c\
  lib_common/BufferPixMapMeta.c\
  lib_common/BufferHandleMeta.c\
  lib_common/Fifo.c\
  lib_common/FourCC.c\
  lib_common/PixMapBuffer.c\
  lib_common/IntVector.c\
  lib_common/Planes.c\
  lib_common/Error.c\
  lib_common/DisplayInfoMeta.c\
  lib_common/PicFormat.c\
  lib_common/StaticFifo.c\
  lib_common/WorkPool.c\
  lib_common/IntFifo.c\
  lib_common/Profiles.c\
  lib_common/AxiPropertiesInternal.c

HAS_COLOR_SPACE_CONVERSION=0





ifneq ($(ENABLE_AVC),0)
  LIB_COMMON_SRC+=lib_common/AvcLevelsLimit.c
  LIB_COMMON_SRC+=lib_common/AvcLevels.c
  LIB_COMMON_SRC+=lib_common/AvcUtils.c
endif

ifneq ($(ENABLE_HEVC),0)
  LIB_COMMON_SRC+=lib_common/HevcLevelsLimit.c
  LIB_COMMON_SRC+=lib_common/HevcLevels.c
  LIB_COMMON_SRC+=lib_common/HevcUtils.c
endif


HAS_HEVC_MULTI_LAYER=0







ifneq ($(ENABLE_CODEC),0)
  LIB_COMMON_SRC+=lib_common/LevelLimit.c
  LIB_COMMON_SRC+=lib_common/StreamBuffer.c
  LIB_COMMON_SRC+=lib_common/ChannelResources.c
  LIB_COMMON_SRC+=lib_common/SyntaxConversion.c
  LIB_COMMON_SRC+=lib_common/BufferCircMeta.c
  LIB_COMMON_SRC+=lib_common/BufferStreamMeta.c
  LIB_COMMON_SRC+=lib_common/BufferPictureMeta.c
  LIB_COMMON_SRC+=lib_common/BufferPictureDecMeta.c
  LIB_COMMON_SRC+=lib_common/BufferSeiMeta.c



ifneq ($(ENABLE_ENC_SW_MULTIPASS),0)
  LIB_COMMON_SRC+=lib_common/BufferLookAheadMeta.c
endif

ifneq ($(ENABLE_SW_HIGH_DYNAMIC_RANGE),0)
  LIB_COMMON_SRC+=lib_common/HDR.c
endif
endif



BUILD_IP_STATE=0

ifneq ($(BUILD_IP_STATE),0)
  LIB_COMMON_SRC+=lib_common/IpState.c
endif
