/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2023 Allegro DVT2
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

#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

struct TJsonToken;

struct TJsonValue
{
  enum EValueType
  {
    JSON_VALUE_BOOL,
    JSON_VALUE_NUMBER,
    JSON_VALUE_STRING,
    JSON_VALUE_ARRAY,
    JSON_VALUE_OBJECT
  };

  EValueType eType {};
  bool boolValue {};
  int intValue {};
  std::string stringValue {};
  std::vector<TJsonValue> arrayValue {};
  std::map<std::string, TJsonValue> objectValue {};

  /* ------- Constructors -------  */
  TJsonValue() : TJsonValue(JSON_VALUE_BOOL) {};
  TJsonValue(EValueType eType) : eType(eType) {};
  TJsonValue(bool boolValue) : eType(JSON_VALUE_BOOL), boolValue(boolValue) {};
  TJsonValue(int intValue) : eType(JSON_VALUE_NUMBER), intValue(intValue) {};
  TJsonValue(std::string stringValue) : eType(JSON_VALUE_STRING), stringValue(stringValue) {};

  /* ------- Arrays Helpers -------  */
  bool HasValue(int i);
  bool GetValue(int i, EValueType eValueType, TJsonValue*& pValue);
  template<typename T>
  TJsonValue& PushBackValue(T tValue);

  /* ------- Object Helpers ------- */
  bool HasValue(const std::string& sKey);
  bool GetValue(const std::string& sKey, EValueType eValueType, TJsonValue*& pValue);
  bool GetValue(const std::string& sKey, bool& bValue);
  bool GetValue(const std::string& sKey, int& iValue);
  template<typename T>
  TJsonValue& AddValue(const std::string& sKey, T tValue);
};

class CJsonReader
{
public:
  CJsonReader(const std::string& sFile);
  bool Read(TJsonValue& tValue);

private:
  const std::string sFile;
  char cToken;

  bool ReadValue(std::ifstream& ifs, TJsonValue& tValue);
  bool ReadArray(std::ifstream& ifs, TJsonValue& tValue);
  bool ReadObject(std::ifstream& ifs, TJsonValue& tValue);
  void InitTokenization();
  TJsonToken ReadNextToken(std::ifstream& ifs);
};

class CJsonWriter
{
public:
  CJsonWriter(const std::string& sFile, bool bStreamMode = false);
  ~CJsonWriter();
  bool Write(const TJsonValue& tValue);

private:
  enum EJsonWriterState
  {
    JSON_SINGLE_WRITE,
    JSON_STREAM_CLOSED,
    JSON_STREAM_OPENED
  };
  const std::string sFile;
  EJsonWriterState eState;
  bool bOneLinerStream;

  void WriteValue(std::ofstream& ofs, const TJsonValue& tValue, int iTab);
  void WriteArray(std::ofstream& ofs, const TJsonValue& tValue, int iTab);
  void WriteObject(std::ofstream& ofs, const TJsonValue& tValue, int iTab);

  void OpenArray(std::ofstream& ofs);
  void CloseArray(std::ofstream& ofs, bool bOneLiner, int iTab);
  void ArrayAlignment(std::ofstream& ofs, bool bFirstVal, bool bOneLiner, int iTab);
  bool IsOneLiner(const TJsonValue& tValue);
  void NextLineAlignment(std::ofstream& ofs, int iTab);
  void WriteSeparator(std::ofstream& ofs);
};

template<typename T>
TJsonValue & TJsonValue::PushBackValue(T tValue)
{
  if(eType != JSON_VALUE_ARRAY)
    throw std::runtime_error("eType must be JSON_VALUE_ARRAY");
  arrayValue.push_back(TJsonValue(tValue));
  return arrayValue.back();
}

template<typename T>
TJsonValue & TJsonValue::AddValue(const std::string& sKey, T tValue)
{
  if(eType != JSON_VALUE_OBJECT)
    throw std::runtime_error("eType must be JSON_VALUE_OBJECT");
  objectValue[sKey] = TJsonValue(tValue);
  return objectValue[sKey];
}
