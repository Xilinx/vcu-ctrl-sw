LIB_PARSING_SRC:=\
	lib_parsing/common_syntax.c\
	lib_parsing/SliceHdrParsing.c\
	lib_parsing/DPB.c\
	lib_parsing/I_PictMngr.c\
	lib_parsing/Concealment.c\

ifneq ($(ENABLE_AVC),0)
	LIB_PARSING_SRC+=lib_parsing/AvcParser.c
	LIB_PARSING_SRC+=lib_parsing/Avc_PictMngr.c
endif

ifneq ($(ENABLE_HEVC),0)
	LIB_PARSING_SRC+=lib_parsing/HevcParser.c
	LIB_PARSING_SRC+=lib_parsing/Hevc_PictMngr.c
endif

