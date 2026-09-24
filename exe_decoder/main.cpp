// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <climits>
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include "lib_common/BufCommon.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/BufferHandleMeta.h"
#include "lib_common/BufferSeiMeta.h"
#include "lib_common/DisplayInfoMeta.h"
#include "lib_common/Error.h"
#include "lib_common/Round.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_decode/lib_decode.h"
#include "lib_rtos/utils.h"
#include "lib_common_dec/HDRMeta.h"
#include "lib_common/BufferPictureDecMeta.h"
}
#include "lib_app/BufPool.hpp"
#include "lib_app/FileUtils.hpp"
#include "lib_app/MD5.hpp"
#include "lib_app/PixMapBufPool.hpp"
#include "lib_app/SinkFilter.hpp"
#include "lib_app/SinkCrop.hpp"
#include "lib_app/SinkCrcDump.hpp"
#include "lib_app/SinkFrame.hpp"
#include "lib_app/SinkYuvMd5.hpp"
#include "lib_app/SinkCrcDump.hpp"
#include "lib_app/SinkDecPerf.hpp"
#include "lib_app/UnCompFrameReader.hpp"
#include "lib_app/YuvIO.hpp"
#include "lib_app/console.hpp"
#include "lib_app/convert.hpp"
#include "lib_app/plateform.hpp"
#include "lib_app/timing.hpp"
#include "lib_app/utils.hpp"
#include <cassert>

#include "CmdParser.hpp"
#include "CodecUtils.hpp"
#include "InputLoader.hpp"
#include "HeifLoader.hpp"
#include "IpDevice.hpp"
#include "SinkYuvCrc.hpp"
#include "HDRWriter.hpp"

using namespace std;

/******************************************************************************/
enum DeviceType
{
  DEVICE_BASE_DECODER,
};

using Devices = map<DeviceType, shared_ptr<I_IpDevice>>;
using UseBoards = map<DeviceType, bool>;

bool bFindNextDevice = false;

struct codec_error : public runtime_error
{
  explicit codec_error(AL_ERR eErrCode) : runtime_error(AL_Codec_ErrorToString(eErrCode)), Code(eErrCode)
  {
  }

  const AL_ERR Code;
};

/******************************************************************************/
static void ConvertFrameBuffer(AL_TBuffer* pInput, AL_TBuffer*& pOutput, int32_t iBdOut, AL_TPosition const& tPos, TFourCC tOutFourCC)
{
  (void)tPos;
  TFourCC tRecFourCC = AL_PixMapBuffer_GetFourCC(pInput);
  AL_TDimension tRecDim = AL_PixMapBuffer_GetDimension(pInput);
  AL_EChromaMode eRecChromaMode = AL_GetChromaMode(tRecFourCC);

  TFourCC tConvFourCC = tOutFourCC;
  AL_TPicFormat tConvPicFormat;
  assert(tConvFourCC);

  if(pOutput != nullptr)
  {
    AL_TDimension tYuvDim = AL_PixMapBuffer_GetDimension(pOutput);

    AL_GetPicFormat(tConvFourCC, &tConvPicFormat);

    if(tRecDim.iWidth != tYuvDim.iWidth || tRecDim.iHeight != tYuvDim.iHeight ||
       eRecChromaMode != tConvPicFormat.eChromaMode || iBdOut != tConvPicFormat.uBitDepth)
    {
      AL_Buffer_Destroy(pOutput);
      pOutput = nullptr;
    }
  }

  AL_PixMapBuffer_SetDimension(pInput, { tPos.iX + tRecDim.iWidth, tPos.iY + tRecDim.iHeight });

  if(pOutput == nullptr)
  {
    AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pInput);

    pOutput = AllocateDefaultYuvIOBuffer(tDim, tConvFourCC);

    if(pOutput == nullptr)
      throw runtime_error("Couldn't allocate YuvBuffer");
  }

  if(ConvertPixMapBuffer(pInput, pOutput))
    throw runtime_error("Couldn't convert buffer");

  AL_PixMapBuffer_SetDimension(pInput, tRecDim);
  AL_PixMapBuffer_SetDimension(pOutput, tRecDim);
}

/******************************************************************************/
static bool IsEndOfStream(AL_TBuffer const* pFrame, AL_TInfoDecode const* pInfo)
{
  return !pFrame && !pInfo;
}

/******************************************************************************/
static bool IsReleaseFrame(AL_TBuffer const* pFrame, AL_TInfoDecode const* pInfo)
{
  return pFrame && !pInfo;
}

/******************************************************************************/
class DisplayManager
{
public:
  void Configure(Config const& config);
  void ConfigureMainOutputWriters(AL_TDecOutputSettings const& tDecOutputSettings);

  bool Process(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo, int32_t iBitDepthAlloc, bool& bIsMainDisplay, bool& bNumFrameReached, bool bDecoderExists);

private:
  void ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int32_t iBdOut, TFourCC tOutFourCC);

  void CopyMetaData(AL_TBuffer* pDstFrame, AL_TBuffer* pSrcFrame, AL_EMetaType eMetaType);

  unique_ptr<MultiSink> multisinkRaw = unique_ptr<MultiSink>(new MultiSink);
  unique_ptr<MultiSink> multisinkOut = unique_ptr<MultiSink>(new MultiSink);

  AL_EFbStorageMode eMainOutputStorageMode;
  bool bOutputWritersCreated = false;
  int32_t iBitDepth = 8;
  uint64_t uNumFrames = 0;
  uint64_t uMaxFrames = UINT64_MAX;
  uint64_t uFirstFrame = 0;
  TFourCC tOutputFourCC = FOURCC(NULL);
  TFourCC tInputFourCC = FOURCC(NULL);
  AL_TDimension m_tDisplayDimension = { 0, 0 };
  bool bMainOutputCompression = false;
  bool bHasOutput = false;
  bool bEnableYUVOutput = false;
  std::shared_ptr<HDRWriter> pHDRWriter;
  std::shared_ptr<ofstream> hFileOut;
  std::shared_ptr<ofstream> hMapOut;
};

/******************************************************************************/
void DisplayManager::Configure(Config const& config)
{
  if(config.tOutputFourCC != FOURCC(NULL))
    eMainOutputStorageMode = AL_GetStorageMode(config.tOutputFourCC);
  else
  {
    eMainOutputStorageMode = config.tDecSettings.eFBStorageMode;
    bMainOutputCompression = IsOutputStorageModeCompressed(config.tUserOutputSettings, config.tDecSettings.bFrameBufferCompression);

    if((eMainOutputStorageMode != AL_FB_RASTER) && !bMainOutputCompression)
      eMainOutputStorageMode = AL_FB_RASTER;
  }

  bool bMd5Output = !config.md5File.empty();

  bHasOutput = (config.bEnableYUVOutput || config.bCertCRC || bMd5Output);
  bEnableYUVOutput = config.bEnableYUVOutput;

  if(bHasOutput)
  {
    if(config.bEnableYUVOutput)
    {
      {
        hFileOut.reset(new ofstream(config.sMainOut, ios::binary));

        if(!hFileOut->is_open())
          throw runtime_error("Invalid output file");
      }

      if(bMainOutputCompression
         )
      {
        {
          hMapOut.reset(new ofstream(config.sMainOut + ".map", ios::binary));

          if(!hMapOut->is_open())
            throw runtime_error("Invalid output map file");
        }
      }
    }

    if(bMd5Output && !bMainOutputCompression)
    {
      std::unique_ptr<IFrameSink> md5Calculator(createYuvMd5Calculator(config.md5File));
      std::unique_ptr<IFrameSink> md5Sink(new SinkCrop(md5Calculator));
      multisinkOut->addSink(md5Sink);
    }

    if(config.bCertCRC)
    {
      const string sCertCrcFile = "crc_certif_res.hex";
      std::unique_ptr<IFrameSink> crcCSCalculator(createCSCrcCalculator(sCertCrcFile));
      std::unique_ptr<IFrameSink> crcSink(new SinkCrop(crcCSCalculator));
      multisinkOut->addSink(crcSink);
    }
  }

  iBitDepth = config.iOutputBitDepth;
  tOutputFourCC = config.tOutputFourCC;
  uMaxFrames = config.iMaxFrames;

  std::unique_ptr<IFrameSink> crcDump(createStreamCrcDump(config.sCrc));
  multisinkRaw->addSink(crcDump);

  if(!config.hdrFile.empty())
    pHDRWriter = shared_ptr<HDRWriter>(new HDRWriter(config.hdrFile));
}

