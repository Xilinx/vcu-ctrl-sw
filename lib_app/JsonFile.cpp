/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#include "lib_app/JsonFile.h"

struct TJsonToken
{
  enum ETokenType
  {
    JSON_TOKEN_OPEN_ARRAY,
    JSON_TOKEN_CLOSE_ARRAY,
    JSON_TOKEN_OPEN_OBJECT,
    JSON_TOKEN_CLOSE_OBJECT,
    JSON_TOKEN_LIST_SEPARATOR,
    JSON_TOKEN_KEY_SEPARATOR,
    JSON_TOKEN_TRUE,
    JSON_TOKEN_FALSE,
    JSON_TOKEN_NUMBER,
    JSON_TOKEN_STRING,
    JSON_TOKEN_END_FILE,
    JSON_TOKEN_ERROR
  };

  ETokenType eType;
  std::string stringContent;
  int intContent;
};

bool TJsonValue::HasValue(int i)
{
  return eType == JSON_VALUE_ARRAY &&
         i < static_cast<int>(arrayValue.size());
}

bool TJsonValue::GetValue(int i, EValueType eValueType, TJsonValue*& pValue)
{
  if(!HasValue(i))
    return false;

  if(arrayValue[i].eType != eValueType)
    return false;

  pValue = &arrayValue[i];
  return true;
}

bool TJsonValue::HasValue(const std::string& sKey)
{
  return eType == JSON_VALUE_OBJECT &&
         objectValue.find(sKey) != objectValue.end();
}

bool TJsonValue::GetValue(const std::string& sKey, EValueType eValueType, TJsonValue*& pValue)
{
  if(!HasValue(sKey))
    return false;

  if(objectValue[sKey].eType != eValueType)
    return false;

  pValue = &objectValue[sKey];
  return true;
}

bool TJsonValue::GetValue(const std::string& sKey, bool& bValue)
{
  TJsonValue* pValue = nullptr;
  ;

  if(!GetValue(sKey, JSON_VALUE_BOOL, pValue))
    return false;

  bValue = pValue->boolValue;
  return true;
}

bool TJsonValue::GetValue(const std::string& sKey, int& iValue)
{
  TJsonValue* pValue = nullptr;
  ;

  if(!GetValue(sKey, JSON_VALUE_NUMBER, pValue))
    return false;

  iValue = pValue->intValue;
  return true;
}

CJsonReader::CJsonReader(const std::string& sFile) :
  sFile(sFile)
{
}

bool CJsonReader::Read(TJsonValue& tValue)
{
  std::ifstream ifs(sFile);

  if(!ifs.is_open())
    return false;

  InitTokenization();

  return ReadValue(ifs, tValue);
}

bool CJsonReader::ReadValue(std::ifstream& ifs, TJsonValue& tValue)
{
  TJsonToken tToken = ReadNextToken(ifs);
  switch(tToken.eType)
  {
  case TJsonToken::JSON_TOKEN_TRUE:
    tValue = TJsonValue(true);
    break;
  case TJsonToken::JSON_TOKEN_FALSE:
    tValue = TJsonValue(false);
    break;
  case TJsonToken::JSON_TOKEN_NUMBER:
    tValue = TJsonValue(tToken.intContent);
    break;
  case TJsonToken::JSON_TOKEN_STRING:
    tValue = TJsonValue(tToken.stringContent);
    break;
  case TJsonToken::JSON_TOKEN_OPEN_ARRAY:
    return ReadArray(ifs, tValue);
  case TJsonToken::JSON_TOKEN_OPEN_OBJECT:
    return ReadObject(ifs, tValue);
  default:
    return false;
  }

  return true;
}

bool CJsonReader::ReadArray(std::ifstream& ifs, TJsonValue& tValue)
{
  tValue.eType = TJsonValue::JSON_VALUE_ARRAY;
  tValue.arrayValue.clear();

  TJsonToken tToken = TJsonToken {
    TJsonToken::JSON_TOKEN_LIST_SEPARATOR, "", 0
  };

  while(tToken.eType == TJsonToken::JSON_TOKEN_LIST_SEPARATOR)
  {
    TJsonValue tSubValue;

    if(!ReadValue(ifs, tSubValue))
      return false;

    tValue.arrayValue.push_back(tSubValue);

    tToken = ReadNextToken(ifs);
  }

  return tToken.eType == TJsonToken::JSON_TOKEN_CLOSE_ARRAY;
}

