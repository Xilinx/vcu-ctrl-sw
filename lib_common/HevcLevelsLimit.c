/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

#include "HevcLevelsLimit.h"
#include "Utils.h"

/****************************************************************************/
bool AL_HEVC_CheckLevel(int level)
{
  return (level == 10)
         || ((level >= 20) && (level <= 21))
         || ((level >= 30) && (level <= 31))
         || ((level >= 40) && (level <= 41))
         || ((level >= 50) && (level <= 52))
         || ((level >= 60) && (level <= 62));
}

/****************************************************************************/
uint32_t AL_HEVC_GetMaxNumberOfSlices(int level)
{
  switch(level)
  {
  case 10:
  case 20: return 16;
  case 21: return 20;
  case 30: return 30;
  case 31: return 40;
  case 40:
  case 41: return 75;
  case 50:
  case 51:
  case 52: return 200;
  case 60:
  case 61:
  case 62:
  default: return 600;
  }
}

/*****************************************************************************/
uint32_t AL_HEVC_GetMaxTileColumns(int level)
{
  switch(level)
  {
  case 10:
  case 20:
  case 21: return 1;
  case 30: return 2;
  case 31: return 3;
  case 40:
  case 41: return 5;
  case 50:
  case 51:
  case 52: return 10;
  default: return 20;
  }
}

/*****************************************************************************/
uint32_t AL_HEVC_GetMaxTileRows(int level)
{
  switch(level)
  {
  case 10:
  case 20:
  case 21: return 1;
  case 30: return 2;
  case 31: return 3;
  case 40:
  case 41: return 5;
  case 50:
  case 51:
  case 52: return 11;
  default: return 22;
  }
}

/****************************************************************************/
uint32_t AL_HEVC_GetMaxCPBSize(int level, int tier)
{
  if(tier == 1)
  {
    switch(level)
    {
    case 10: return 350u;
    case 20: return 1500u;
    case 21: return 3000u;
    case 30: return 6000u;
    case 31: return 10000u;
    case 40: return 30000u;
    case 41: return 50000u;
    case 50: return 100000u;
    case 51: return 160000u;
    case 52: return 240000u;
    case 60: return 240000u;
    case 61: return 480000u;
    case 62:
    default: return 800000u;
    }
  }
  else
  {
    switch(level)
    {
    case 10: return 350u;
    case 20: return 1500u;
    case 21: return 3000u;
    case 30: return 6000u;
    case 31: return 10000u;
    case 40: return 12000u;
    case 41: return 20000u;
    case 50: return 25000u;
    case 51: return 40000u;
    case 52: return 60000u;
    case 60: return 60000u;
    case 61: return 120000u;
    case 62:
    default: return 240000u;
    }
  }
}

/******************************************************************************/
static int getMaxLumaPS(int iLevel)
{
  switch(iLevel)
  {
  case 10: return 36864;
  case 20: return 122880;
  case 21: return 245760;
  case 30: return 552960;
  case 31: return 983040;
  case 40:
  case 41: return 2228224;
  case 50:
  case 51:
  case 52:
  default: return 8912896;
  case 60:
  case 61:
  case 62: return 35651584;
  }
}

/******************************************************************************/
uint32_t AL_HEVC_GetMaxDPBSize(int iLevel, int iWidth, int iHeight, bool bIsIntraProfile, bool bIsStillProfile, bool bDecodeIntraOnly)
{
  if(bIsStillProfile)
    return 1;

  if(bIsIntraProfile || bDecodeIntraOnly)
    return 2;

  int iMaxLumaPS = getMaxLumaPS(iLevel);

  int const iPictSizeY = iWidth * iHeight;

  if(iPictSizeY <= (iMaxLumaPS / 4))
    return 16;
  else if(iPictSizeY <= (iMaxLumaPS / 2))
    return 12;
  else if(iPictSizeY <= ((iMaxLumaPS * 2) / 3))
    return 9;
  else if(iPictSizeY <= ((3 * iMaxLumaPS) / 4))
    return 8;
  else
    return 6;
}

/****************************************************************************/
const AL_TLevelLimit AL_HEVC_MAX_PIX_PER_FRAME[] =
{
  { 36864u, 10u },
  { 122880u, 20u },
  { 245760u, 21u },
  { 552960u, 30u },
  { 983040u, 31u },
  { 2228224u, 40u },
  { 8912896u, 50u },
  { 35651584u, 60u }
};

