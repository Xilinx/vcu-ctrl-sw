LIB_CFG_PARSING_A=$(BIN)/liballegro_cfg_parsing.a
LIB_CFG_PARSING_DLL=$(BIN)/liballegro_cfg_parsing.so

LIB_CFG_PARSING_SRC:=\
  lib_cfg_parsing/Parser.cpp\
  lib_cfg_parsing/Tokenizer.cpp\

LIB_CFG_PARSING_OBJ:=$(LIB_CFG_PARSING_SRC:%=$(BIN)/%.o)

$(LIB_CFG_PARSING_A): $(LIB_CFG_PARSING_OBJ)

$(LIB_CFG_PARSING_DLL): $(LIB_CFG_PARSING_OBJ)

liballegro_cfg_parsing: liballegro_cfg_parsing_dll liballegro_cfg_parsing_a

liballegro_cfg_parsing_dll: $(LIB_CFG_PARSING_DLL)

liballegro_cfg_parsing_a: $(LIB_CFG_PARSING_A)

TARGETS+=liballegro_cfg_parsing_dll

liballegro_cfg_parsing_src: $(LIB_CFG_PARSING_SRC)
	@echo $(LIB_CFG_PARSING_SRC)

.PHONY: liballegro_cfg_parsing liballegro_cfg_parsing_dll liballegro_cfg_parsing_a liballegro_cfg_parsing_src
