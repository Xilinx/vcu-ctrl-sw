// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "HeifLoader.hpp"

extern "C"
{
#include "lib_common/Nuts.h"
}

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
struct AL_THeifBox
{
  std::string sType;
  size_t zStart = 0;
  size_t zPayload = 0;
  size_t zEnd = 0;
};

struct AL_THeifBrands
{
  bool bAvifBrand = false;
  bool bHeicBrand = false;
};

class CHeifReader
{
public:
  CHeifReader(std::vector<uint8_t> const& vData, size_t zBegin, size_t zEnd) :
    m_vData(vData),
    m_zPos(zBegin),
    m_zEnd(zEnd)
  {
    if(zBegin > zEnd || zEnd > vData.size())
      throw std::runtime_error("Invalid HEIF reader range");
  }

  size_t GetPosition() const
  {
    return m_zPos;
  }

  size_t GetEnd() const
  {
    return m_zEnd;
  }

  size_t GetBytesLeft() const
  {
    return m_zEnd - m_zPos;
  }

  bool IsEmpty() const
  {
    return GetBytesLeft() == 0;
  }

  void SetPosition(size_t zPos)
  {
    if(zPos > m_zEnd)
      throw std::runtime_error("HEIF parser seek beyond box end");

    m_zPos = zPos;
  }

  void Skip(size_t zSize)
  {
    Require(zSize);
    m_zPos += zSize;
  }

  uint8_t ReadU8()
  {
    Require(1);
    return m_vData[m_zPos++];
  }

  uint16_t ReadU16BE()
  {
    Require(2);
    uint16_t uValue = ((uint16_t)m_vData[m_zPos] << 8) |
                      ((uint16_t)m_vData[m_zPos + 1]);
    m_zPos += 2;
    return uValue;
  }

  uint32_t ReadU32BE()
  {
    Require(4);
    uint32_t uValue = ((uint32_t)m_vData[m_zPos] << 24) |
                      ((uint32_t)m_vData[m_zPos + 1] << 16) |
                      ((uint32_t)m_vData[m_zPos + 2] << 8) |
                      ((uint32_t)m_vData[m_zPos + 3]);
    m_zPos += 4;
    return uValue;
  }

  uint64_t ReadU64BE()
  {
    Require(8);
    uint64_t uValue = ((uint64_t)m_vData[m_zPos] << 56) |
                      ((uint64_t)m_vData[m_zPos + 1] << 48) |
                      ((uint64_t)m_vData[m_zPos + 2] << 40) |
                      ((uint64_t)m_vData[m_zPos + 3] << 32) |
                      ((uint64_t)m_vData[m_zPos + 4] << 24) |
                      ((uint64_t)m_vData[m_zPos + 5] << 16) |
                      ((uint64_t)m_vData[m_zPos + 6] << 8) |
                      ((uint64_t)m_vData[m_zPos + 7]);
    m_zPos += 8;
    return uValue;
  }

  uint64_t ReadUIntBE(size_t zSize)
  {
    if(zSize > 8)
      throw std::runtime_error("Unsupported HEIF integer field size");

    Require(zSize);
    uint64_t uValue = 0;

    for(size_t i = 0; i < zSize; ++i)
      uValue = (uValue << 8) | m_vData[m_zPos + i];

    m_zPos += zSize;
    return uValue;
  }

  std::string ReadFourCC()
  {
    Require(4);
    std::string sFourCC(reinterpret_cast<char const*>(&m_vData[m_zPos]), 4);
    m_zPos += 4;
    return sFourCC;
  }

  std::vector<uint8_t> ReadBytes(size_t zSize)
  {
    Require(zSize);
    std::vector<uint8_t> vBytes(m_vData.begin() + static_cast<std::ptrdiff_t>(m_zPos),
                                m_vData.begin() + static_cast<std::ptrdiff_t>(m_zPos + zSize));
    m_zPos += zSize;
    return vBytes;
  }