bool CJsonReader::ReadObject(std::ifstream& ifs, TJsonValue& tValue)
{
  tValue.eType = TJsonValue::JSON_VALUE_OBJECT;
  tValue.objectValue.clear();

  TJsonToken tToken = TJsonToken {
    TJsonToken::JSON_TOKEN_LIST_SEPARATOR, "", 0
  };

  while(tToken.eType == TJsonToken::JSON_TOKEN_LIST_SEPARATOR)
  {
    tToken = ReadNextToken(ifs);

    if(tToken.eType != TJsonToken::JSON_TOKEN_STRING)
      return false;

    if(ReadNextToken(ifs).eType != TJsonToken::JSON_TOKEN_KEY_SEPARATOR)
      return false;

    TJsonValue tSubValue;

    if(!ReadValue(ifs, tSubValue))
      return false;

    tValue.objectValue[tToken.stringContent] = tSubValue;

    tToken = ReadNextToken(ifs);
  }

  return tToken.eType == TJsonToken::JSON_TOKEN_CLOSE_OBJECT;
}

TJsonToken CJsonReader::ReadNextToken(std::ifstream& ifs)
{
  static const TJsonToken ERROR_TOKEN = { TJsonToken::JSON_TOKEN_ERROR, "", 0 };

  while((cToken == ' ' || cToken == '\n' || cToken == '\t' || cToken == '\r') && ifs.get(cToken))
  {
  }

  ;

  if(ifs.eof())
    return TJsonToken {
             TJsonToken::JSON_TOKEN_END_FILE, "", 0
    };

  auto CheckToken = [&ifs](const std::string& sExpected)
                    {
                      char c;

                      for(auto cExpected : sExpected)
                        if(!ifs.get(c) || c != cExpected)
                          return false;

                      return true;
                    };

  TJsonToken tParsedToken = { TJsonToken::JSON_TOKEN_ERROR, "", 0 };
  switch(cToken)
  {
  case '[':
    tParsedToken.eType = TJsonToken::JSON_TOKEN_OPEN_ARRAY;
    break;
  case ']':
    tParsedToken.eType = TJsonToken::JSON_TOKEN_CLOSE_ARRAY;
    break;
  case '{':
    tParsedToken.eType = TJsonToken::JSON_TOKEN_OPEN_OBJECT;
    break;
  case '}':
    tParsedToken.eType = TJsonToken::JSON_TOKEN_CLOSE_OBJECT;
    break;
  case ',':
    tParsedToken.eType = TJsonToken::JSON_TOKEN_LIST_SEPARATOR;
    break;
  case ':':
    tParsedToken.eType = TJsonToken::JSON_TOKEN_KEY_SEPARATOR;
    break;
  case '"':
  {
    while(ifs.get(cToken) && (cToken != '"'))
      tParsedToken.stringContent += cToken;

    if(ifs.eof())
      return ERROR_TOKEN;
    tParsedToken.eType = TJsonToken::JSON_TOKEN_STRING;
    break;
  }
  case 't':
  {
    if(!CheckToken("rue"))
      return ERROR_TOKEN;
    tParsedToken.eType = TJsonToken::JSON_TOKEN_TRUE;
    break;
  }
  case 'f':
  {
    if(!CheckToken("alse"))
      return ERROR_TOKEN;
    tParsedToken.eType = TJsonToken::JSON_TOKEN_FALSE;
    break;
  }
  default:
  {
    bool bNegative;

    if(cToken == '-')
    {
      bNegative = true;
      tParsedToken.intContent = 0;
    }
    else if(cToken >= '0' && cToken <= '9')
    {
      bNegative = false;
      tParsedToken.intContent = cToken - '0';
    }
    else
      return ERROR_TOKEN;

    while(ifs.get(cToken) && (cToken >= '0' && cToken <= '9'))
      tParsedToken.intContent = tParsedToken.intContent * 10 + cToken - '0';

    if(bNegative)
      tParsedToken.intContent = -tParsedToken.intContent;

    tParsedToken.eType = TJsonToken::JSON_TOKEN_NUMBER;
    break;
  }
  }

  if(tParsedToken.eType != TJsonToken::JSON_TOKEN_NUMBER)
    ifs.get(cToken);

  return tParsedToken;
}

void CJsonReader::InitTokenization()
{
  cToken = ' ';
}

CJsonWriter::CJsonWriter(const std::string& sFile, bool bStreamMode) :
  sFile(sFile), bOneLinerStream(true)
{
  eState = bStreamMode ? JSON_STREAM_CLOSED : JSON_SINGLE_WRITE;
}

CJsonWriter::~CJsonWriter()
{
  if(eState == JSON_STREAM_OPENED)
  {
    std::ofstream ofs(sFile, ofstream::out | std::ofstream::app);
    CloseArray(ofs, bOneLinerStream, 0);
  }
}

