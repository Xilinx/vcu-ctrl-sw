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

ifneq ($(ENABLE_STATIC),0)
	LINK_COMPAT+=-static-libstdc++ -static-libgcc -static
endif

ifneq ($(findstring mingw,$(TARGET)),mingw)
    LINK_COMPAT+=-Wl,--hash-style=both
endif

$(BIN)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CFLAGS) $(INCLUDES) -std=c++11 -o $@ -c $<
	@$(CXX) -MM "$<" -MT "$@" -o "$(BIN)/$*_cpp.deps" $(INCLUDES) $(CFLAGS) -std=c++11
	@echo "CXX $<"

$(BIN)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) -std=gnu99 -o $@ -c $<
	@$(CC) -MM "$<" -MT "$@" -o "$(BIN)/$*_c.deps" $(INCLUDES) $(CFLAGS)
	@echo "CC $<"

$(BIN)/%.a:
	@mkdir -p $(dir $@)
	$(Q)$(AR) cr $@ $^
	@echo "AR $@"

$(BIN)/%.exe:
	@mkdir -p $(dir $@)
	$(Q)$(CXX) -o $@ $^ $(LINK_COMPAT) $(LDFLAGS)
	@echo "CXX $@"

$(BIN)/%.so:
	$(Q)$(CXX) $(CFLAGS) -shared -Wl,-soname,$(notdir $@).$(MAJOR) -o "$@.$(VERSION)" $^ $(LDFLAGS)
	@echo "LD $@"
	@rm -f $@
	@ln "$@.$(VERSION)" $@

clean:
	$(Q)rm -rf $(BIN)
	@echo "CLEAN $(BIN)"


define get-my-dir
$(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
endef

# Dependency generation

include $(shell test -d $(BIN) && find $(BIN) -name "*.deps")
