LIB_COMMON_SRC:=\
  lib_common/Utils.c\
  lib_common/BufCommon.c\
  lib_common/AllocatorDefault.c\
  lib_common/AlignedAllocator.c\
  lib_common/ChannelResources.c\
  lib_common/MemDesc.c\
  lib_common/HwScalingList.c\
  lib_common/BufferAPI.c\
  lib_common/BufferPixMapMeta.c\
  lib_common/BufferCircMeta.c\
  lib_common/BufferStreamMeta.c\
  lib_common/BufferPictureMeta.c\
  lib_common/BufferLookAheadMeta.c\
  lib_common/BufferHandleMeta.c\
  lib_common/BufferSeiMeta.c\
  lib_common/Fifo.c\
  lib_common/LevelLimit.c\
  lib_common/StreamBuffer.c\
  lib_common/FourCC.c\
  lib_common/HardwareDriver.c\
  lib_common/HDR.c\
  lib_common/PixMapBuffer.c\
  lib_common/SyntaxConversion.c

ifneq ($(ENABLE_AVC),0)
  LIB_COMMON_SRC+=lib_common/AvcLevelsLimit.c
  LIB_COMMON_SRC+=lib_common/AvcUtils.c
endif

ifneq ($(ENABLE_HEVC),0)
  LIB_COMMON_SRC+=lib_common/HevcLevelsLimit.c
  LIB_COMMON_SRC+=lib_common/HevcUtils.c
endif



