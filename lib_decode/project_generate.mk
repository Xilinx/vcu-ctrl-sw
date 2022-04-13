GENERATOR=$(BIN)/riscv_generator.exe
INCLUDES_GEN_DECODE=-Ilib_decode -Iinclude/lib_decode -Iinclude -I.

GENERATED_FILES+= \
	lib_decode/msg_itf_generated.c \
	lib_decode/msg_itf_generated.h

lib_decode/LibDecodeRiscv.c: lib_decode/msg_itf_generated.h

lib_decode/msg_itf_generated.c: $(BIN)/lib_decode/msg_itf.c lib_decode/msg_itf_generated.h $(GENERATOR)
	@echo "GEN $@"
	$(Q)$(GENERATOR) $< $(INCLUDES_GEN_DECODE) > $@

lib_decode/msg_itf_generated.h: $(BIN)/lib_decode/msg_itf.c $(GENERATOR)
	@echo "GEN $@"
	$(Q)$(GENERATOR) -h $< $(INCLUDES_GEN_DECODE) > $@

$(BIN)/lib_decode/msg_itf.c:
	@mkdir -p $(dir $@)
	@echo "GEN $@"
	@echo "#include <stdint.h>\n#include \"config.h\"\n#include \"lib_decode/msg_itf.h\"\n" > $@
	@$(CC) -MP -MM $@ -MT "$@" -o "$(BIN)/lib_decode/msg_itf_c.deps" $(INCLUDES_GEN_DECODE)
	@sed -i 's| $(BIN)/lib_decode/msg_itf.c ||' $(BIN)/lib_decode/msg_itf_c.deps
