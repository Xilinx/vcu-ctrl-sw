LIB_DECODE_SRC+=\
  lib_decode/NalUnitParser.c\
  lib_decode/NalDecoder.c\
  lib_decode/FrameParam.c\
  lib_decode/SliceDataParsing.c\
  lib_decode/DefaultDecoder.c\
  lib_decode/LibDecodeHost.c\
  lib_decode/lib_decode.c\
  lib_decode/UnsplitBufferFeeder.c\
  lib_decode/Patchworker.c\
  lib_decode/DecoderFeeder.c\
  lib_decode/SplitBufferFeeder.c\
  lib_decode/I_DecScheduler.c\
  lib_decode/WorkPool.c\
  lib_decode/DecSettings.c



ifneq ($(ENABLE_MCU),0)
  LIB_DECODE_SRC+=lib_decode/DecSchedulerMcu.c
else
  # the proxy mode also uses the mcu.
endif

ifneq ($(ENABLE_AVC),0)
  LIB_DECODE_SRC+=lib_decode/AvcDecoder.c
endif

ifneq ($(ENABLE_HEVC),0)
  LIB_DECODE_SRC+=lib_decode/HevcDecoder.c
endif