  std::vector<uint8_t> ReadRemainingBytes()
  {
    std::vector<uint8_t> vBytes(m_vData.begin() + static_cast<std::ptrdiff_t>(m_zPos),
                                m_vData.begin() + static_cast<std::ptrdiff_t>(m_zEnd));
    m_zPos = m_zEnd;
    return vBytes;
  }

private:
  void Require(size_t zSize) const
  {
    if(zSize > GetBytesLeft())
      throw std::runtime_error("Truncated HEIF box");
  }

  std::vector<uint8_t> const& m_vData;
  size_t m_zPos = 0;
  size_t m_zEnd = 0;
};

static AL_THeifBox ReadBox(CHeifReader& tReader)
{
  size_t zStart = tReader.GetPosition();

  if(tReader.GetBytesLeft() < 8)
    throw std::runtime_error("Truncated HEIF box header");

  uint32_t uSize32 = tReader.ReadU32BE();
  std::string sType = tReader.ReadFourCC();
  uint64_t uSize = uSize32;

  if(uSize32 == 1)
    uSize = tReader.ReadU64BE();
  else if(uSize32 == 0)
    uSize = tReader.GetEnd() - zStart;

  size_t zPayload = tReader.GetPosition();

  if(uSize < (zPayload - zStart))
    throw std::runtime_error("Invalid HEIF box size");

  uint64_t uEnd = (uint64_t)zStart + uSize;

  if(uEnd > tReader.GetEnd() || uEnd > std::numeric_limits<size_t>::max())
    throw std::runtime_error("HEIF box exceeds parent bounds");

  AL_THeifBox tBox {};
  tBox.sType = sType;
  tBox.zStart = zStart;
  tBox.zPayload = zPayload;
  tBox.zEnd = (size_t)uEnd;
  return tBox;
}

static CHeifReader CreatePayloadReader(std::vector<uint8_t> const& vData, AL_THeifBox const& tBox)
{
  return CHeifReader(vData, tBox.zPayload, tBox.zEnd);
}

static bool IsAvifBrand(std::string const& sBrand)
{
  return sBrand == "avif" || sBrand == "avis";
}

static bool IsHeicBrand(std::string const& sBrand)
{
  return sBrand == "heic" || sBrand == "heix";
}

static void ParseFtyp(CHeifReader& tReader, AL_THeifBrands& tBrands)
{
  std::string sMajorBrand = tReader.ReadFourCC();
  tReader.ReadU32BE(); // minor_version

  tBrands.bAvifBrand = IsAvifBrand(sMajorBrand);
  tBrands.bHeicBrand = IsHeicBrand(sMajorBrand);

  // Remaining ftyp payload is the compatible_brands[] array, one FourCC per entry.
  while(tReader.GetBytesLeft() >= 4)
  {
    std::string sBrand = tReader.ReadFourCC();

    if(IsAvifBrand(sBrand))
      tBrands.bAvifBrand = true;

    if(IsHeicBrand(sBrand))
      tBrands.bHeicBrand = true;
  }
}

struct AL_THeifFullBox
{
  uint8_t uVersion = 0;
  uint32_t uFlags = 0;
};

struct AL_THeifItemExtent
{
  uint64_t uOffset = 0;
  uint64_t uLength = 0;
};

struct AL_THeifItemLocation
{
  uint32_t uId = 0;
  uint16_t uConstructionMethod = 0;
  uint16_t uDataReferenceIndex = 0;
  std::vector<AL_THeifItemExtent> vExtents;
};

struct AL_THeifItemProperty
{
  std::string sType;
  uint32_t uWidth = 0;
  uint32_t uHeight = 0;
  std::vector<uint8_t> vAv1ConfigObus;
  std::vector<uint8_t> vHevcParameterSets;
  uint8_t uHevcLengthSize = 0;
  bool bHasHevcVps = false;
  bool bHasHevcSps = false;
  bool bHasHevcPps = false;
};

