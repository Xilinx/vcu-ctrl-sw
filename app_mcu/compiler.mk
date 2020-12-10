# compiler quirks

COMPILER_NAME:=$(shell $(CC) --version | head -n 1 | cut -d ' ' -f 1)

ifeq ($(findstring gcc,$(COMPILER_NAME)),gcc)

GCC_MAJOR_VER:=$(shell $(CC) -dumpversion | sed 's/\..*//')

# Tested version are 4.9.2 and 9.2
GCC_IS_OLD:=$(shell test $(GCC_MAJOR_VER) -lt 8 && echo 1 || echo 0)

# gcc 4.9.2 has a bug with the --relax option and generate wrong jump table in some cases,
# resulting in switch statement jumping to the wrong address

CFLAGS+=-Os
ifeq ($(GCC_IS_OLD),1)
	LDFLAGS+=-Wl,--no-relax
else
	# The Os optim doesn't do aggressive inline anymore and this impacts runtime in the firmware
  # O3 flags one by one added to Os to get the optim we need while still optimising for size (Keeps the same behavior as 4.9.2)
	CFLAGS+=-finline-functions
	CFLAGS+=-funswitch-loops
	CFLAGS+=-fpredictive-commoning
	CFLAGS+=-fgcse-after-reload
	CFLAGS+=-ftree-loop-vectorize
	CFLAGS+=-ftree-slp-vectorize
	CFLAGS+=-fvect-cost-model
	CFLAGS+=-ftree-partial-pre
	CFLAGS+=-fipa-cp-clone
endif

endif
