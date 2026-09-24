##############################################################
#general rules
##############################################################
.SUFFIXES:

V?=0

ifneq ($(V),0)
	Q=
else
	Q=@
endif

LINK_COMPAT:=

INTROSPECT_FLAGS=

ifneq ($(ENABLE_STATIC),0)
	LINK_COMPAT+=-static-libstdc++ -static-libgcc -static
endif

ifneq ($(findstring mingw,$(TARGET)),mingw)
ifneq ($(findstring darwin,$(TARGET)),darwin)
	LINK_COMPAT+=-Wl,--hash-style=both
endif
endif

define filter_out_dyn_lib
	$(filter %.o %.a, $^)
endef

define add_dyn_lib_link
	$(foreach dyn_lib,$(filter %.so, $^),-L$(BIN) -l$(basename $(subst $(BIN)/lib,,$(dyn_lib))))
endef

$(BIN)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CFLAGS) $(INTROSPECT_FLAGS) $(INCLUDES) -std=gnu++11 -fPIC -o $@ -c $<
	@$(CXX) -MP -MM "$<" -MT "$@" -o "$(BIN)/$*_cpp.deps" $(INCLUDES) $(CFLAGS) -std=gnu++11 -fPIC
	@echo "CXX $<"

$(BIN)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -Wstrict-prototypes $(INTROSPECT_FLAGS) $(INCLUDES) -std=gnu11 -fPIC -o $@ -c $<
	@$(CC) -MP -MM "$<" -MT "$@" -o "$(BIN)/$*_c.deps" $(INCLUDES) $(CFLAGS) -std=gnu11 -fPIC
	@echo "CC $<"


$(BIN)/%.c.o: $(GENERATED_FILES_DIR)/%.c
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -Wstrict-prototypes $(INTROSPECT_FLAGS) $(INCLUDES) -std=gnu11 -fPIC -o $@ -c $<
	@$(CC) -MP -MM "$<" -MT "$@" -o "$(BIN)/$*_c.deps" $(INCLUDES) $(CFLAGS) -std=gnu11 -fPIC
	@echo "CC $<"

$(BIN)/%.a:
	@mkdir -p $(dir $@)
	$(Q)$(AR) cr $@ $^
	@echo "AR $@"
	$(Q)$(RANLIB) $@
	@echo "RANLIB $@"

$(BIN)/%.so:
	$(Q)$(CXX) $(CFLAGS) -shared -Wl,-soname,$(notdir $@).$(MAJOR) -o "$@.$(VERSION)" $(call filter_out_dyn_lib, $^) $(LDFLAGS) $(call add_dyn_lib_link, $^)
	@echo "LD $@"
	@ln -fs "$(@:$(BIN)/%=%).$(VERSION)" $@.$(MAJOR)
	@ln -fs "$(@:$(BIN)/%=%).$(VERSION)" $@

$(BIN)/include/config.h: include/config.h
	@mkdir -p $(dir $@)
	@cp $< $(dir $@)

$(BIN)/%.exe: $(BIN)/include/config.h
	@mkdir -p $(dir $@)
	$(Q)$(CXX) -o $@ $(call filter_out_dyn_lib, $^) $(LINK_COMPAT) $(LDFLAGS) $(call add_dyn_lib_link, $^)
	@echo "CXX $@"

clean:
ifeq ($(ENABLE_CLIENT_FLAG),1)
	$(MAKE) clean -C lib_ref_customer
endif
	$(Q)rm -rf $(BIN) $(GENERATED_FILES)
	@echo "CLEAN $(BIN) $(GENERATED_FILES)"

# Dependency generation

include $(shell test -d $(BIN) && find $(BIN) -name "*.deps")