struct AL_THeifDescription
{
  bool bAvifBrand = false;
  bool bHeicBrand = false;
  uint32_t uPrimaryItemId = 0;
  std::vector<uint8_t> vItemData;
  std::map<uint32_t, AL_THeifItemLocation> tLocations;
  std::map<uint32_t, std::string> tItemTypes;
  std::vector<AL_THeifItemProperty> vProperties;
  std::map<uint32_t, std::vector<uint32_t>> tPropertyAssociations;
};

struct AL_THeifGridInfo
{
  bool bValid = false;
  uint32_t uRows = 0;
  uint32_t uColumns = 0;
  uint32_t uWidth = 0;
  uint32_t uHeight = 0;
};

static AL_THeifFullBox ReadFullBox(CHeifReader& tReader)
{
  uint32_t uVersionFlags = tReader.ReadU32BE();
  AL_THeifFullBox tFullBox {};
  tFullBox.uVersion = (uint8_t)(uVersionFlags >> 24);
  tFullBox.uFlags = uVersionFlags & 0x00FFFFFF;
  return tFullBox;
}

static void ParsePitm(CHeifReader& tReader, AL_THeifDescription& tHeif)
{
  AL_THeifFullBox tFullBox = ReadFullBox(tReader);
  tHeif.uPrimaryItemId = (tFullBox.uVersion == 0) ? tReader.ReadU16BE() : tReader.ReadU32BE();
}

static void ParseIloc(CHeifReader& tReader, AL_THeifDescription& tHeif)
{
  AL_THeifFullBox tFullBox = ReadFullBox(tReader);
  uint8_t uSizes0 = tReader.ReadU8();
  uint8_t uSizes1 = tReader.ReadU8();
  size_t zOffsetSize = uSizes0 >> 4;
  size_t zLengthSize = uSizes0 & 0x0F;
  size_t zBaseOffsetSize = uSizes1 >> 4;
  size_t zIndexSize = (tFullBox.uVersion == 1 || tFullBox.uVersion == 2) ? (uSizes1 & 0x0F) : 0;
  uint32_t uItemCount = (tFullBox.uVersion < 2) ? tReader.ReadU16BE() : tReader.ReadU32BE();

  for(uint32_t i = 0; i < uItemCount; ++i)
  {
    AL_THeifItemLocation tLocation {};
    tLocation.uId = (tFullBox.uVersion < 2) ? tReader.ReadU16BE() : tReader.ReadU32BE();

    if(tFullBox.uVersion == 1 || tFullBox.uVersion == 2)
      tLocation.uConstructionMethod = tReader.ReadU16BE() & 0x000F;

    tLocation.uDataReferenceIndex = tReader.ReadU16BE();
    uint64_t uBaseOffset = tReader.ReadUIntBE(zBaseOffsetSize);
    uint16_t uExtentCount = tReader.ReadU16BE();

    for(uint16_t j = 0; j < uExtentCount; ++j)
    {
      if((tFullBox.uVersion == 1 || tFullBox.uVersion == 2) && zIndexSize > 0)
        tReader.ReadUIntBE(zIndexSize);

      uint64_t uExtentOffset = tReader.ReadUIntBE(zOffsetSize);
      uint64_t uExtentLength = tReader.ReadUIntBE(zLengthSize);
      AL_THeifItemExtent tExtent {};
      tExtent.uOffset = uBaseOffset + uExtentOffset;
      tExtent.uLength = uExtentLength;
      tLocation.vExtents.push_back(tExtent);
    }

    tHeif.tLocations[tLocation.uId] = tLocation;
  }
}

static void ParseInfe(CHeifReader& tReader, AL_THeifDescription& tHeif)
{
  AL_THeifFullBox tFullBox = ReadFullBox(tReader);

  if(tFullBox.uVersion < 2)
    return;

  uint32_t uItemId = (tFullBox.uVersion == 2) ? tReader.ReadU16BE() : tReader.ReadU32BE();
  tReader.ReadU16BE(); // item_protection_index
  tHeif.tItemTypes[uItemId] = tReader.ReadFourCC();
}

