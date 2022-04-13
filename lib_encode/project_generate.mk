GENERATOR=$(BIN)/riscv_generator.exe
INCLUDES_GEN_ENCODE=-Ilib_encode -Iinclude/lib_encode -Iinclude -I.

GENERATED_FILES+= \
	lib_encode/msg_itf_generated.c \
	lib_encode/msg_itf_generated.h

lib_encode/LibEncoderRiscv.c: lib_encode/msg_itf_generated.h

lib_encode/msg_itf_generated.c: $(BIN)/lib_encode/msg_itf.c lib_encode/msg_itf_generated.h $(GENERATOR)
	@echo "GEN $@"
	$(Q)$(GENERATOR) -penc_ $< $(INCLUDES_GEN_ENCODE) > $@

lib_encode/msg_itf_generated.h: $(BIN)/lib_encode/msg_itf.c $(GENERATOR)
	@echo "GEN $@"
	$(Q)$(GENERATOR) -h -penc_ $< $(INCLUDES_GEN_ENCODE) > $@

$(BIN)/lib_encode/msg_itf.c:
	@mkdir -p $(dir $@)
	@echo "GEN $@"
	@echo "#include <stdint.h>\n#include \"config.h\"\n#include \"lib_encode/msg_itf.h\"\n" > $@
	@$(CC) -MP -MM $@ -MT "$@" -o "$(BIN)/lib_encode/msg_itf_c.deps" $(INCLUDES_GEN_ENCODE)
	@sed -i 's| $(BIN)/lib_encode/msg_itf.c ||' $(BIN)/lib_encode/msg_itf_c.deps