void DisplayManager::ConfigureMainOutputWriters(AL_TDecOutputSettings const& tDecOutputSettings)
{
  (void)tDecOutputSettings;

  if(!bHasOutput || bOutputWritersCreated || !bEnableYUVOutput)
    return;

  AL_EOutputType eOutputType = AL_OUTPUT_MAIN;
  AL_EFbStorageMode eOutputStorageMode = eMainOutputStorageMode;

  {
    {
      std::unique_ptr<IFrameSink> frameSink(createUnCompFrameSink(hFileOut, eOutputStorageMode));
      std::unique_ptr<IFrameSink> uncompressedSink(new SinkFilter(eOutputType, frameSink));
      multisinkOut->addSink(uncompressedSink);
    }
  }

  bOutputWritersCreated = true;
}

/******************************************************************************/
static void sFreeWithoutDestroyingMemory(AL_TBuffer* buffer)
{
  buffer->iChunkCnt = 0;
  AL_Buffer_Destroy(buffer);
}

/******************************************************************************/
void DisplayManager::CopyMetaData(AL_TBuffer* pDstFrame, AL_TBuffer* pSrcFrame, AL_EMetaType eMetaType)
{
  AL_TMetaData* pMetaD = nullptr;

  AL_TMetaData* pOrigMeta = AL_Buffer_GetMetaData(pSrcFrame, eMetaType);

  if(!pOrigMeta)
    throw runtime_error("Metadata is NULL");
  switch(eMetaType)
  {
  case AL_META_TYPE_PIXMAP:
  {
    pMetaD = (AL_TMetaData*)AL_PixMapMetaData_Clone((AL_TPixMapMetaData*)pOrigMeta);
    break;
  }
  case AL_META_TYPE_DISPLAY_INFO:
  {
    pMetaD = (AL_TMetaData*)AL_DisplayInfoMetaData_Clone((AL_TDisplayInfoMetaData*)pOrigMeta);
    break;
  }
  default:
    throw std::runtime_error("Metadata type is not supported");
    break;
  }

  if(pMetaD == NULL)
    throw runtime_error("Clone of MetaData was not created!");

  if(!AL_Buffer_AddMetaData(pDstFrame, pMetaD))
    throw runtime_error("Cloned pMetaD did not get added!\n");
}

/******************************************************************************/
bool DisplayManager::Process(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo, int32_t iBitDepthAlloc, bool& bIsMainDisplay, bool& bNumFrameReached, bool bDecoderExists)
{
  bNumFrameReached = false;
  bIsMainDisplay = (pInfo->eOutputID == AL_OUTPUT_MAIN || pInfo->eOutputID == AL_OUTPUT_POSTPROC);

  if(bDecoderExists)
  {
    if(uFirstFrame <= uNumFrames && uNumFrames < uMaxFrames)
    {
      if(!AL_Buffer_GetData(pFrame))
        throw runtime_error("Data buffer is null");

      AL_TBuffer* pDisplayFrame = AL_Buffer_ShallowCopy(pFrame, &sFreeWithoutDestroyingMemory);

      auto scopepDisplayFrame = scopeExit([&]() {
        AL_Buffer_Unref(pDisplayFrame);
      });

      AL_Buffer_Ref(pDisplayFrame);
      CopyMetaData(pDisplayFrame, pFrame, AL_META_TYPE_PIXMAP);
      CopyMetaData(pDisplayFrame, pFrame, AL_META_TYPE_DISPLAY_INFO);

      int32_t iCurrentBitDepth = max(pInfo->uBitDepthY, pInfo->uBitDepthC);

      if(iBitDepth == OUTPUT_BD_FIRST)
        iBitDepth = iCurrentBitDepth;
      else if(iBitDepth == OUTPUT_BD_ALLOC)
        iBitDepth = iBitDepthAlloc;

      int32_t iEffectiveBitDepth = iBitDepth == OUTPUT_BD_STREAM ? iCurrentBitDepth : iBitDepth;

      multisinkRaw->ProcessFrame(pDisplayFrame);

      if(bHasOutput)
        ProcessFrame(*pDisplayFrame, *pInfo, iEffectiveBitDepth, tOutputFourCC);

      if(bIsMainDisplay)
      {
        AL_THDRMetaData* pOrigHDRMeta = (AL_THDRMetaData*)AL_Buffer_GetMetaData(pFrame, AL_META_TYPE_HDR);

        if(pOrigHDRMeta != NULL)
        {
          AL_THDRMetaData pHDRMeta;
          AL_HDRMetaData_Copy(pOrigHDRMeta, &pHDRMeta);

          if(pHDRWriter != nullptr)
            pHDRWriter->WriteHDRSEIs(pHDRMeta.eColourDescription, pHDRMeta.eTransferCharacteristics, pHDRMeta.eColourMatrixCoeffs, pHDRMeta.tHDRSEIs);
        }
        // TODO: increase only when last frame
        DisplayFrameStatus(uNumFrames);
      }
    }

    if(bIsMainDisplay)
      uNumFrames++;
  }

  if(uNumFrames >= uMaxFrames)
    bNumFrameReached = true;

  return bNumFrameReached;
}

/******************************************************************************/
static void PrintHexdump(ostream* logger, uint8_t* data, int32_t size)
{
  int32_t column = 0;
  int32_t toPrint = size;

  *logger << std::hex;

  while(toPrint > 0)
  {
    *logger << setfill('0') << setw(2) << (int)data[size - toPrint];
    --toPrint;
    ++column;

    if(toPrint > 0)
    {
      if(column % 8 == 0)
        *logger << endl;
      else
        *logger << " ";
    }
  }

  *logger << std::dec;
}

/******************************************************************************/
static void WriteSei(bool bIsPrefix, int32_t iPayloadType, uint8_t* pPayload, int32_t iPayloadSize, ostream* seiOut, int32_t iNumFrame)
{
  if(!seiOut)
    return;

  if(iNumFrame != SEI_NOT_ASSOCIATED_WITH_FRAME)
    *seiOut << "Num Frame: " << iNumFrame << endl;

  *seiOut << "is_prefix: " << boolalpha << bIsPrefix << endl
          << "sei_payload_type: " << iPayloadType << endl
          << "sei_payload_size: " << iPayloadSize << endl
          << "raw:" << endl;
  PrintHexdump(seiOut, pPayload, iPayloadSize);
  *seiOut << endl << endl;
}

/******************************************************************************/
static void WriteSyncSei(std::vector<AL_TSeiMetaData*> seis, ofstream* seiOut, int32_t iNumFrame)
{
  if(!seis.empty())
  {
    for(auto const& pSei: seis)
    {
      auto pPayload = pSei->payload;

      for(auto i = 0; i < pSei->numPayload; ++i, ++pPayload)
        WriteSei(pPayload->bPrefix, pPayload->type, pPayload->pData, pPayload->size, seiOut, iNumFrame);
    }
  }
}

/******************************************************************************/
static string SequencePictureToString(AL_ESequenceMode sequencePicture)
{
  if(sequencePicture == AL_SM_UNKNOWN)
    return "unknown";

  if(sequencePicture == AL_SM_PROGRESSIVE)
    return "progressive";

  if(sequencePicture == AL_SM_INTERLACED)
    return "interlaced";
  return "max enum";
}

/******************************************************************************/
static void ShowStreamInfo(int32_t BufferNumber, int32_t BufferSize, AL_TStreamSettings const* pStreamSettings, AL_TCropInfo const* pCropInfo, TFourCC tFourCC, AL_TDimension outputDim)
{
  int32_t iWidth = outputDim.iWidth;
  int32_t iHeight = outputDim.iHeight;

  stringstream ss;
  ss << "Resolution: " << iWidth << "x" << iHeight << endl;
  ss << "FourCC: " << AL_FourCCToString(tFourCC).cFourcc << endl;
  ss << "Profile: " << AL_GET_PROFILE_IDC(pStreamSettings->eProfile) << endl;
  int32_t iOutBitdepth = AL_GetBitDepth(tFourCC);

  if(AL_IS_AVC(pStreamSettings->eProfile) && pStreamSettings->iLevel == 9)
    ss << "Level: 1b" << endl;
  else

  if(pStreamSettings->iLevel != -1)
    ss << "Level: " << pStreamSettings->iLevel << endl;
  ss << "Bitdepth: " << iOutBitdepth << endl;

  if(AL_NeedsCropping(pCropInfo))
  {
    auto uCropWidth = pCropInfo->uCropOffsetLeft + pCropInfo->uCropOffsetRight;
    auto uCropHeight = pCropInfo->uCropOffsetTop + pCropInfo->uCropOffsetBottom;
    ss << "Crop top: " << pCropInfo->uCropOffsetTop << endl;
    ss << "Crop bottom: " << pCropInfo->uCropOffsetBottom << endl;
    ss << "Crop left: " << pCropInfo->uCropOffsetLeft << endl;
    ss << "Crop right: " << pCropInfo->uCropOffsetRight << endl;
    ss << "Display resolution: " << iWidth - uCropWidth << "x" << iHeight - uCropHeight << endl;
  }
  ss << "Sequence picture: " << SequencePictureToString(pStreamSettings->eSequenceMode) << endl;
  ss << "Buffers needed: " << BufferNumber << " of size " << BufferSize << endl;

  LogInfo(CC_DARK_BLUE, "%s\n", ss.str().c_str());
}