static void ParseIinf(CHeifReader& tReader, std::vector<uint8_t> const& vData, AL_THeifDescription& tHeif)
{
  AL_THeifFullBox tFullBox = ReadFullBox(tReader);
  uint32_t uEntryCount = (tFullBox.uVersion == 0) ? tReader.ReadU16BE() : tReader.ReadU32BE();

  for(uint32_t i = 0; i < uEntryCount && !tReader.IsEmpty(); ++i)
  {
    AL_THeifBox tBox = ReadBox(tReader);

    if(tBox.sType == "infe")
    {
      CHeifReader tPayload = CreatePayloadReader(vData, tBox);
      ParseInfe(tPayload, tHeif);
    }

    tReader.SetPosition(tBox.zEnd);
  }
}

static uint8_t GetHevcNalUnitType(std::vector<uint8_t> const& vNalUnit)
{
  if(vNalUnit.size() < 2)
    throw std::runtime_error("Truncated HEVC NAL unit");

  return (vNalUnit[0] >> 1) & 0x3F;
}

static void PushStartCode(std::vector<uint8_t>& vOut)
{
  vOut.insert(vOut.end(), { 0x00, 0x00, 0x00, 0x01 });
}

static void PushAnnexBNalUnit(std::vector<uint8_t>& vOut, std::vector<uint8_t> const& vNalUnit)
{
  PushStartCode(vOut);
  vOut.insert(vOut.end(), vNalUnit.begin(), vNalUnit.end());
}

static void ParseHvcC(CHeifReader& tReader, AL_THeifItemProperty& tProperty)
{
  if(tReader.GetBytesLeft() < 23)
    throw std::runtime_error("Truncated HEVC configuration property");

  tReader.ReadU8(); // configurationVersion
  tReader.Skip(20);
  tProperty.uHevcLengthSize = (tReader.ReadU8() & 0x03) + 1;
  uint8_t uNumArrays = tReader.ReadU8();

  for(uint8_t i = 0; i < uNumArrays; ++i)
  {
    uint8_t uArrayCompletenessAndType = tReader.ReadU8();
    (void)uArrayCompletenessAndType;
    uint16_t uNumNalus = tReader.ReadU16BE();

    for(uint16_t j = 0; j < uNumNalus; ++j)
    {
      uint16_t uNalUnitLength = tReader.ReadU16BE();
      std::vector<uint8_t> vNalUnit = tReader.ReadBytes(uNalUnitLength);
      uint8_t uNalUnitType = GetHevcNalUnitType(vNalUnit);

      if(uNalUnitType == AL_HEVC_NUT_VPS)
      {
        PushAnnexBNalUnit(tProperty.vHevcParameterSets, vNalUnit);
        tProperty.bHasHevcVps = true;
      }
      else if(uNalUnitType == AL_HEVC_NUT_SPS)
      {
        PushAnnexBNalUnit(tProperty.vHevcParameterSets, vNalUnit);
        tProperty.bHasHevcSps = true;
      }
      else if(uNalUnitType == AL_HEVC_NUT_PPS)
      {
        PushAnnexBNalUnit(tProperty.vHevcParameterSets, vNalUnit);
        tProperty.bHasHevcPps = true;
      }
    }
  }
}

static AL_THeifItemProperty ParseProperty(AL_THeifBox const& tBox, std::vector<uint8_t> const& vData)
{
  CHeifReader tReader = CreatePayloadReader(vData, tBox);
  AL_THeifItemProperty tProperty {};
  tProperty.sType = tBox.sType;

  if(tBox.sType == "hvcC")
    ParseHvcC(tReader, tProperty);

  if(tBox.sType == "ispe")
  {
    ReadFullBox(tReader);
    tProperty.uWidth = tReader.ReadU32BE();
    tProperty.uHeight = tReader.ReadU32BE();
  }

  return tProperty;
}

