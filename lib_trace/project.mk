LIB_TRACE_SRC:=\
  lib_trace/CommonTraces.c\

LIB_TRACE_SRC_ENC:=\
  $(LIB_TRACE_SRC)\
  lib_trace/EncTraces.c\

LIB_TRACE_SRC_DEC:=\
  $(LIB_TRACE_SRC)\
  lib_trace/DecTraces.c\

LIB_TRACE_SRC_FBC:=\
  $(LIB_TRACE_SRC)\
  lib_trace/FbcTraces.c\

$(BIN)/lib_trace/DecTraces.c.o: CFLAGS+=-Wno-format-overflow
$(BIN)/lib_trace/DecTraces.c.o: CFLAGS+=-Wno-format-truncation
