LIB_MVD_SRC+=\
	  lib_mvd/AL_Mvd.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/BSDMA/mvd_bsdma.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/BSDMA/mvd_bsdma_cq.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/BSDMA/mvd_streambuf_queue.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/CommandQueue/mvd_supercq_control.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/CommandQueue/mvd_sync.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibBaseDecIf.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibCallback.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibClock.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibDbg.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibDiag.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibHWControl.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibHWFeature.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibHWIsr.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibIsrThread.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibStreamControl.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control/DecLibUData.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/CoreIF/mvd_hif_control.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/CoreIF/mvd_sif_control.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/DBE/mvd_dbe.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/DFE/mvd_dfe.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Pixif/mvd_pixif.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/PlayerIF/DecoderLib.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/PlayerIF/DecoderLibPrivate.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/PreParser/mvd_entropy_decode.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/PreParser/mvd_preparser_api.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Resampler/mvd_resampler.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Scheduler/DecLibScheduler.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/SlabCache/mvd_slab_alloc.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Support/mvd_funcs.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Support/mvd_system_info.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Support/mvd_tag.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Support/mvd_timers.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_api/media_player_api.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_control/media_player_control.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_control/media_player_qmeter.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_control/media_player_str_control.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_control/media_player_thread.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_control/media_player_timer.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_control/media_player_ud_control.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_fs_control/media_player_fs_control_external.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_fs_control/media_player_fs_control_internal.c\
    lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_scheduler/media_player_scheduler.c\
    lib_mvd/fw_data/PAL/common/pal_clocks.c\
    lib_mvd/fw_data/PAL/lib_rtos/pal.c



LIB_MVD_INC:=\
	-I ./lib_mvd\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/BaseDecoders/inc\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/BaseDecoders/av1\
		-I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/BaseDecoders/vp9\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/BSDMA\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/CommandQueue\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Control\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/CoreIF\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/DBE\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/DecPixelVerifier\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/DFE\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Pixif\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Incl\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/PlayerIF\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/PreParser\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Resampler\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Scheduler\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/SlabCache\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/DecLib/Support\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/include\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_api\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_control\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_fs_control\
    -I ./lib_mvd/fw_data/FWTip/Modules/Decoder/Core/Players/Media_Player/player_scheduler\
    -I ./lib_mvd/fw_data/Incl\
		-I ./lib_mvd/fw_data/Incl/API/MediaSys\
    -I ./lib_mvd/fw_data/Incl/App\
    -I ./lib_mvd/fw_data/Incl/Platform\
    -I ./lib_mvd/fw_data/Incl/Platform/GenTB\
    -I ./lib_mvd/fw_data/PAL/Incl\
    -I ./lib_mvd/fw_data/PAL/common\

LIB_MVD_CFLAGS:=-include config_mvd.h

PACK_INCLUDES+=$(LIB_MVD_INC)
PACK_DEFINES+=$(LIB_MVD_DEFINES)