static void ParseIpco(CHeifReader& tReader, std::vector<uint8_t> const& vData, AL_THeifDescription& tHeif)
{
  while(!tReader.IsEmpty())
  {
    AL_THeifBox tBox = ReadBox(tReader);
    tHeif.vProperties.push_back(ParseProperty(tBox, vData));
    tReader.SetPosition(tBox.zEnd);
  }
}

static void ParseIpma(CHeifReader& tReader, AL_THeifDescription& tHeif)
{
  AL_THeifFullBox tFullBox = ReadFullBox(tReader);
  uint32_t uEntryCount = tReader.ReadU32BE();
  bool bLargePropertyIndex = (tFullBox.uFlags & 1) != 0;

  for(uint32_t i = 0; i < uEntryCount; ++i)
  {
    uint32_t uItemId = (tFullBox.uVersion < 1) ? tReader.ReadU16BE() : tReader.ReadU32BE();
    uint8_t uAssociationCount = tReader.ReadU8();

    for(uint8_t j = 0; j < uAssociationCount; ++j)
    {
      uint32_t uPropertyIndex;

      if(bLargePropertyIndex)
        uPropertyIndex = tReader.ReadU16BE() & 0x7FFF;
      else
        uPropertyIndex = tReader.ReadU8() & 0x7F;

      if(uPropertyIndex != 0)
        tHeif.tPropertyAssociations[uItemId].push_back(uPropertyIndex);
    }
  }
}

static void ParseIprp(CHeifReader& tReader, std::vector<uint8_t> const& vData, AL_THeifDescription& tHeif)
{
  while(!tReader.IsEmpty())
  {
    AL_THeifBox tBox = ReadBox(tReader);
    CHeifReader tPayload = CreatePayloadReader(vData, tBox);

    if(tBox.sType == "ipco")
      ParseIpco(tPayload, vData, tHeif);
    else if(tBox.sType == "ipma")
      ParseIpma(tPayload, tHeif);

    tReader.SetPosition(tBox.zEnd);
  }
}

static void ParseMeta(CHeifReader& tReader, std::vector<uint8_t> const& vData, AL_THeifDescription& tHeif)
{
  ReadFullBox(tReader);

  while(!tReader.IsEmpty())
  {
    AL_THeifBox tBox = ReadBox(tReader);
    CHeifReader tPayload = CreatePayloadReader(vData, tBox);

    if(tBox.sType == "pitm")
      ParsePitm(tPayload, tHeif);
    else if(tBox.sType == "iloc")
      ParseIloc(tPayload, tHeif);
    else if(tBox.sType == "iinf")
      ParseIinf(tPayload, vData, tHeif);
    else if(tBox.sType == "iprp")
      ParseIprp(tPayload, vData, tHeif);
    else if(tBox.sType == "idat")
    {
      std::vector<uint8_t> vItemData = tPayload.ReadRemainingBytes();
      tHeif.vItemData.insert(tHeif.vItemData.end(), vItemData.begin(), vItemData.end());
    }

    tReader.SetPosition(tBox.zEnd);
  }
}

static AL_THeifDescription ParseHeif(std::vector<uint8_t> const& vData)
{
  CHeifReader tReader(vData, 0, vData.size());
  AL_THeifDescription tHeif {};

  while(!tReader.IsEmpty())
  {
    AL_THeifBox tBox = ReadBox(tReader);
    CHeifReader tPayload = CreatePayloadReader(vData, tBox);

    if(tBox.sType == "ftyp")
    {
      AL_THeifBrands tBrands {};
      ParseFtyp(tPayload, tBrands);
      tHeif.bAvifBrand = tBrands.bAvifBrand;
      tHeif.bHeicBrand = tBrands.bHeicBrand;
    }
    else if(tBox.sType == "meta")
      ParseMeta(tPayload, vData, tHeif);

    tReader.SetPosition(tBox.zEnd);
  }

  return tHeif;
}