bool CJsonWriter::Write(const TJsonValue& tValue)
{
  std::ios_base::openmode openMode = ofstream::out;

  if(eState == JSON_STREAM_OPENED)
    openMode |= std::ofstream::app;

  std::ofstream ofs(sFile, openMode);

  if(!ofs.is_open())
    return false;

  if(eState == JSON_STREAM_OPENED)
    WriteSeparator(ofs);
  else if(eState == JSON_STREAM_CLOSED)
  {
    OpenArray(ofs);
    bOneLinerStream = IsOneLiner(tValue);
    eState = JSON_STREAM_OPENED;
  }

  int iTab = 0;

  if(eState == JSON_STREAM_OPENED)
  {
    ArrayAlignment(ofs, eState == JSON_STREAM_CLOSED, bOneLinerStream, 0);
    iTab = 1;
  }

  WriteValue(ofs, tValue, iTab);

  return true;
}

void CJsonWriter::WriteValue(std::ofstream& ofs, const TJsonValue& tValue, int iTab)
{
  switch(tValue.eType)
  {
  case TJsonValue::JSON_VALUE_BOOL:
    ofs << (tValue.boolValue ? "true" : "false");
    break;
  case TJsonValue::JSON_VALUE_NUMBER:
    ofs << tValue.intValue;
    break;
  case TJsonValue::JSON_VALUE_STRING:
    ofs << "\"" << tValue.stringValue << "\"";
    break;
  case TJsonValue::JSON_VALUE_ARRAY:
    WriteArray(ofs, tValue, iTab);
    break;
  case TJsonValue::JSON_VALUE_OBJECT:
    WriteObject(ofs, tValue, iTab);
    break;
  }
}

void CJsonWriter::WriteArray(std::ofstream& ofs, const TJsonValue& tValue, int iTab)
{
  OpenArray(ofs);

  bool bOneLiner = tValue.arrayValue.size() == 0 || tValue.arrayValue[0].eType == TJsonValue::JSON_VALUE_BOOL ||
                   tValue.arrayValue[0].eType == TJsonValue::JSON_VALUE_NUMBER;

  for(size_t i = 0; i < tValue.arrayValue.size(); i++)
  {
    ArrayAlignment(ofs, i == 0, bOneLiner, iTab);

    WriteValue(ofs, tValue.arrayValue[i], iTab + 1);

    if(i != tValue.arrayValue.size() - 1)
      WriteSeparator(ofs);
  }

  CloseArray(ofs, bOneLiner, iTab);
}

void CJsonWriter::WriteObject(std::ofstream& ofs, const TJsonValue& tValue, int iTab)
{
  ofs << "{";

  size_t i = 0;

  for(auto const& tSubValue : tValue.objectValue)
  {
    NextLineAlignment(ofs, iTab + 1);

    ofs << "\"" << tSubValue.first << "\" : ";

    bool bIsOneLiner = IsOneLiner(tSubValue.second);

    if(!bIsOneLiner)
      NextLineAlignment(ofs, iTab + 1);

    WriteValue(ofs, tSubValue.second, iTab + 1);

    if(i != tValue.objectValue.size() - 1)
      WriteSeparator(ofs);

    i++;
  }

  NextLineAlignment(ofs, iTab);

  ofs << "}";
}

void CJsonWriter::OpenArray(std::ofstream& ofs)
{
  ofs << "[";
}

void CJsonWriter::CloseArray(std::ofstream& ofs, bool bOneLiner, int iTab)
{
  if(!bOneLiner)
    NextLineAlignment(ofs, iTab);

  ofs << "]";
}

void CJsonWriter::ArrayAlignment(std::ofstream& ofs, bool bFirstVal, bool bOneLiner, int iTab)
{
  if(bOneLiner)
  {
    if(!bFirstVal)
      ofs << " ";
  }
  else
    NextLineAlignment(ofs, iTab + 1);
}

bool CJsonWriter::IsOneLiner(const TJsonValue& tValue)
{
  switch(tValue.eType)
  {
  case TJsonValue::JSON_VALUE_OBJECT:
    return false;
  case TJsonValue::JSON_VALUE_ARRAY:
    return tValue.arrayValue.size() == 0 || tValue.arrayValue[0].eType == TJsonValue::JSON_VALUE_BOOL ||
           tValue.arrayValue[0].eType == TJsonValue::JSON_VALUE_NUMBER;
  default:
    return true;
  }
}

void CJsonWriter::NextLineAlignment(std::ofstream& ofs, int iTab)
{
  ofs << std::endl;

  for(int j = 0; j < iTab; j++)
    ofs << "\t";
}

void CJsonWriter::WriteSeparator(std::ofstream& ofs)
{
  ofs << ",";
}
