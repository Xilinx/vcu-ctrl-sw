##############################################################
#general rules
##############################################################
.SUFFIXES:

V?=0

ifeq ($(V),0)
	Q=@
else
	Q=
endif

LINK_COMPAT:=

INTROSPECT_FLAGS=

ifneq ($(ENABLE_STATIC),0)
	LINK_COMPAT+=-static-libstdc++ -static-libgcc -static
endif

ifneq ($(findstring mingw,$(TARGET)),mingw)
    LINK_COMPAT+=-Wl,--hash-style=both
endif

$(BIN)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CFLAGS) $(INTROSPECT_FLAGS) $(INCLUDES) -std=c++11 -o $@ -c $<
	@$(CXX) -MP -MM "$<" -MT "$@" -o "$(BIN)/$*_cpp.deps" $(INCLUDES) $(CFLAGS) -std=c++11
	@echo "CXX $<"

$(BIN)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) $(INTROSPECT_FLAGS) $(INCLUDES) -std=gnu99 -o $@ -c $<
	@$(CC) -MP -MM "$<" -MT "$@" -o "$(BIN)/$*_c.deps" $(INCLUDES) $(CFLAGS)
	@echo "CC $<"

$(BIN)/%.a:
	@mkdir -p $(dir $@)
	$(Q)$(AR) cr $@ $^
	@echo "AR $@"

$(BIN)/%:
	@mkdir -p $(dir $@)
	$(Q)$(CXX) -o $@ $^ $(LINK_COMPAT) $(LDFLAGS)
	@echo "CXX $@"

$(BIN)/%.so:
	$(Q)$(CXX) $(CFLAGS) -shared -Wl,-soname,$(notdir $@).$(MAJOR) -o "$@.$(VERSION)" $^ $(LDFLAGS)
	@echo "LD $@"
	@ln -fs "$(@:$(BIN)/%=%).$(VERSION)" $@.$(MAJOR)
	@ln -fs "$(@:$(BIN)/%=%).$(VERSION)" $@

clean:
	$(Q)rm -rf $(BIN)
	@echo "CLEAN $(BIN)"


define get-my-dir
$(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
endef

# Dependency generation

include $(shell test -d $(BIN) && find $(BIN) -name "*.deps")