static AL_THeifItemProperty const* FindAssociatedProperty(AL_THeifDescription const& tHeif, uint32_t uItemId, std::string const& sType)
{
  auto tAssociations = tHeif.tPropertyAssociations.find(uItemId);

  if(tAssociations != tHeif.tPropertyAssociations.end())
  {
    for(uint32_t uPropertyIndex : tAssociations->second)
    {
      if(uPropertyIndex == 0 || uPropertyIndex > tHeif.vProperties.size())
        throw std::runtime_error("HEIF item property association is out of range");

      AL_THeifItemProperty const& tProperty = tHeif.vProperties[uPropertyIndex - 1];

      if(tProperty.sType == sType)
        return &tProperty;
    }
  }

  for(auto const& tProperty : tHeif.vProperties)
  {
    if(tProperty.sType == sType)
      return &tProperty;
  }

  return nullptr;
}

static std::vector<uint8_t> ExtractItemPayload(std::vector<uint8_t> const& vData, AL_THeifItemLocation const& tLocation)
{
  if(tLocation.uConstructionMethod != 0)
    throw std::runtime_error("Unsupported HEIF item construction method");

  if(tLocation.uDataReferenceIndex != 0)
    throw std::runtime_error("External HEIF item data references are not supported");

  if(tLocation.vExtents.empty())
    throw std::runtime_error("HEIF primary item has no extents");

  std::vector<uint8_t> vPayload;

  for(auto const& tExtent : tLocation.vExtents)
  {
    if(tExtent.uLength == 0)
      throw std::runtime_error("HEIF primary item has an unsupported zero-length extent");

    if(tExtent.uOffset > vData.size() || tExtent.uLength > vData.size() - tExtent.uOffset)
      throw std::runtime_error("HEIF primary item extent exceeds file size");

    auto tBegin = vData.begin() + static_cast<std::ptrdiff_t>(tExtent.uOffset);
    vPayload.insert(vPayload.end(), tBegin, tBegin + static_cast<std::ptrdiff_t>(tExtent.uLength));
  }

  return vPayload;
}

static std::vector<uint8_t> ExtractItemDataPayload(AL_THeifDescription const& tHeif, AL_THeifItemLocation const& tLocation)
{
  if(tLocation.uConstructionMethod != 1)
    return std::vector<uint8_t> {};

  if(tLocation.uDataReferenceIndex != 0 || tLocation.vExtents.empty())
    return std::vector<uint8_t> {};

  std::vector<uint8_t> vPayload;

  for(auto const& tExtent : tLocation.vExtents)
  {
    if(tExtent.uOffset > tHeif.vItemData.size())
      return std::vector<uint8_t> {};

    uint64_t uExtentLength = tExtent.uLength;

    if(uExtentLength == 0)
      uExtentLength = tHeif.vItemData.size() - tExtent.uOffset;

    if(uExtentLength > tHeif.vItemData.size() - tExtent.uOffset)
      return std::vector<uint8_t> {};

    auto tBegin = tHeif.vItemData.begin() + static_cast<std::ptrdiff_t>(tExtent.uOffset);
    vPayload.insert(vPayload.end(), tBegin, tBegin + static_cast<std::ptrdiff_t>(uExtentLength));
  }

  return vPayload;
}

