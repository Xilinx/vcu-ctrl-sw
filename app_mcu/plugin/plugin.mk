include $(PLUGIN)/plugin_src.mk

$(BIN)/plugin.elf: $(PLUGIN_FIRMWARE_SRC:%=$(BIN)/%.o)

# We generate the plugin by taking the .text and the .text_plugin sections of the code
# The .text corresponds to the plugin code.
# The .text_plugin is at a fixed address. It is the entry point and contains only one function
# used to create the plugin and give more information
$(BIN)/plugin.fw: $(BIN)/plugin.elf
	$(Q) $(OBJCOPY) --only-section .text_plugin --only-section .text -O binary "$<" "$@"

# The plugin data will be placed in the SRAM
# See the plugin.ld link script for more information about the data placement
$(BIN)/plugin_data.fw: $(BIN)/plugin.elf
	$(Q) $(OBJCOPY) --only-section .rodata --only-section .data -O binary "$<" "$@"

# Final assembly. These scripts do a rough "linking" process.
# They merge the plugin.fw and the plugin_data.fw with the al5e_b.fw and al5e.fw
# They put everything at the correct position in the binary so that the offset in the firmware
# correspond to what is written in the link file.
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
