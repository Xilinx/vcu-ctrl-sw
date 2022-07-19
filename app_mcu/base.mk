##############################################################
#general rules
##############################################################
.SUFFIXES:

BIN?=bin
V?=0

ifeq ($(V),0)
	Q=@
else
	Q=
endif

$(BIN)/%.S.o: %.S
	@mkdir -p $(dir $@)
	@echo "CC $<"
	$(Q)$(CC) $(CFLAGS) -c $(INCLUDES) -o "$@" "$<"
	@$(CC) $(CFLAGS) -c $(INCLUDES) -o "$@.dep" "$<" -MM -MT "$@"

$(BIN)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "CXX $<"
	$(Q)$(CXX) $< $(CFLAGS) $(INCLUDES) -std=gnu++11 -o $@ -c
	@$(CC) $(CFLAGS) -c $(INCLUDES) -o "$@.dep" "$<" -MM -MT "$@"

$(BIN)/%.c.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	$(Q)$(CC) $< $(CFLAGS) $(INCLUDES) -std=gnu11 -o $@ -c
	@$(CC) $(CFLAGS) -c $(INCLUDES) -o "$@.dep" "$<" -MM -MT "$@"

$(BIN)/%.elf:
	@mkdir -p $(dir $@)
	@echo "CXX $@"
	$(Q)$(CXX) $^ -o $@ $(INCLUDES) $(LDFLAGS)
	@cp "$@" "$(BIN)/$*"

clean:
	@echo "CLEAN $(ROOT_BIN)"
	$(Q)rm -rf $(ROOT_BIN)

include $(shell test -d $(BIN) && find $(BIN) -name "*.dep")