/******************************************************************************/
static int32_t sConfigureDecBufPool(PixMapBufPool& SrcBufPool, AL_TPicFormat const& tPicFormat, AL_TDimension const& tDim, int32_t iPitchY, bool bSetMultiChunk)
{
  /* When using multichunk option, we over-allocate each plane. It ensures it makes a difference for
     memory layout, compared to single-chunk mode. That way, we are sure SW/HW code access properly
     to the different planes using the dedicated addresses, instead of recomputing an offset based on
     resolution/chroma-mode...
  */
  const int32_t MULTICHUNK_ADDITIONAL_PLANE_SIZE = 2048;

  auto const tFourCC = AL_GetFourCC(&tPicFormat);
  SrcBufPool.SetFormat(tDim, tFourCC);

  std::vector<AL_TPlaneDescription> vPlaneDesc;
  int32_t iOffset = 0;

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  int32_t iNbPlanes = AL_Plane_GetBufferPixelPlanes(tPicFormat, usedPlanes);

  // Set pixels planes
  // -----------------
  for(int32_t iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    int32_t iPitch = (usedPlanes[iPlane] == AL_PLANE_Y || usedPlanes[iPlane] == AL_PLANE_YUV) ? iPitchY : AL_GetChromaPitch(tFourCC, iPitchY);
    vPlaneDesc.push_back(AL_TPlaneDescription { usedPlanes[iPlane], iOffset, iPitch });

    /* We ensure compatibility with 420/422. Required when we use prealloc when input chroma mode
     * to ensure we can handle chroma mode change in the stream. In that case, the UV plane is set
     * just like the U plane. Breaks planes agnostic allocation. */

    if(usedPlanes[iPlane] == AL_PLANE_U)
      vPlaneDesc.push_back(AL_TPlaneDescription { AL_PLANE_UV, iOffset, iPitch });

    iOffset += AL_DecGetAllocSize_Frame_PixPlane(&tPicFormat, tDim, iPitch, usedPlanes[iPlane]);

    if(bSetMultiChunk)
    {
      SrcBufPool.AddChunk(iOffset + MULTICHUNK_ADDITIONAL_PLANE_SIZE, vPlaneDesc);
      vPlaneDesc.clear();
      iOffset = 0;
    }
  }

  if(!bSetMultiChunk)
    SrcBufPool.AddChunk(iOffset, vPlaneDesc);

  return iOffset;
}

/******************************************************************************/
class DecoderContext
{
public:
  DecoderContext(Config& config, AL_TAllocator* pAllocator);
  ~DecoderContext();
  void CreateBaseDecoder(shared_ptr<I_IpDevice> device);
  AL_HDecoder GetBaseDecoderHandle() const { return hBaseDec; }
  AL_ERR SetupBaseDecoderPool(int32_t iBufferNumber, AL_TStreamSettings const* pStreamSettings, AL_TCropInfo const* pCropInfo);

  bool WaitExit(uint32_t uTimeout);
  void ReceiveFrameToDisplayFrom(DeviceType eDevice, AL_TBuffer* pFrame, AL_TInfoDecode* pInfo);
  int32_t GetNumConcealedFrame() const { return iNumFrameConceal; };
  int32_t GetNumDecodedFrames() const { return iNumDecodedFrames; };
  std::unique_lock<mutex> LockDisplay() { return std::unique_lock<mutex>(hDisplayMutex); };
  void StopSendingBuffer() { LockDisplay(); bPushBackToDecoder = false; };
  void SetPushBackToDecoder() { bPushBackToDecoder = true; };
  bool CanSendBackBufferToDecoder() { return bPushBackToDecoder; };
  void ReceiveBaseDecoderDecodedFrame(AL_TBuffer* pFrame);
  void ManageError(AL_ERR eError);
  void StoreSeiMetaData(AL_TBuffer* pParsedFrame, int32_t iParsingId);
  void PrintSei(bool bIsPrefix, int32_t iPayloadType, uint8_t* pPayload, int32_t iPayloadSize);

private:
  AL_TAllocator* pAllocator;
  AL_HDecoder hBaseDec = nullptr;
  DisplayManager tDisplayManager {};
  bool bPushBackToDecoder = true;
  int32_t iNumFrameConceal = 0;
  int32_t iNumDecodedFrames = 0;
  AL_TDecCallBacks CB {};
  AL_TDecSettings* pDecSettings;
  bool bUsePreAlloc = false;
  PixMapBufPool tBaseBufPool[MAX_DEC_NUM_LAYER];
  bool bSetRecPoolInMultiChunk = false;
  AL_TDecOutputSettings* pUserOutputSettings;
  ofstream seiOutput;
  ofstream seiSyncOutput;

  AL_HANDLE GetDecoderHandle(DeviceType eDevice) const;
  AL_ERR TreatError(DeviceType eDevice, AL_TBuffer const* pFrame, AL_TInfoDecode const* pInfo);
  void PrintSyncedSeiMetaData(AL_TBuffer* pFrame);
  AL_TDimension ComputeBaseDecoderFinalResolution(AL_TStreamSettings const* pStreamSettings);
  int32_t ComputeBaseDecoderRecBufferSizing(AL_TStreamSettings const* pStreamSettings, AL_TDecOutputSettings const* pUserOutputSettings);
  void AttachMetaDataToBaseDecoderRecBuffer(AL_TDimension const& tDimension, AL_TBuffer* pDecPict);

  bool bAddHDRMetaData = false;

  map<AL_TBuffer*, std::vector<AL_TSeiMetaData*>> displaySeis;
  map<AL_ERR, int32_t> errorCounts;
  EDecErrorLevel eExitCondition = DEC_ERROR;
  AL_EVENT hExitMain = nullptr;
  mutex hDisplayMutex;
};

/******************************************************************************/
DecoderContext::DecoderContext(Config& config, AL_TAllocator* pAlloc)
{
  pAllocator = pAlloc;
  pDecSettings = &config.tDecSettings;
  pUserOutputSettings = &config.tUserOutputSettings;
  tDisplayManager.Configure(config);

  bUsePreAlloc = config.bUsePreAlloc;

  bAddHDRMetaData = !config.hdrFile.empty();

  if(!config.seiFile.empty())
  {
    OpenOutput(seiOutput, config.seiFile);

    if(pDecSettings->eInputMode == AL_DEC_SPLIT_INPUT)
      OpenOutput(seiSyncOutput, config.seiFile + "_sync.txt");
  }

  eExitCondition = config.eExitCondition;
  hExitMain = Rtos_CreateEvent(false);
  bSetRecPoolInMultiChunk = config.bMultiChunk;
}

/******************************************************************************/
DecoderContext::~DecoderContext(void)
{
  Rtos_DeleteEvent(hExitMain);
}

/******************************************************************************/
AL_HANDLE DecoderContext::GetDecoderHandle(DeviceType eDevice) const
{
  (void)eDevice;
  AL_HANDLE h = hBaseDec;

  return h;
}

/******************************************************************************/
bool DecoderContext::WaitExit(uint32_t uTimeout)
{
  return Rtos_WaitEvent(hExitMain, uTimeout);
}

/******************************************************************************/
static AL_ERR sBaseResolutionFound(int32_t iBufferNumber, AL_TStreamSettings const* pStreamSettings, AL_TCropInfo const* pCropInfo, void* pUserParam)
{
  auto pCtx = (DecoderContext*)pUserParam;
  return pCtx->SetupBaseDecoderPool(iBufferNumber, pStreamSettings, pCropInfo);
}

/******************************************************************************/
AL_TDimension DecoderContext::ComputeBaseDecoderFinalResolution(AL_TStreamSettings const* pStreamSettings)
{
  AL_TDimension tOutputDim = pStreamSettings->tDim;

  /* For pre-allocation, we must use 8x8 (HEVC) or MB (AVC) rounded dimensions, like the SPS. */
  /* Actually, round up to the LCU so we're able to support resolution changes with the same LCU sizes. */
  /* And because we don't know the codec here, always use 64 as MB/LCU size. */
  tOutputDim.iWidth = static_cast<int32_t>(AL_RoundUp(tOutputDim.iWidth, 64));
  tOutputDim.iHeight = static_cast<int32_t>(AL_RoundUp(tOutputDim.iHeight, 64));

  return tOutputDim;
}

