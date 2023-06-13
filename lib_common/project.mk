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
  lib_common/HardwareDriver.c\
  lib_common/PixMapBuffer.c\
  lib_common/IntVector.c\
  lib_common/Planes.c\
  lib_common/HardwareConfig.c\
  lib_common/Error.c\
  lib_common/DisplayInfoMeta.c\


HAS_CODEC=0
ifneq ($(ENABLE_AVC),0)
  LIB_COMMON_SRC+=lib_common/AvcLevelsLimit.c
  LIB_COMMON_SRC+=lib_common/AvcUtils.c
  HAS_CODEC=1
endif

ifneq ($(ENABLE_HEVC),0)
  LIB_COMMON_SRC+=lib_common/HevcLevelsLimit.c
  LIB_COMMON_SRC+=lib_common/HevcUtils.c
  HAS_CODEC=1
endif






ifneq ($(HAS_CODEC),0)
  LIB_COMMON_SRC+=lib_common/LevelLimit.c
  LIB_COMMON_SRC+=lib_common/StreamBuffer.c
  LIB_COMMON_SRC+=lib_common/ChannelResources.c
  LIB_COMMON_SRC+=lib_common/HwScalingList.c
  LIB_COMMON_SRC+=lib_common/SyntaxConversion.c
  LIB_COMMON_SRC+=lib_common/BufferCircMeta.c
  LIB_COMMON_SRC+=lib_common/BufferStreamMeta.c
  LIB_COMMON_SRC+=lib_common/BufferPictureMeta.c
  LIB_COMMON_SRC+=lib_common/BufferSeiMeta.c
  LIB_COMMON_SRC+=lib_common/BufferStatisticsMeta.c


  LIB_COMMON_SRC+=lib_common/BufferLookAheadMeta.c

ifneq ($(ENABLE_HIGH_DYNAMIC_RANGE),0)
  LIB_COMMON_SRC+=lib_common/HDR.c
endif
endif