static AL_THeifGridInfo ParseGridInfo(std::vector<uint8_t> const& vGridPayload)
{
  AL_THeifGridInfo tGrid {};

  if(vGridPayload.size() < 8)
    return tGrid;

  try
  {
    CHeifReader tReader(vGridPayload, 0, vGridPayload.size());
    uint8_t uVersion = tReader.ReadU8();

    if(uVersion != 0)
      return tGrid;

    uint8_t uFlags = tReader.ReadU8();
    tGrid.uRows = (uint32_t)tReader.ReadU8() + 1;
    tGrid.uColumns = (uint32_t)tReader.ReadU8() + 1;

    if((uFlags & 1) != 0)
    {
      if(tReader.GetBytesLeft() < 8)
        return AL_THeifGridInfo {};

      tGrid.uWidth = tReader.ReadU32BE();
      tGrid.uHeight = tReader.ReadU32BE();
    }
    else
    {
      if(tReader.GetBytesLeft() < 4)
        return AL_THeifGridInfo {};

      tGrid.uWidth = tReader.ReadU16BE();
      tGrid.uHeight = tReader.ReadU16BE();
    }

    tGrid.bValid = true;
  }
  catch(std::exception const &)
  {
    return AL_THeifGridInfo {};
  }

  return tGrid;
}

static AL_THeifGridInfo GetGridInfo(std::vector<uint8_t> const& vData, AL_THeifDescription const& tHeif, uint32_t uItemId)
{
  auto tLocation = tHeif.tLocations.find(uItemId);

  if(tLocation == tHeif.tLocations.end())
    return AL_THeifGridInfo {};

  try
  {
    if(tLocation->second.uConstructionMethod == 1)
      return ParseGridInfo(ExtractItemDataPayload(tHeif, tLocation->second));

    return ParseGridInfo(ExtractItemPayload(vData, tLocation->second));
  }
  catch(std::exception const &)
  {
    return AL_THeifGridInfo {};
  }
}

static void LogUnsupportedGrid(std::string const& sFormatName, std::vector<uint8_t> const& vData, AL_THeifDescription const& tHeif, uint32_t uItemId)
{
  AL_THeifGridInfo tGrid = GetGridInfo(vData, tHeif, uItemId);

  std::cout << sFormatName << " is a HEIF tile grid image";

  if(tGrid.bValid)
  {
    std::cout << " (" << tGrid.uColumns << "x" << tGrid.uRows << " tiles";

    if(tGrid.uWidth != 0 && tGrid.uHeight != 0)
      std::cout << ", " << tGrid.uWidth << "x" << tGrid.uHeight;

    std::cout << ")";
  }

  std::cout << "; tile composition is not supported." << std::endl;
}

static bool IsHevcImageItemType(std::string const& sType)
{
  return sType == "hvc1" || sType == "hev1";
}

static uint32_t SelectPrimaryHevcItem(AL_THeifDescription const& tHeif)
{
  if(tHeif.uPrimaryItemId != 0)
    return tHeif.uPrimaryItemId;

  uint32_t uItemId = 0;

  for(auto const& tItem : tHeif.tItemTypes)
  {
    if(!IsHevcImageItemType(tItem.second))
      continue;

    if(uItemId != 0)
      throw std::runtime_error("HEIC file has multiple HEVC items and no primary item");

    uItemId = tItem.first;
  }

  if(uItemId != 0)
    return uItemId;

  if(tHeif.tLocations.size() == 1)
    return tHeif.tLocations.begin()->first;

  throw std::runtime_error("HEIC primary item not found");
}

static void PushLengthPrefixedHevcPayload(std::vector<uint8_t>& vOut, std::vector<uint8_t> const& vItemPayload, uint8_t uLengthSize)
{
  if(uLengthSize == 0 || uLengthSize > 4)
    throw std::runtime_error("Unsupported HEIC HEVC NAL length size");

  size_t zPos = 0;

  while(zPos < vItemPayload.size())
  {
    if(vItemPayload.size() - zPos < uLengthSize)
      throw std::runtime_error("Truncated HEIC HEVC NAL length");

    uint64_t uNalUnitLength = 0;

    for(uint8_t i = 0; i < uLengthSize; ++i)
      uNalUnitLength = (uNalUnitLength << 8) | vItemPayload[zPos + i];

    zPos += uLengthSize;

    if(uNalUnitLength == 0)
      throw std::runtime_error("HEIC HEVC payload contains a zero-length NAL unit");

    if(uNalUnitLength > vItemPayload.size() - zPos)
      throw std::runtime_error("HEIC HEVC NAL unit exceeds item payload");

    PushStartCode(vOut);
    vOut.insert(vOut.end(),
                vItemPayload.begin() + static_cast<std::ptrdiff_t>(zPos),
                vItemPayload.begin() + static_cast<std::ptrdiff_t>(zPos + uNalUnitLength));
    zPos += static_cast<size_t>(uNalUnitLength);
  }
}