/******************************************************************************/
int32_t DecoderContext::ComputeBaseDecoderRecBufferSizing(AL_TStreamSettings const* pStreamSettings, AL_TDecOutputSettings const* pUserOutputSettings)
{
  uint8_t const uLayerID = 0;
  // Up to this point pUserOutputSettings is already updated in the resolution found callback (SetupBaseDecoderPool)
  int32_t iBufferSize = 0;

  // Compute output resolution
  AL_TDimension tOutputDim = ComputeBaseDecoderFinalResolution(pStreamSettings);

  // Buffer sizing
  auto minPitch = AL_Decoder_GetMinPitch(tOutputDim.iWidth, &pUserOutputSettings->tPicFormat);

  if(tBaseBufPool[uLayerID].IsInit())
    iBufferSize = AL_DecGetAllocSize_Frame(tOutputDim, minPitch, pUserOutputSettings->tPicFormat);
  else
    iBufferSize = sConfigureDecBufPool(tBaseBufPool[uLayerID], pUserOutputSettings->tPicFormat, tOutputDim, minPitch, bSetRecPoolInMultiChunk);

  return iBufferSize;
}

/******************************************************************************/
static void AddHDRMetaData(AL_TBuffer* pBufStream)
{
  if(AL_Buffer_GetMetaData(pBufStream, AL_META_TYPE_HDR))
    return;

  auto pHDReta = AL_HDRMetaData_Create();

  if(pHDReta)
    AL_Buffer_AddMetaData(pBufStream, (AL_TMetaData*)pHDReta);
}

/******************************************************************************/
void DecoderContext::AttachMetaDataToBaseDecoderRecBuffer(AL_TDimension const& tDimension, AL_TBuffer* pDecPict)
{
  (void)tDimension;

  if(bAddHDRMetaData)
    AddHDRMetaData(pDecPict);

  AL_TPictureDecMetaData* pPictureDecMeta = AL_PictureDecMetaData_Create();
  AL_Buffer_AddMetaData(pDecPict, (AL_TMetaData*)pPictureDecMeta);

  AL_TDisplayInfoMetaData* pDisplayInfoMeta = AL_DisplayInfoMetaData_Create();
  AL_Buffer_AddMetaData(pDecPict, (AL_TMetaData*)pDisplayInfoMeta);

}

/* Update picture format using stream settings and decoder's settings*/
static void SetDecOutputSettings(AL_TDecOutputSettings& tUserOutputSettings, AL_TStreamSettings const& tStreamSettings, AL_TDecSettings const& tDecSettings)
{
  {
    /* Chroma mode */
    if(AL_CHROMA_MAX_ENUM == tUserOutputSettings.tPicFormat.eChromaMode)
      tUserOutputSettings.tPicFormat.eChromaMode = tStreamSettings.eChroma;

    /* Bitdepth */
    bool bUserProvidedExplicitBitdepth =
      (tUserOutputSettings.tPicFormat.uBitDepth != (uint8_t)OUTPUT_BD_FIRST) &&
      (tUserOutputSettings.tPicFormat.uBitDepth != (uint8_t)OUTPUT_BD_ALLOC) &&
      (tUserOutputSettings.tPicFormat.uBitDepth != (uint8_t)OUTPUT_BD_STREAM);

    if(!bUserProvidedExplicitBitdepth)
      tUserOutputSettings.tPicFormat.uBitDepth = tStreamSettings.iBitDepth;

    /* Plane mode */
    if(AL_PLANE_MODE_MAX_ENUM == tUserOutputSettings.tPicFormat.ePlaneMode)
      tUserOutputSettings.tPicFormat.ePlaneMode = GetInternalBufPlaneMode(tUserOutputSettings.tPicFormat.eChromaMode);

    /* Component order */

    if(AL_COMPONENT_ORDER_MAX_ENUM == tUserOutputSettings.tPicFormat.eComponentOrder)
    {
      tUserOutputSettings.tPicFormat.eComponentOrder = AL_COMPONENT_ORDER_YUV;
    }

  }

  tUserOutputSettings.tPicFormat.bCompressed = IsOutputStorageModeCompressed(tUserOutputSettings, tDecSettings.bFrameBufferCompression);
  tUserOutputSettings.tPicFormat.eStorageMode = GetMainOutputStorageMode(tUserOutputSettings, tDecSettings.eFBStorageMode);

  if(IsTile(tUserOutputSettings.tPicFormat.eStorageMode))
    tUserOutputSettings.tPicFormat.eSamplePackMode = AL_SAMPLE_PACK_MODE_PACKED;

  /* TODO: The user will have to indicate the pack_mode so maybe this setting should not be here.
    Added from previous version of AL_GetDecPicFormat()*/
  if(AL_FB_RASTER == tUserOutputSettings.tPicFormat.eStorageMode && 10 == tUserOutputSettings.tPicFormat.uBitDepth)
    tUserOutputSettings.tPicFormat.eSamplePackMode = AL_SAMPLE_PACK_MODE_PACKED_XV;

  if(tUserOutputSettings.tPicFormat.ePlaneMode == AL_PLANE_MODE_INTERLEAVED && tUserOutputSettings.tPicFormat.eChromaMode == AL_CHROMA_4_4_4 && tUserOutputSettings.tPicFormat.eAlphaMode == AL_ALPHA_MODE_DISABLED)
    tUserOutputSettings.tPicFormat.eAlphaMode = AL_ALPHA_MODE_AFTER;
}

/******************************************************************************/
AL_ERR DecoderContext::SetupBaseDecoderPool(int32_t iBufferNumber, AL_TStreamSettings const* pStreamSettings, AL_TCropInfo const* pCropInfo)
{
  uint8_t const uLayerID = 0;
  auto lockDisplay = LockDisplay();

  SetDecOutputSettings(*pUserOutputSettings, *pStreamSettings, *pDecSettings);

  this->tDisplayManager.ConfigureMainOutputWriters(*pUserOutputSettings);

  if(!AL_Decoder_ConfigureOutputSettings(GetBaseDecoderHandle(), pUserOutputSettings))
  {
    std::string errstr;
    AL_ERR err;
    err = AL_Decoder_GetLastError(this->GetBaseDecoderHandle());
    errstr = "Could not configure the output settings";
    errstr += AL_Codec_ErrorToString(err);
    throw runtime_error(errstr);
  }

  /* Compute buffer sizing */
  int32_t iBufferSize = ComputeBaseDecoderRecBufferSizing(pStreamSettings, pUserOutputSettings);

  AL_TCropInfo pUserCropInfo = *pCropInfo;

  AL_TDimension outputDim = pStreamSettings->tDim;
  ShowStreamInfo(iBufferNumber, iBufferSize, pStreamSettings, &pUserCropInfo, AL_GetFourCC(&pUserOutputSettings->tPicFormat), outputDim);

  if(tBaseBufPool[uLayerID].IsInit())
    return AL_SUCCESS;

  /* Create the buffers */
  int32_t iNumBuf = iBufferNumber + uDefaultNumBuffersHeldByNextComponent;

  if(!tBaseBufPool[uLayerID].Init(pAllocator, iNumBuf, "decoded picture buffer"))
    return AL_ERR_NO_MEMORY;

  // Attach the metas + push to decoder
  // ----------------------------------
  for(int32_t i = 0; i < iNumBuf; ++i)
  {
    auto pDecPict = tBaseBufPool[uLayerID].GetSharedBuffer(AL_EBufMode::AL_BUF_MODE_NONBLOCK);

    if(!pDecPict)
      throw runtime_error("pDecPict is null");

    AL_Buffer_Cleanup(pDecPict.get());

    AttachMetaDataToBaseDecoderRecBuffer(pStreamSettings->tDim, pDecPict.get());

    bool bAdded;

    {
      bAdded = AL_Decoder_PutDisplayPicture(GetBaseDecoderHandle(), pDecPict.get());
    }

    if(!bAdded)
      throw runtime_error("bAdded must be true");
  }

  return AL_SUCCESS;
}

/******************************************************************************/
static void sInputParsed(AL_TBuffer* pParsedFrame, void* pUserParam, int32_t iParsingId)
{
  auto pCtx = (DecoderContext*)pUserParam;
  pCtx->StoreSeiMetaData(pParsedFrame, iParsingId);
}

