LIB_PARSING_SRC:=\
	lib_parsing/common_syntax.c\
	lib_parsing/VpsParsing.c\
	lib_parsing/SpsParsing.c\
	lib_parsing/PpsParsing.c\
	lib_parsing/SeiParsing.c\
	lib_parsing/SliceHdrParsing.c\
	lib_parsing/DPB.c\
	lib_parsing/I_PictMngr.c\
	lib_parsing/Avc_PictMngr.c\
	lib_parsing/Hevc_PictMngr.c\
	lib_parsing/Concealment.c\

UNITTEST+=$(LIB_PARSING_SRC)
