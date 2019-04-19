include $(PLUGIN)/plugin_src.mk

$(BIN)/plugin.elf: $(PLUGIN_FIRMWARE_SRC:%=$(BIN)/%.o)

$(BIN)/plugin.fw: $(BIN)/plugin.elf
	$(Q) $(OBJCOPY) --only-section .text_plugin --only-section .text -O binary "$<" "$@"

$(BIN)/plugin_data.fw: $(BIN)/plugin.elf
	$(Q) $(OBJCOPY) --only-section .rodata --only-section .data -O binary "$<" "$@"

$(BIN)/plugged_al5e.fw: $(AL5E) $(BIN)/plugin.fw
	$(Q) $(PLUGIN_SCRIPT_DIR)/text_plugin_gen.sh $^ $@

$(BIN)/plugged_al5e_b.fw: $(AL5E_B) $(BIN)/plugin_data.fw
	$(Q) $(PLUGIN_SCRIPT_DIR)/data_plugin_gen.sh $^ $@

$(BIN)/%.elf:
	@echo "LD $@"
	$(Q)$(CC) $(CFLAGS) -o "$@" $(LINK_SCRIPT) $^ $(LIBS) $(LDFLAGS)
	@echo "SIZE $@"
	$(Q)$(SIZE) $@

TARGETS+=$(BIN)/plugged_al5e.fw
TARGETS+=$(BIN)/plugged_al5e_b.fw
