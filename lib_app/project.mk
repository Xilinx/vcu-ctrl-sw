LIB_APP_A=$(BIN)/liballegro_app.a
LIB_APP_DLL=$(BIN)/liballegro_app.so

LIB_APP_SRC:=\
  lib_app/utils.cpp\
  lib_app/convert.cpp\
  lib_app/BufPool.cpp\
  lib_app/PixMapBufPool.cpp\
  lib_app/AllocatorTracker.cpp\
  lib_app/YuvIO.cpp\
  lib_app/JsonFile.cpp\
  lib_app/FileUtils.cpp\
  lib_app/MD5.cpp\
  lib_app/Tokenizer.cpp\
  lib_app/FrameReader.cpp\
  lib_app/UnCompFrameReader.cpp\
  lib_app/UnCompFrameWriter.cpp\
  lib_app/CommonCmdParser.cpp\
  lib_app/CompFrameCommon.cpp\
  lib_app/BaseFrameWriter.cpp\
	lib_app/SinkFrame.cpp\
	lib_app/SinkYuvMd5.cpp\
	lib_app/SinkCrop.cpp\
  lib_app/SinkCrcDump.cpp\
  lib_app/SinkStreamMd5.cpp\
  lib_app/PlaneUtils.cpp\
  lib_app/RasterInputLoader.cpp\
  lib_app/AL_RasterConvert.cpp\
  lib_app/WrapLogger.cpp\
  $(LIB_LOG_SRC)\
  $(LIB_COMMON_SRC)\
  $(LIB_RTOS_SRC)

NEED_PARSER=0
ifneq ($(ENABLE_EXE_ENCODER),0)
  NEED_PARSER = 1
endif

ifneq ($(NEED_PARSER),0)
  LIB_APP_SRC+=lib_app/Parser.cpp
endif








ENABLE_TILE_INPUT_LOADER=0
ifneq ($(ENABLE_EXE_SITE),0)
  ENABLE_TILE_INPUT_LOADER=1
endif

ifneq ($(ENABLE_TILE_INPUT_LOADER),0)
  LIB_APP_SRC+=lib_app/TileInputLoader.cpp
endif



NEED_PIXEL_BUS_FEEDER=0

ifneq ($(NEED_PIXEL_BUS_FEEDER),0)
  LIB_APP_SRC+=lib_app/PixelBusSourceFeeder.cpp
endif

ifeq ($(findstring mingw,$(TARGET)),mingw)
  LIB_APP_SRC+=lib_app/plateform_windows.cpp
else
  LIB_APP_SRC+=lib_app/console_linux.cpp
  LIB_APP_SRC+=lib_app/plateform_linux.cpp
endif

LIB_APP_OBJ:=$(LIB_APP_SRC:%=$(BIN)/%.o)

$(LIB_APP_A): $(LIB_APP_OBJ)

$(LIB_APP_DLL): $(LIB_APP_OBJ)

liballegro_app: liballegro_app_dll liballegro_app_a

liballegro_app_dll: $(LIB_APP_DLL)

liballegro_app_a: $(LIB_APP_A)

TARGETS+=liballegro_app_dll
CFLAGS+=-Wno-date-time


liballegro_app_src: $(LIB_APP_SRC)
	@echo $(LIB_APP_SRC)

.PHONY: liballegro_app liballegro_app_dll liballegro_app_a liballegro_app_src