/******************************************************************************/
void DecoderContext::StoreSeiMetaData(AL_TBuffer* pParsedFrame, int32_t iParsingId)
{
  AL_THandleMetaData* pHandlesMeta = (AL_THandleMetaData*)AL_Buffer_GetMetaData(pParsedFrame, AL_META_TYPE_HANDLE);

  if(!pHandlesMeta)
    return;

  if(iParsingId > AL_HandleMetaData_GetNumHandles(pHandlesMeta))
    throw runtime_error("ParsingId is out of bounds");

  AL_TDecMetaHandle* pDecMetaHandle = (AL_TDecMetaHandle*)AL_HandleMetaData_GetHandle(pHandlesMeta, iParsingId);

  if(pDecMetaHandle->eState == AL_DEC_HANDLE_STATE_CONCEALED)
    return;

  if(pDecMetaHandle->eState == AL_DEC_HANDLE_STATE_PROCESSED)
  {
    AL_TBuffer* pStream = pDecMetaHandle->pHandle;

    if(!pStream)
      throw runtime_error("pStream is not allocated");

    auto seiMeta = (AL_TSeiMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_SEI);

    if(seiMeta != nullptr)
    {
      AL_Buffer_RemoveMetaData(pStream, (AL_TMetaData*)seiMeta);
      displaySeis[pParsedFrame].push_back(seiMeta);
    }

    return;
  }

  throw runtime_error("Input parsing error");
}

/******************************************************************************/
int32_t convertBitDepthToEven(int32_t iBd)
{
  return ((iBd % 2) != 0) ? iBd + 1 : iBd;
}

static void sFrameDecoded(AL_TBuffer* pFrame, void* pUserParam)
{
  auto pCtx = static_cast<DecoderContext*>(pUserParam);
  pCtx->ReceiveBaseDecoderDecodedFrame(pFrame);
}

void DisplayManager::ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int32_t iBdOut, TFourCC tOutFourCC)
{
  AL_PixMapBuffer_SetDimension(&tRecBuf, info.tDim);

  iBdOut = convertBitDepthToEven(iBdOut);

  AL_TCropInfo tCrop {};
  tCrop = info.tCrop;
  AL_TPosition tPos = { 0, 0 };

  TFourCC tRecBufFourCC = AL_PixMapBuffer_GetFourCC(&tRecBuf);
  AL_TPicFormat tRecPicFormat;
  AL_GetPicFormat(tRecBufFourCC, &tRecPicFormat);

  bool bNewInputFourCCFound = false;

  if(tInputFourCC != tRecBufFourCC)
  {
    bNewInputFourCCFound = true;
    tInputFourCC = tRecBufFourCC;
  }

  if(tOutFourCC == FOURCC(NULL))
  {
    AL_EPlaneMode ePlaneMode = AL_PLANE_MODE_PLANAR;
    bool bMSB = tRecPicFormat.bMSB;

    AL_EComponentOrder eConvCompOrder = AL_COMPONENT_ORDER_YUV;

    AL_TPicFormat tConvPicFormat = AL_TPicFormat {
      tRecPicFormat.eChromaMode,
      AL_ALPHA_MODE_DISABLED,
      static_cast<uint8_t>(iBdOut),
      AL_FB_RASTER,
      ePlaneMode,
      eConvCompOrder,
      AL_SAMPLE_PACK_MODE_BYTE,
      false,
      bMSB
    };

    tOutFourCC = AL_GetFourCC(&tConvPicFormat);
  }
  else if(tOutFourCC == FOURCC(hard))
  {
    tOutFourCC = tRecBufFourCC;
  }

  bool bCompress = AL_IsCompressed(tRecBufFourCC);
  bool bConvert = !bCompress && tOutFourCC != tRecBufFourCC;

  AL_TDisplayInfoMetaData* pMeta = reinterpret_cast<AL_TDisplayInfoMetaData*>(AL_Buffer_GetMetaData(&tRecBuf, AL_META_TYPE_DISPLAY_INFO));

  if(pMeta)
    pMeta->tCrop = tCrop;

  AL_TDimension tDisplayDimension = AL_PixMapBuffer_GetDimension(&tRecBuf);

  if(tCrop.bCropping)
  {
    auto uCropWidth = tCrop.uCropOffsetLeft + tCrop.uCropOffsetRight;
    auto uCropHeight = tCrop.uCropOffsetTop + tCrop.uCropOffsetBottom;
    tDisplayDimension.iWidth = tDisplayDimension.iWidth - uCropWidth;
    tDisplayDimension.iHeight = tDisplayDimension.iHeight - uCropHeight;
  }

  if(m_tDisplayDimension.iWidth != tDisplayDimension.iWidth ||
     m_tDisplayDimension.iHeight != tDisplayDimension.iHeight)
  {
    m_tDisplayDimension = tDisplayDimension;
    stringstream ss;
    ss << "Display resolution: " << m_tDisplayDimension.iWidth << "x" << m_tDisplayDimension.iHeight << std::endl;
    LogInfo(CC_DARK_BLUE, "%s\n", ss.str().c_str());
    FFLUSH(stdout);
  }

  if(bConvert)
  {
    if(tInputFourCC != tOutFourCC && bNewInputFourCCFound)
    {
      stringstream ss;
      ss << "Software conversion done from " << AL_FourCCToString(tRecBufFourCC).cFourcc << " to " << AL_FourCCToString(tOutFourCC).cFourcc << endl;
      LogInfo(CC_DARK_BLUE, "%s\n", ss.str().c_str());
      FFLUSH(stdout);
    }

    AL_TBuffer* YuvBuffer = NULL;
    ConvertFrameBuffer(&tRecBuf, YuvBuffer, iBdOut, tPos, tOutFourCC);

    CopyMetaData(YuvBuffer, &tRecBuf, AL_META_TYPE_DISPLAY_INFO);

    multisinkOut->ProcessFrame(YuvBuffer);

    AL_Buffer_Destroy(YuvBuffer);
  }
  else
  {
    multisinkOut->ProcessFrame(&tRecBuf);
  }

}

/******************************************************************************/
void DecoderContext::ReceiveBaseDecoderDecodedFrame(AL_TBuffer* pFrame)
{
  PrintSyncedSeiMetaData(pFrame);

  if(GetBaseDecoderHandle())
    iNumDecodedFrames++;
}

/******************************************************************************/
void DecoderContext::PrintSyncedSeiMetaData(AL_TBuffer* pFrame)
{

  auto seis = displaySeis[pFrame];

  if(seiSyncOutput)
  {
    WriteSyncSei(seis, &seiSyncOutput, iNumDecodedFrames);
  }

  for(auto const& pSei: seis)
    AL_MetaData_Destroy((AL_TMetaData*)pSei);

  displaySeis.erase(pFrame);
}

/******************************************************************************/
static void sParsedSei(bool bIsPrefix, int32_t iPayloadType, uint8_t* pPayload, int32_t iPayloadSize, void* pUserParam)
{
  auto pCtx = static_cast<DecoderContext*>(pUserParam);
  pCtx->PrintSei(bIsPrefix, iPayloadType, pPayload, iPayloadSize);
}

/******************************************************************************/
void DecoderContext::PrintSei(bool bIsPrefix, int32_t iPayloadType, uint8_t* pPayload, int32_t iPayloadSize)
{
  WriteSei(bIsPrefix, iPayloadType, pPayload, iPayloadSize, &seiOutput, SEI_NOT_ASSOCIATED_WITH_FRAME);
}

/******************************************************************************/
static void sDecoderError(AL_ERR eError, void* pUserParam)
{
  auto pCtx = static_cast<DecoderContext*>(pUserParam);

  pCtx->ManageError(eError);
}

/******************************************************************************/
static void sBaseDecoderFrameDisplay(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo, void* pUserParam)
{
  auto pCtx = reinterpret_cast<DecoderContext*>(pUserParam);
  pCtx->ReceiveFrameToDisplayFrom(DEVICE_BASE_DECODER, pFrame, pInfo);
}

/******************************************************************************/
void DecoderContext::CreateBaseDecoder(shared_ptr<I_IpDevice> device)
{
  CB.endParsingCB = { &sInputParsed, this };
  CB.endDecodingCB = { &sFrameDecoded, this };
  CB.displayCB = { &sBaseDecoderFrameDisplay, this };
  CB.resolutionFoundCB = { &sBaseResolutionFound, this };
  CB.parsedSeiCB = { &sParsedSei, this };
  CB.errorCB = { &sDecoderError, this };

  AL_IDecScheduler* pScheduler = static_cast<AL_IDecScheduler*>(device->GetScheduler());

  AL_ERR error;
  error = AL_Decoder_Create(&hBaseDec, pScheduler, pAllocator, pDecSettings, &CB);

  if(AL_IS_ERROR_CODE(error))
    throw codec_error(error);

  if(!hBaseDec)
    throw runtime_error("Cannot create base decoder");
}

