LIB_ENCODE_SRC+=\
	lib_encode/Com_Encoder.c\
	lib_encode/Sections.c\
	lib_encode/NalWriters.c\
	lib_encode/EncUtils.c\
	lib_encode/IP_Stream.c\
	lib_encode/LibEncoderHost.c\
	lib_encode/lib_encoder.c\
	lib_encode/SourceBufferChecker.c\
	lib_encode/LoadLda.c\


ifneq ($(ENABLE_AVC),0)
  LIB_ENCODE_SRC+=lib_encode/AVC_EncUtils.c
	LIB_ENCODE_SRC+=lib_encode/AVC_Encoder.c
	LIB_ENCODE_SRC+=lib_encode/AVC_Sections.c
endif

ifneq ($(ENABLE_HEVC),0)
  LIB_ENCODE_SRC+=lib_encode/HEVC_EncUtils.c
	LIB_ENCODE_SRC+=lib_encode/HEVC_Encoder.c
	LIB_ENCODE_SRC+=lib_encode/HEVC_Sections.c
endif






ISCHEDULER_SRC:=\
  lib_encode/I_EncScheduler.c\
  lib_encode/EncSchedulerCommon.c\
