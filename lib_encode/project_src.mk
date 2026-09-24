LIB_ENCODE_SRC+=\
	lib_encode/Common_Encoder.c\
	lib_encode/NalWriters.c\
	lib_encode/EncUtils.c\
	lib_encode/IP_Stream.c\
	lib_encode/LibEncoderHost.c\
	lib_encode/lib_encode.c\
	lib_encode/SourceBufferChecker.c\
	lib_encode/LoadLda.c\
	lib_encode/ITU_Section.c\
	lib_encode/I_EncScheduler.c\
	lib_encode/EncSchedulerCommon.c


ifneq ($(ENABLE_ENC_AVC),0)
	LIB_ENCODE_SRC+=lib_encode/AVC_EncUtils.c
	LIB_ENCODE_SRC+=lib_encode/AVC_Encoder.c
	LIB_ENCODE_SRC+=lib_encode/AVC_Sections.c
	LIB_ENCODE_SRC+=lib_encode/AVC_EncHwScalingList.c
endif

ifneq ($(ENABLE_ENC_HEVC),0)
	LIB_ENCODE_SRC+=lib_encode/HEVC_EncUtils.c
	LIB_ENCODE_SRC+=lib_encode/HEVC_Encoder.c
	LIB_ENCODE_SRC+=lib_encode/HEVC_Sections.c
	LIB_ENCODE_SRC+=lib_encode/HEVC_EncHwScalingList.c
endif







ifneq ($(ENABLE_MICROBLAZE),0)
	LIB_ENCODE_SRC+=lib_encode/EncDriverDataConversions.c
	LIB_ENCODE_SRC+=lib_encode/EncSchedulerMcu.c
endif