/******************************************************************************/
void DecoderContext::ManageError(AL_ERR eError)
{
  auto count = ++errorCounts[eError];

  if(eError == AL_WARN_UNEXPECTED_SLICE_IN_INTRA_PROFILE && count == 1)
  {
    SetConsoleColor(CC_YELLOW);
    cerr << " /!\\ Warning: " << AL_Codec_ErrorToString(eError) << endl;
    SetConsoleColor(CC_DEFAULT);
  }

  if(AL_IS_ERROR_CODE(eError) || eExitCondition == DEC_WARNING)
    Rtos_SetEvent(hExitMain);
}

/******************************************************************************/
void DecoderContext::ReceiveFrameToDisplayFrom(DeviceType eDevice, AL_TBuffer* pFrame, AL_TInfoDecode* pInfo)
{
  unique_lock<mutex> lock(hDisplayMutex);

  bool bLastFrame = false;

  if(IsEndOfStream(pFrame, pInfo))
  {
    LogVerbose(CC_GREY, "Complete\n\n");
    bLastFrame = true;

  }
  else if(!IsReleaseFrame(pFrame, pInfo))
  {
    AL_Buffer_Ref(pFrame);
    AL_Buffer_InvalidateMemory(pFrame);

    auto err = TreatError(eDevice, pFrame, pInfo);

    if(AL_IS_ERROR_CODE(err))
      bLastFrame = true;
    else
    {
      {
        bool bIsBaseDecoder = eDevice == DEVICE_BASE_DECODER;
        bool bIsFrameMainDisplay;
        auto hDec = GetDecoderHandle(eDevice);
        int32_t iBitDepthAlloc = 8;

        if(bIsBaseDecoder)
          iBitDepthAlloc = AL_Decoder_GetMaxBD(hDec);
        bool bDecoderExists = GetBaseDecoderHandle() != NULL;
        tDisplayManager.Process(pFrame, pInfo, iBitDepthAlloc, bIsFrameMainDisplay, bLastFrame, bDecoderExists);

        if(bIsFrameMainDisplay && CanSendBackBufferToDecoder() && !bLastFrame)
        {
          if(err == AL_WARN_CONCEAL_DETECT || err == AL_WARN_HW_CONCEAL_DETECT || err == AL_WARN_INVALID_ACCESS_UNIT_STRUCTURE)
            iNumFrameConceal++;

          if(bIsBaseDecoder)
          {
            bool bAdded;

            bAdded = AL_Decoder_PutDisplayPicture(GetDecoderHandle(eDevice), pFrame);

            if(!bAdded)
              throw runtime_error("bAdded must be true");
          }
        }
      }
    }

    AL_Buffer_Unref(pFrame);
  }

  bool bJobDone = bLastFrame;

  if(bJobDone)
  {
    Rtos_SetEvent(hExitMain);
  }
}

/******************************************************************************/
AL_ERR DecoderContext::TreatError(DeviceType eDevice, AL_TBuffer const* pFrame, AL_TInfoDecode const* pInfo)
{
  bool bExitError = false;
  AL_ERR err = AL_SUCCESS;
  (void)pInfo;

  auto hDec = GetDecoderHandle(eDevice);

  if(hDec)
  {
    if(eDevice == DEVICE_BASE_DECODER)
      err = AL_Decoder_GetFrameError(hDec, pFrame);

    bExitError |= AL_IS_ERROR_CODE(err);
  }

  if(bExitError)
  {
    LogDimmedWarning("\n%s\n", AL_Codec_ErrorToString(err));

    if(err == AL_WARN_SEI_OVERFLOW)
      LogDimmedWarning("\nDecoder has discarded some SEI while the SEI metadata buffer was too small\n");

    LogError("Error: %d\n", err);
  }

  return err;
}

/******************************************************************************/
void ShowStatistics(double durationInSeconds, int32_t iNumFrameConceal, int32_t decodedFrameNumber, bool timeoutOccurred)
{
  string guard = "Decoded time = ";

  if(timeoutOccurred)
    guard = "TIMEOUT = ";

  auto msg = guard + "%.4f s;  Decoding FrameRate ~ %.4f Fps; Frame(s) conceal = %d\n";
  LogInfo(msg.c_str(),
          durationInSeconds,
          decodedFrameNumber / durationInSeconds,
          iNumFrameConceal);
}

/******************************************************************************/
AL_TPixMapMetaData* CreateAndFillPixMapMeta(TFourCC tFourCC, AL_TDimension tDim, int32_t iPitchY)
{
  AL_TPicFormat tPicFmt;
  AL_GetPicFormat(tFourCC, &tPicFmt);
  bool bHasChroma = tPicFmt.eChromaMode != AL_CHROMA_4_0_0;
  bool bIs444 = tPicFmt.eChromaMode == AL_CHROMA_4_4_4;

  auto uSizeY = AL_DecGetAllocSize_Frame_PixPlane(&tPicFmt, tDim, iPitchY, AL_PLANE_Y);
  AL_TPlane tPlaneY {
    0, 0, iPitchY
  };

  int32_t iPitchC = 0;
  auto uSizeC = 0;
  AL_TPlane tPlaneU {
    0, 0, 0
  };
  AL_TPlane tPlaneV {
    0, 0, 0
  };
  int32_t uPlaneOffset = 0;

  if(bHasChroma)
  {
    AL_EPlaneId ePlaneId = AL_PLANE_UV;

    if(bIs444 || tPicFmt.ePlaneMode != AL_PLANE_MODE_SEMIPLANAR)
      ePlaneId = AL_PLANE_U;

    iPitchC = AL_GetChromaPitch(tFourCC, iPitchY);
    uSizeC = AL_DecGetAllocSize_Frame_PixPlane(&tPicFmt, tDim, iPitchC, ePlaneId);
    tPlaneU = { 0, uSizeY, iPitchC };
    tPlaneV = { 0, uSizeY + uSizeC, iPitchC };
  }

  // Attach PixMap
  AL_TPixMapMetaData* pMeta = AL_PixMapMetaData_CreateEmpty(tFourCC);
  AL_PixMapMetaData_AddPlane(pMeta, tPlaneY, AL_PLANE_Y);
  uPlaneOffset += uSizeY;

  if(bHasChroma)
  {
    if(bIs444 || tPicFmt.ePlaneMode != AL_PLANE_MODE_SEMIPLANAR)
    {
      AL_PixMapMetaData_AddPlane(pMeta, tPlaneU, AL_PLANE_U);
      AL_PixMapMetaData_AddPlane(pMeta, tPlaneV, AL_PLANE_V);
      uPlaneOffset += 2 * uSizeY;
    }
    else
    {
      uPlaneOffset += uSizeC;
    }
    AL_PixMapMetaData_AddPlane(pMeta, tPlaneU, AL_PLANE_UV);
  }

  pMeta->tDim = tDim;

  return pMeta;
}

/******************************************************************************/
typedef void (* EndOfInputCallBack)(AL_HANDLE hDec);
typedef bool (* PushBufferCallBack)(AL_HANDLE hDec, AL_TBuffer* pBuf, size_t uSize, uint8_t uFlags);

struct AsyncFileInput final
{
  AsyncFileInput(void);
  ~AsyncFileInput(void);
  void Init(AL_HDecoder hDec_, BufPool& bufPool_, EndOfInputCallBack endOfInputCB_, PushBufferCallBack pushBufferCB_);
  void ConfigureStreamInput(string const& sPath, string const& sPathSplitSizes, bool bSplitInput, AL_ECodec eCodec, bool bVclSplit, EInputContainer eContainer);
  void Start();

private:
  void Run();

  AL_HDecoder m_hDec;
  ifstream m_ifFileStream;
  ifstream ifFileSizes;
  BufPool* m_pBufPool;
  bool m_bStreamInputSet = false;
  std::unique_ptr<InputLoader> m_StreamLoader;
  thread m_thread;
  PushBufferCallBack m_pushBufferCB;
  EndOfInputCallBack m_endOfInputCB;
  atomic<bool> m_bExit;

};

/******************************************************************************/
AsyncFileInput::AsyncFileInput(void) {}

/******************************************************************************/
AsyncFileInput::~AsyncFileInput(void)
{
  m_bExit = true;

  if(m_thread.joinable())
    m_thread.join();

}

/******************************************************************************/
void AsyncFileInput::Init(AL_HDecoder hDec, BufPool& bufPool, EndOfInputCallBack endOfInputCB, PushBufferCallBack pushBufferCB)
{
  m_hDec = hDec;
  m_pBufPool = &bufPool;
  m_pushBufferCB = pushBufferCB;
  m_endOfInputCB = endOfInputCB;
  m_bExit = false;
}

