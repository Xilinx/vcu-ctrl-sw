LIB_PARSING_SRC:=\
	lib_parsing/common_syntax.c\
	lib_parsing/DPB.c\
	lib_parsing/I_PictMngr.c\
	lib_parsing/Concealment.c\

ifneq ($(ENABLE_DEC_ITU),0)
	LIB_PARSING_SRC+=lib_parsing/SeiParser.c
endif

ifneq ($(ENABLE_DEC_AVC),0)
	LIB_PARSING_SRC+=lib_parsing/AvcParser.c
	LIB_PARSING_SRC+=lib_parsing/Avc_PictMngr.c
	LIB_PARSING_SRC+=lib_parsing/Avc_SliceHeaderParsing.c
endif

ifneq ($(ENABLE_DEC_HEVC),0)
	LIB_PARSING_SRC+=lib_parsing/HevcParser.c
	LIB_PARSING_SRC+=lib_parsing/Hevc_PictMngr.c
	LIB_PARSING_SRC+=lib_parsing/Hevc_SliceHeaderParsing.c
endif