static std::vector<uint8_t> BuildHeicAnnexBStream(std::vector<uint8_t> const& vData)
{
  AL_THeifDescription tHeif = ParseHeif(vData);

  if(!tHeif.bHeicBrand)
    throw std::runtime_error("Input is not a HEIC file");

  uint32_t uPrimaryItemId = SelectPrimaryHevcItem(tHeif);
  auto tItemType = tHeif.tItemTypes.find(uPrimaryItemId);

  if(tItemType == tHeif.tItemTypes.end())
    throw std::runtime_error("HEIC primary item type not found");

  if(!IsHevcImageItemType(tItemType->second))
  {
    if(tItemType->second == "grid")
    {
      LogUnsupportedGrid("HEIC", vData, tHeif, uPrimaryItemId);
      throw std::runtime_error("Unsupported HEIC HEIF tile grid image");
    }

    throw std::runtime_error("Unsupported HEIC primary item type '" + tItemType->second + "'");
  }

  auto tLocation = tHeif.tLocations.find(uPrimaryItemId);

  if(tLocation == tHeif.tLocations.end())
    throw std::runtime_error("HEIC primary item location not found");

  AL_THeifItemProperty const* pHevcConfig = FindAssociatedProperty(tHeif, uPrimaryItemId, "hvcC");

  if(pHevcConfig == nullptr)
    throw std::runtime_error("HEIC HEVC configuration property not found");

  if(!pHevcConfig->bHasHevcVps || !pHevcConfig->bHasHevcSps || !pHevcConfig->bHasHevcPps)
    throw std::runtime_error("HEIC HEVC configuration property does not contain VPS/SPS/PPS");

  std::vector<uint8_t> vItemPayload = ExtractItemPayload(vData, tLocation->second);
  std::vector<uint8_t> vStream = pHevcConfig->vHevcParameterSets;
  PushLengthPrefixedHevcPayload(vStream, vItemPayload, pHevcConfig->uHevcLengthSize);
  return vStream;
}

}

HeifLoader::HeifLoader(AL_ECodec eCodec) :
  m_eCodec(eCodec)
{
}

void HeifLoader::PrepareStream(std::istream& ifFileStream)
{
  std::vector<uint8_t> vFileData((std::istreambuf_iterator<char>(ifFileStream)), std::istreambuf_iterator<char>());

  if(vFileData.empty())
    throw std::runtime_error("Empty HEIF input file");

  if(m_eCodec == AL_CODEC_HEVC)
  {
    m_Stream = BuildHeicAnnexBStream(vFileData);
    m_zReadOffset = 0;
    m_bPrepared = true;
    return;
  }

  throw std::runtime_error("Unknown HEIF format");
}

uint32_t HeifLoader::ReadStream(std::istream& ifFileStream, AL_TBuffer* pBufStream, uint8_t& uBufFlags)
{
  if(!m_bPrepared)
    PrepareStream(ifFileStream);

  if(m_zReadOffset >= m_Stream.size())
    return 0;

  uint8_t* pBuf = AL_Buffer_GetData(pBufStream);
  size_t zBytesToCopy = std::min((size_t)AL_Buffer_GetSize(pBufStream), m_Stream.size() - m_zReadOffset);
  std::memcpy(pBuf, m_Stream.data() + m_zReadOffset, zBytesToCopy);
  m_zReadOffset += zBytesToCopy;

  uBufFlags = AL_STREAM_BUF_FLAG_UNKNOWN;

  return (uint32_t)zBytesToCopy;
}