/******************************************************************************/
void AsyncFileInput::ConfigureStreamInput(string const& sPath, string const& sPathSplitSizes, bool bSplitInput, AL_ECodec eCodec, bool bVclSplit, EInputContainer eContainer)
{
  (void)eCodec;
  (void)sPathSplitSizes;
  OpenInput(m_ifFileStream, sPath);
  m_bStreamInputSet = true;

  if(!sPathSplitSizes.empty())
  {
    OpenInput(ifFileSizes, sPathSplitSizes, false);
    m_StreamLoader.reset(new SplitInputFromSizes(ifFileSizes));
  }

  if(eContainer == EInputContainer::Heif)
  {
    m_StreamLoader.reset(new HeifLoader(eCodec));
    return;
  }

  if(bSplitInput)
  {

    if(AL_IS_ITU_CODEC(eCodec))
      m_StreamLoader.reset(new SplitInput(m_pBufPool->GetBufSize(), eCodec, bVclSplit));

  }
  else
    m_StreamLoader.reset(new BasicLoader());
}

/******************************************************************************/
void AsyncFileInput::Start(void)
{
  if(!m_bStreamInputSet)
    throw runtime_error("Stream input must be set (call AsyncFileInput::ConfigureStreamInput)");

  m_thread = thread(&AsyncFileInput::Run, this);
}

/******************************************************************************/
void AsyncFileInput::Run(void)
{
  Rtos_SetCurrentThreadName("FileInput");

  while(!m_bExit)
  {
    shared_ptr<AL_TBuffer> pInputBuf;
    try
    {
      pInputBuf = m_pBufPool->GetSharedBuffer();
    }
    catch(bufpool_decommited_error &)
    {
      continue;
    }

    uint8_t uBufFlags;
    bool bInputFinished = false;
    uint32_t uAvailSize = 0;
    {
      uAvailSize = m_StreamLoader->ReadStream(m_ifFileStream, pInputBuf.get(), uBufFlags);
      bInputFinished = (uAvailSize == 0);
    }

    if(bInputFinished)
    {
      m_endOfInputCB(m_hDec);
      break;
    }

    auto bRet = m_pushBufferCB(m_hDec, pInputBuf.get(), uAvailSize, uBufFlags);

    if(!bRet)
      throw runtime_error("Failed to push buffer");
  }
}

/******************************************************************************/
constexpr int32_t MAX_CHANNELS = 32;

/******************************************************************************/
int32_t GetChannelsArgv(vector<char*>* argvChannels, int32_t argc, char** argv)
{
  int32_t curChan = 0;

  for(int32_t i = 0; i < argc; ++i)
  {
    if(string(argv[i]) == "--next-chan")
    {
      ++curChan;

      if(curChan >= MAX_CHANNELS)
        throw runtime_error("Too many channels");

      argvChannels[curChan].push_back(argv[0]);
      continue;
    }

    argvChannels[curChan].push_back(argv[i]);
  }

  return curChan;
}

/******************************************************************************/
struct WorkerConfig
{
  Config* pConfig;
  Devices* devices;
  UseBoards* useBoards;
};

/******************************************************************************/
void AdjustStreamBufferSettings(Config& config)
{
  uint32_t uMinStreamBuf = config.tDecSettings.iStackSize;
  config.uInputBufferNum = max(uMinStreamBuf, config.uInputBufferNum);
  config.zInputBufferSize = max(size_t(1), config.zInputBufferSize);

  bool bUsePreAlloc = config.UseBaseDecoder() && config.bUsePreAlloc && config.zInputBufferSize == zDefaultInputBufferSize;

  if(bUsePreAlloc)
    config.zInputBufferSize = AL_GetMaxNalSize(config.tDecSettings.tStream.tDim, config.tDecSettings.tStream.eChroma,
                                               config.tDecSettings.tStream.iBitDepth, config.tDecSettings.tStream.eProfile,
                                               config.tDecSettings.tStream.iLevel);
}

/******************************************************************************/
void CheckAndAdjustChannelConfiguration(Config& config)
{
  FILE* out = g_Verbosity ? stdout : nullptr;

  // Check base decoder settings
  // ---------------------------
  if(config.UseBaseDecoder())
  {
    int32_t err = AL_DecSettings_CheckValidity(&config.tDecSettings, out);
    err += AL_DecOutputSettings_CheckValidity(&config.tUserOutputSettings, config.tDecSettings.eCodec, out);

    if(err)
    {
      stringstream ss;
      ss << err << " errors(s). " << "Invalid settings, please check your command line.";
      throw runtime_error(ss.str());
    }

    auto const incoherencies = AL_DecSettings_CheckCoherency(&config.tDecSettings, out);

    if(incoherencies < 0)
      throw runtime_error("Fatal coherency error in settings, please check your command line.");
  }

  // Adjust settings
  // ---------------
  AdjustStreamBufferSettings(config);
}

/******************************************************************************/
void ConfigureInputPool(Config const& config, AL_TAllocator* pAllocator, BufPool& tInputPool)
{
  std::string sDebugName = "input_pool";
  uint32_t uNumBuf = config.uInputBufferNum;
  uint32_t zBufSize = config.zInputBufferSize;
  auto pBufPoolAllocator = config.tDecSettings.eInputMode == AL_DEC_SPLIT_INPUT ? pAllocator : AL_GetDefaultAllocator();
  AL_TMetaData* pBufMeta = nullptr;

  auto ret = tInputPool.Init(pBufPoolAllocator, uNumBuf, zBufSize, pBufMeta, sDebugName);

  if(pBufMeta != nullptr)
    AL_MetaData_Destroy(pBufMeta);

  if(!ret)
    throw runtime_error("Can't create BufPool");
}