// Max MB processing rate
const AL_TLevelLimit HEVC_MAX_PIX_RATE[] =
{
  { 552960u, 10u },
  { 3686400u, 20u },
  { 7372800u, 21u },
  { 16588800u, 30u },
  { 33177600u, 31u },
  { 66846720u, 40u },
  { 133693440u, 41u },
  { 267386880u, 50u },
  { 534773760u, 51u },
  { 1069547520u, 52u },
  { 2139095040u, 61u },
  { 4278190080u, 62u }
};

// Max Video processing rate - High Tier
const AL_TLevelLimit HEVC_MAX_VIDEO_BITRATE_HIGH[] =
{
  { 128u, 10u },
  { 1500u, 20u },
  { 3000u, 21u },
  { 6000u, 30u },
  { 10000u, 31u },
  { 30000u, 40u },
  { 50000u, 41u },
  { 100000u, 50u },
  { 160000u, 51u },
  { 240000u, 52u },
  { 480000u, 61u },
  { 800000u, 62u }
};

// Max Video processing rate - MainTier
const AL_TLevelLimit HEVC_MAX_VIDEO_BITRATE_MAIN[] =
{
  { 128u, 10u },
  { 1500u, 20u },
  { 3000u, 21u },
  { 6000u, 30u },
  { 10000u, 31u },
  { 12000u, 40u },
  { 20000u, 41u },
  { 25000u, 50u },
  { 40000u, 51u },
  { 60000u, 52u },
  { 120000u, 61u },
  { 240000u, 62u }
};

// Max number of tile columns
const AL_TLevelLimit HEVC_MAX_TILE_COLS[] =
{
  { 1u, 10u },
  { 2u, 30u },
  { 3u, 31u },
  { 5u, 40u },
  { 10u, 50u },
  { 20u, 60u },
};

#define NUM_LIMIT(array) (sizeof(array) / sizeof(AL_TLevelLimit))

/*************************************************************************/
uint8_t AL_HEVC_GetLevelFromFrameSize(int numPix)
{
  return AL_GetRequiredLevel(numPix, AL_HEVC_MAX_PIX_PER_FRAME, NUM_LIMIT(AL_HEVC_MAX_PIX_PER_FRAME));
}

/*************************************************************************/
uint8_t AL_HEVC_GetLevelFromPixRate(int pixRate)
{
  return AL_GetRequiredLevel(pixRate, HEVC_MAX_PIX_RATE, NUM_LIMIT(HEVC_MAX_PIX_RATE));
}

/*************************************************************************/
uint8_t AL_HEVC_GetLevelFromBitrate(int bitrate, int tier)
{
  if(tier)
    return AL_GetRequiredLevel(bitrate, HEVC_MAX_VIDEO_BITRATE_HIGH, NUM_LIMIT(HEVC_MAX_VIDEO_BITRATE_HIGH));
  else
    return AL_GetRequiredLevel(bitrate, HEVC_MAX_VIDEO_BITRATE_MAIN, NUM_LIMIT(HEVC_MAX_VIDEO_BITRATE_MAIN));
}

/****************************************************************************/
uint8_t AL_HEVC_GetLevelFromTileCols(int tileCols)
{
  return AL_GetRequiredLevel(tileCols, HEVC_MAX_TILE_COLS, NUM_LIMIT(HEVC_MAX_TILE_COLS));
}

/****************************************************************************/
static uint8_t AL_HEVC_GetMaxDpbPicBuf(int maxPixRate, int numPixPerFrame)
{
  // Values computed from HEVC Annex A - with maxDpbPicBuf = 6
  if(numPixPerFrame <= (maxPixRate >> 2))
    return 16;
  else if(numPixPerFrame <= (maxPixRate >> 1))
    return 12;
  else if(numPixPerFrame <= ((3 * maxPixRate) >> 2))
    return 8;

  return 6;
}

/****************************************************************************/
uint8_t AL_HEVC_GetLevelFromDPBSize(int dpbSize, int pixRate)
{
  for(size_t i = 0; i < NUM_LIMIT(AL_HEVC_MAX_PIX_PER_FRAME); i++)
  {
    uint8_t maxDpbSize = AL_HEVC_GetMaxDpbPicBuf(AL_HEVC_MAX_PIX_PER_FRAME[i].uLimit, pixRate);

    if(dpbSize <= maxDpbSize)
      return AL_HEVC_MAX_PIX_PER_FRAME[i].uLevel;
  }

  return 255;
}