/******************************************************************************/
void SafeRunChannelMain(WorkerConfig& w)
{
  auto& config = *w.pConfig;
  AL_TAllocator* pAllocator = nullptr;

  std::shared_ptr<CIpDevice> pIpDevice = nullptr;

  if(config.eSchedulerType == AL_ESchedulerType::AL_SCHEDULER_TYPE_MCU)
  {
    pIpDevice = std::dynamic_pointer_cast<CIpDevice>(w.devices->at(DEVICE_BASE_DECODER));

    if(!pIpDevice)
      throw std::runtime_error("Failed to cast I_IpDevice to CIpDevice");
  }

  if(config.UseBaseDecoder())
    pAllocator = w.devices->at(DEVICE_BASE_DECODER)->GetAllocator();

  // Settings checks
  // ------------------
  CheckAndAdjustChannelConfiguration(config);

  // Ugly goto that need to be revamp in the upper layer
  // ---------------------------------------------------
  findDecDev:
  bool bShouldUseGoto = false;
  {

  // Configure the decoders
  // ----------------------
  DecoderContext tDecCtx(config, pAllocator);

  // Create the decoders
  // -------------------
  if(config.UseBaseDecoder())
  {
    shared_ptr<I_IpDevice> device = w.devices->at(DEVICE_BASE_DECODER);

    if(config.bSelectDeviceWithLowestAvailableResources && bFindNextDevice)
    {
      tDecCtx.SetPushBackToDecoder();
      pIpDevice->SelectNextDevice();
      pAllocator = pIpDevice->GetAllocator();
      bFindNextDevice = false;
    }

    tDecCtx.CreateBaseDecoder(device);
  }

  // Parametrization of the base decoder for traces
  // ----------------------------------------------
  if(config.UseBaseDecoder())
  {
    auto hDec = tDecCtx.GetBaseDecoderHandle();
    AL_Decoder_SetParam(hDec, w.useBoards->at(DEVICE_BASE_DECODER) ? "Fpga" : "Ref", config.iTraceIdx, config.iTraceNumber, config.ipCtrlMode == AL_EIpCtrlMode::AL_IPCTRL_MODE_TRACE, config.bOutputTraceOnly);
  }

  // Parametrization of the lcevc decoder for traces
  // -----------------------------------------------

  // Configure the stream buffer pool
  // --------------------------------
  // Note : Must be before scopeExit so that AL_Decoder_Destroy can be called
  // before the BufPool destroyer. Can it be done differently so that it is not dependant of this order ?
  BufPool tInputPool;
  ConfigureInputPool(config, pAllocator, tInputPool);

  // Insure destroying is done even after throwing
  // ---------------------------------------------
  auto scopeDecoder = scopeExit([&]() {
    tDecCtx.StopSendingBuffer(); // Prevent to push buffer to the decoder while destroying it

    if(config.UseBaseDecoder())
      AL_Decoder_Destroy(tDecCtx.GetBaseDecoderHandle());
  });

  // Use preallocation for buffer sizing
  // -----------------------------------
  if(config.UseBaseDecoder() && config.bUsePreAlloc)
  {
    auto hDec = tDecCtx.GetBaseDecoderHandle();

    if(!AL_Decoder_PreallocateBuffers(hDec))
      if(auto eErr = AL_Decoder_GetLastError(hDec))
        throw codec_error(eErr);
  }

  // Start feeding the decoder
  // -------------------------
  auto const uBegin = GetPerfTime();
  bool timeoutOccurred = false;

  for(int32_t iLoop = 0; iLoop < config.iLoop; ++iLoop)
  {
    tInputPool.Commit();

    if(iLoop > 0)
      LogVerbose(CC_GREY, "  Looping\n");

    // Setup the reader of bitstream in the file.
    // It will send bitstream chunk to the decoder
    AsyncFileInput producer;
    AL_ECodec eCodec = config.tDecSettings.eCodec;

    producer.Init(tDecCtx.GetBaseDecoderHandle(), tInputPool, AL_Decoder_Flush, AL_Decoder_PushStreamBuffer);

    producer.ConfigureStreamInput(config.sIn, config.sSplitSizesFile, config.tDecSettings.eInputMode == AL_DEC_SPLIT_INPUT, eCodec, config.tDecSettings.eDecUnit == AL_VCL_NAL_UNIT, config.eContainer);
    producer.Start();

    auto const maxWait = config.iTimeoutInSeconds * 1000;
    auto const timeout = maxWait >= 0 ? maxWait : AL_WAIT_FOREVER;

    if(!tDecCtx.WaitExit(timeout))
      timeoutOccurred = true;

    tInputPool.Decommit();
  }

  auto const uEnd = GetPerfTime();

  // Prevent the display to produce some outputs
  auto lock = tDecCtx.LockDisplay();

  // Get the errors
  // --------------
  AL_ERR eErr = AL_SUCCESS;

  if(tDecCtx.GetBaseDecoderHandle())
    eErr = AL_Decoder_GetLastError(tDecCtx.GetBaseDecoderHandle());

  if(AL_IS_ERROR_CODE(eErr) || (AL_IS_WARNING_CODE(eErr) && config.eExitCondition == DEC_WARNING))
  {

    if((config.eSchedulerType == AL_ESchedulerType::AL_SCHEDULER_TYPE_MCU) && config.bSelectDeviceWithLowestAvailableResources)
    {
      bFindNextDevice = pIpDevice->HandleDeviceFailure();

      if(!bFindNextDevice)
        throw codec_error(eErr);
      bShouldUseGoto = true;
    }
    else
    throw codec_error(eErr);
  }

// Part of the ugly goto, need to be removed
  if(!bShouldUseGoto)
  {

  if(AL_IS_WARNING_CODE(eErr))
    cerr << endl << "Warning: " << AL_Codec_ErrorToString(eErr) << endl;

  if(!tDecCtx.GetNumDecodedFrames())
    throw runtime_error("No frame decoded");

  auto const duration = (uEnd - uBegin) / 1000.0;
  ShowStatistics(duration, tDecCtx.GetNumConcealedFrame(), tDecCtx.GetNumDecodedFrames(), timeoutOccurred);
  // Part of the ugly goto, need to be removed
}
}// <- End of tDecCtx

if(bShouldUseGoto)
  goto findDecDev;
}

/******************************************************************************/
static std::shared_ptr<CIpDevice> CreateAndConfigureBaseDecoderIpDevice(Config const* pConfig)
{
  CIpDeviceParam param;

  param.eSchedulerType = pConfig->eSchedulerType;
  param.eDeviceType = pConfig->eDeviceType;
  param.eTrackDmaMode = pConfig->eTrackDmaMode;
  param.ipCtrlMode = pConfig->ipCtrlMode;
  param.apbFile = pConfig->apbFile;
  static std::set<std::string> decDevicePath = pConfig->sDecDevicePath;
  param.bSelectDeviceWithLowestAvailableResources = pConfig->bSelectDeviceWithLowestAvailableResources;

  std::shared_ptr<CIpDevice> pIpDevice = std::shared_ptr<CIpDevice>(new CIpDevice(param, pConfig->eDeviceType, decDevicePath));

  if(!pIpDevice)
    throw runtime_error("Can't create BaseDecoderIpDevice");

  return pIpDevice;
}

/******************************************************************************/
void SetupArchitecture(Config const& conf)
{
  (void)conf;
  AL_ELibDecoderArch eArch = AL_LIB_DECODER_ARCH_HOST;

  if(!AL_IS_SUCCESS_CODE(AL_Lib_Decoder_Init(eArch)))
    throw runtime_error("Can't setup decode library");

}

/******************************************************************************/
int32_t GetChannelConfigurations(int32_t argc, char** argv, array<Config, MAX_CHANNELS>& cfgChannels)
{
  vector<char*> argvChannels[MAX_CHANNELS] {};
  int32_t const iNbChan = GetChannelsArgv(argvChannels, argc, argv) + 1;

  for(int32_t chan = 0; chan < iNbChan; ++chan)
    cfgChannels.at(chan) = ParseCommandLine((int)argvChannels[chan].size(), argvChannels[chan].data());

  return iNbChan;
}

/******************************************************************************/
// Run one channel only
static void RunChannelMain(WorkerConfig& w, std::exception_ptr& exception)
{
  try
  {
    SafeRunChannelMain(w);
    exception = nullptr;
    return;
  }
  catch(codec_error const& error)
  {
    (void)error;
    exception = std::current_exception();
  }
  catch(runtime_error const& error)
  {
    (void)error;
    exception = std::current_exception();
  }
}

/******************************************************************************/
void RunChannels(array<Config, MAX_CHANNELS>& cfgChannels, uint8_t uNbChan, Devices& devices, UseBoards& useBoards)
{
  array<std::exception_ptr, MAX_CHANNELS> errorChannels {};
  array<WorkerConfig, MAX_CHANNELS> workerConfigs;

  // Set the worker configurations
  // -----------------------------
  for(int32_t chan = 0; chan < uNbChan; ++chan)
  {
    WorkerConfig w
    {
      &cfgChannels.at(chan),
      &devices,
      &useBoards,
    };

    workerConfigs.at(chan) = w;
  }

  // Mono channel case
  // -----------------
  if(uNbChan == 1)
  {
    RunChannelMain(workerConfigs.at(0), errorChannels.at(0));

    if(errorChannels[0])
      std::rethrow_exception(errorChannels[0]);
  }
  // Multichannel case
  // -----------------
  else
  {
    // Launch all channel in different threads
    array<std::thread, MAX_CHANNELS> workers;

    for(int32_t chan = 0; chan < uNbChan; ++chan)
      workers[chan] = std::thread(&RunChannelMain, std::ref(workerConfigs[chan]), std::ref(errorChannels[chan]));

    // Wait all the channels are finished
    for(int32_t chan = 0; chan < uNbChan; ++chan)
      workers[chan].join();

    // Check for errors
    for(int32_t chan = 0; chan < uNbChan; ++chan)
    {
      if(errorChannels[chan])
      {
        cerr << "Channel " << chan << " has errors" << endl;
        std::rethrow_exception(errorChannels[chan]);
      }
    }
  }
}

/******************************************************************************/
void SafeMain(int32_t argc, char** argv)
{
  InitializePlateform();

  // Get all channel configuration
  // -----------------------------
  array<Config, MAX_CHANNELS> cfgChannels;
  int32_t const maxChan = GetChannelConfigurations(argc, argv, cfgChannels);

  // Use first channel to configure the ip devices
  auto config = cfgChannels[0];

  if(config.help)
    return;

  DisplayVersionInfo();

  // Setup of the decoder(s) architecture
  // ------------------------------------
  SetupArchitecture(config);

  // Create the devices
  // ------------------
  Devices devices;
  UseBoards useBoards;

  if(config.UseBaseDecoder())
  {
    devices.insert({ DEVICE_BASE_DECODER, CreateAndConfigureBaseDecoderIpDevice(&config) });
    useBoards.insert({ DEVICE_BASE_DECODER, (config.eDeviceType == AL_EDeviceType::AL_DEVICE_TYPE_BOARD) });
  }

  // Run all the channels
  // --------------------
  RunChannels(cfgChannels, maxChan, devices, useBoards);

  AL_Lib_Decoder_DeInit();
}

/******************************************************************************/
int main(int argc, char** argv)
{
  try
  {
    SafeMain(argc, argv);
    return 0;
  }
  catch(codec_error const& error)
  {
    cerr << endl << "Codec error: " << error.what() << endl;
    return error.Code;
  }
  catch(runtime_error const& error)
  {
    cerr << endl << "Exception caught: " << error.what() << endl;
    return 1;
  }
}

/******************************************************************************/
