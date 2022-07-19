/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

#pragma once
#include "Tokenizer.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <iomanip>

std::deque<Token> toReversePolish(std::deque<Token>& tokens);
std::string parseString(std::deque<Token>& tokens);
void adaptPath(std::string path);
std::string parsePath(std::deque<Token>& tokens);

enum class Codec
{
  Hevc,
  Avc,
  Vvc,
  Vp9,
  Av1,
  Jpeg,
};

static inline std::string toString(Codec codec)
{
  switch(codec)
  {
  case Codec::Hevc: return "HEVC";
  case Codec::Avc: return "AVC";
  case Codec::Vvc: return "VVC";
  case Codec::Vp9: return "VP9";
  case Codec::Av1: return "AV1";
  case Codec::Jpeg: return "JPEG";
  }

  return "Unknown";
}

static inline std::vector<Codec> filterCodecs(std::vector<Codec> inputCodecs)
{
  std::vector<Codec> filteredCodecs {};

  for(auto const& codec: inputCodecs)
  {

    if(codec == Codec::Hevc)
      filteredCodecs.push_back(Codec::Hevc);

    if(codec == Codec::Avc)
      filteredCodecs.push_back(Codec::Avc);
  }

  return filteredCodecs;
}

static inline std::vector<Codec> isOnlyCodec(Codec codec)
{
  return filterCodecs({ codec });
}

static inline std::vector<Codec> aomCodecs()
{
  return filterCodecs({ Codec::Vp9, Codec::Av1 });
}

static inline std::vector<Codec> ituCodecs()
{
  return filterCodecs({ Codec::Hevc, Codec::Avc, Codec::Vvc });
}

static inline std::vector<Codec> aomituCodecs()
{
  std::vector<Codec> aomituCodecs {};

  for(auto const& codec : aomCodecs())
    aomituCodecs.push_back(codec);

  for(auto const& codec : ituCodecs())
    aomituCodecs.push_back(codec);

  return aomituCodecs;
}

static inline std::vector<Codec> allCodecs()
{
  std::vector<Codec> allCodecs {
    aomituCodecs()
  };

  for(auto const& codec : filterCodecs({ Codec::Jpeg }))
    allCodecs.push_back(codec);

  return allCodecs;
}

template<typename T>
struct EnumDescription
{
  T name;
  std::string description;
  std::vector<Codec> codecs;
};

template<class T>
static inline void SetEnumDescr(std::map<std::string, EnumDescription<T>>& enumDescr, std::string const& key, T const& name, std::string const& descr, std::vector<Codec> const& codecs)
{
  auto it = enumDescr.find(key);

  if(it != enumDescr.end())
  {
    assert(enumDescr[key].name == name);
    enumDescr[key].codecs.insert(enumDescr[key].codecs.end(), codecs.begin(), codecs.end());
  }
  else
    enumDescr[key] = { name, descr, codecs };
}

int parseEnum(std::deque<Token>& tokens, std::map<std::string, EnumDescription<int>> const& availableEnums);
std::map<std::string, EnumDescription<int>> createBoolEnums(std::vector<Codec> codecs);
int parseBoolEnum(std::deque<Token>& tokens, std::map<std::string, EnumDescription<int>> boolEnums);
bool hasOnlyOneIdentifier(std::deque<Token>& tokens);

struct TokenError : public std::runtime_error
{
  TokenError(Token const& token, char const* msg) : std::runtime_error(std::string(msg) + " at " + toString(token.position) + " on token (" + escape(token.text) + ") parsed as (" + toString(token.type) + ")")
  {
  }

  TokenError(Token const& token, std::string const& msg) : std::runtime_error(msg + " at " + toString(token.position) + " on token (" + escape(token.text) + ") parsed as (" + toString(token.type) + ")")
  {
  }

  TokenError(Token const& token, char const* msg, std::string const& match) : std::runtime_error(std::string(msg) + " at " + toString(token.position) + " on token (" + escape(token.text) + ") parsed as (" + toString(token.type) + "), closest match is (" + match + ")")
  {
  }
};

enum class ParameterType
{
  Array,
  String,
  ArithExpr,
};

static inline std::string toString(ParameterType type)
{
  switch(type)
  {
  case ParameterType::Array: return "array";
  case ParameterType::String: return "string";
  case ParameterType::ArithExpr: return "arithmetic expression";
  }

  return "Unknown";
}

template<typename T>
struct ArithInfo
{
  ArithInfo(std::vector<Codec> codecs_ = aomituCodecs(), T min_ = std::numeric_limits<T>::min(), T max_ = std::numeric_limits<T>::max()) :
    codecs{codecs_},
    min{min_},
    max{max_}
  {
    if(!codecs_.empty())
      assert(min < max);
  }

  std::vector<Codec> codecs;
  T min;
  T max;
};

template<typename T>
struct ArithInfoList
{
  ArithInfoList(std::vector<Codec> codecs_, std::vector<T>& availableValuesList_) :
    codecs{codecs_},
    availableValuesList{availableValuesList_}
  {}

  std::vector<Codec> codecs;
  std::vector<T> availableValuesList;
};

static inline std::vector<std::string> noNote()
{
  return {};
}

enum class Section
{
  Input,
  DynamicInput,
  Output,
  Gop,
  RateControl,
  Settings,
  Run,
  Global,
  Hardware,
  Trace,
  Unknown,
};

static inline std::string toString(Section section)
{
  switch(section)
  {
  case Section::Input:
    return "INPUT";
  case Section::DynamicInput:
    return "DYNAMIC_INPUT";
  case Section::Output:
    return "OUTPUT";
  case Section::Settings:
    return "SETTINGS";
  case Section::Run:
    return "RUN";
  case Section::Trace:
    return "TRACE";
  case Section::RateControl:
    return "RATE_CONTROL";
  case Section::Gop:
    return "GOP";
  case Section::Hardware:
    return "HARDWARE";
  case Section::Global:
    return "GLOBAL";
  default:
    return "Unknown";
  }
}

static inline bool isNumber(Token const& token)
{
  return token.type == TokenType::Integral || token.type == TokenType::HexIntegral || token.type == TokenType::Float;
}

static inline bool isOperator(Token const& token)
{
  return token.type == TokenType::Plus || token.type == TokenType::Minus || token.type == TokenType::Multiply || token.type == TokenType::Divide || token.type == TokenType::UnaryMinus || token.type == TokenType::UnaryPlus;
}

static inline bool isLeftAssociative(Token const &)
{
  return true; // all our token are left associative
}

static inline bool isRightAssociative(Token const &)
{
  return true; // all our token are right associative
}

template<typename T>
static T applyOperator(Token const& token, std::vector<T>& values)
{
  switch(token.type)
  {
  case TokenType::Plus:
    return values[1] + values[0];
  case TokenType::Minus:
    return values[1] - values[0];
  case TokenType::UnaryMinus:
    return -values[0];
  case TokenType::UnaryPlus:
    return values[0];
  case TokenType::Multiply:
    return values[1] * values[0];
  case TokenType::Divide:

    if(values[0] == 0)
      throw TokenError(token, "bad arith: division by 0");
    return values[1] / values[0];
  default:
    throw TokenError(token, "not an arith expression");
  }
}

static inline int getNumberOperand(Token const& token)
{
  return (token.type == TokenType::UnaryMinus || token.type == TokenType::UnaryPlus) ? 1 : 2;
}

template<typename T>
struct ArithToken
{
  ArithToken(Token const& token, T value, bool valid) : token{token}, value{value}, valid{valid} {}
  Token token;
  T value;
  bool valid;
};

template<typename T>
static ArithToken<T> createArithToken(Token const& token, T value, bool valid)
{
  return ArithToken<T>(token, value, valid);
}

template<typename T>
static T get(ArithToken<T> const& arith)
{
  if(arith.valid)
    return arith.value;
  Token const& token = arith.token;
  int base = 10;

  if(token.type == TokenType::HexIntegral)
    base = 16;
  else if(token.type == TokenType::Integral)
    base = 10;
  else
    throw TokenError(token, "not an integer");
  return std::strtol(token.text.c_str(), NULL, base);
}

template<typename T>
static T reversePolishEval(std::deque<Token> tokens)
{
  std::deque<ArithToken<T>> stack;

  Token concealToken {
    TokenType::Undefined, "", {
      -1, -1
    }
  };

  for(Token token : tokens)
  {
    concealToken = token;

    if(!isOperator(token))
    {
      stack.push_back(createArithToken(token, T {}, false));
    }
    else
    {
      std::vector<T> values;

      for(int i = 0; i < getNumberOperand(token); ++i)
      {
        if(!stack.empty())
        {
          values.push_back(get(stack.back()));
          stack.pop_back();
        }
        else
          throw TokenError(token, "bad arith expression: stack empty");
      }

      T result = applyOperator(token, values);
      stack.push_back(createArithToken(Token { TokenType::Integral, std::to_string(result), token.position }, result, true));
    }
  }

  if(stack.size() != 1)
  {
    if(!stack.empty())
      throw TokenError(stack.back().token, "bad arith expression");
    else
      throw TokenError(concealToken, "bad arith expression");
  }
  return get(stack.back());
}

template<typename T>
static T parseArithmetic(std::deque<Token>& tokens)
{
  return reversePolishEval<T>(toReversePolish(tokens));
}

template<typename T>
std::vector<T> parseArray(std::deque<Token>& tokens, size_t expectedSize)
{
  std::vector<T> array;
  int sign = 1;

  for(auto& token : tokens)
  {
    if(token.type == TokenType::Minus)
      sign = -1;

    else if(isNumber(token))
    {
      array.push_back(sign * get<T>(createArithToken(token, T {}, false)));
      sign = 1;
    }
    else
      throw TokenError(token, "Array doesn't support special character " + toString(token.type));
  }

  if(array.size() != expectedSize)
    throw std::runtime_error("Array doesn't fit expected size, require " + std::to_string(expectedSize) + " elements");
  return array;
}

template<typename T>
static void resetFlag(T* bitfield, uint32_t uFlag)
{
  *bitfield = (T)((uint32_t)*bitfield & ~uFlag);
}

template<typename T>
static void setFlag(T* bitfield, uint32_t uFlag)
{
  *bitfield = (T)((uint32_t)*bitfield | uFlag);
}

template<typename T>
static bool getFlag(T* bitfield, uint32_t uFlag)
{
  return (uint32_t)*bitfield & uFlag;
}

template<typename T>
static T getFlags(T* bitfield, uint32_t uMask)
{
  return (T)((uint32_t)*bitfield & uMask);
}

template<typename T>
static std::string describeEnum(std::map<std::string, T> const& availableEnums)
{
  std::string description = "";
  bool first = true;

  for(auto it : availableEnums)
  {
    if(!first)
      description += ", ";
    description += it.first;
    first = false;
  }

  return description;
}

template<typename T>
static std::map<std::string, std::string> descriptionEnum(std::map<std::string, EnumDescription<T>> const& enumsDescription)
{
  std::map<std::string, std::string> description {};

  for(auto const& enumDescription: enumsDescription)
    description.insert(std::pair<std::string, std::string>(enumDescription.first, enumDescription.second.description));

  return description;
}

template<typename T>
std::string getDefaultEnumValue(T const& t, std::map<std::string, EnumDescription<int>> const& availableEnums)
{
  for(auto& it : availableEnums)
  {
    if(it.second.name == (int)t)
      return it.first;
  }

  return "unknown";
}

template<typename T>
std::string getDefaultArrayValue(T* t, int arraySize, int rescale = 1)
{
  std::string s = "";

  for(auto i = 0; i < (int)arraySize; ++i)
  {
    s += std::to_string(t[i] / rescale);

    if(i != arraySize - 1)
      s += " ";
  }

  return s;
}

struct SeeAlsoInfo
{
  Section section;
  std::string name;
};

struct CallbackInfo
{
  std::vector<Codec> codecs;
  std::string available;
  std::map<std::string, std::string> availableDescription;
};

struct Callback
{
  Callback() = default;
  ~Callback() = default;
  Callback(Callback const &) = default;
  Callback & operator = (Callback const &) = default;
  Callback(Callback &&) = default;
  Callback & operator = (Callback &&) = default;

  Callback(std::function<void(std::deque<Token> &)> const& func_, std::string const& showName_, std::string const& description_, std::vector<ParameterType> const& types_, std::function<std::string()> defaultValue_, std::vector<CallbackInfo> const& info_) : // , std::vector<std::string> const& notes_
    func{func_},
    showName{showName_},
    description{description_},
    types{types_},
    defaultValue{defaultValue_},
    info{info_}
  {}

  std::function<void(std::deque<Token> &)> func;
  std::string showName;
  std::string description;
  std::vector<ParameterType> types;
  std::function<std::string()> defaultValue;
  std::vector<std::string> notes;
  std::vector<CallbackInfo> info;
  std::vector<SeeAlsoInfo> seealso;
  bool isAdvancedFeature = false;
};

static int safeToLower(int c)
{
  if(c < 0 && c != EOF)
    throw std::runtime_error("Character is not ASCII");
  return ::tolower(c);
}

template<typename T>
static std::string setPrecision(T value, int precision = 2)
{
  std::stringstream ss;
  ss << std::setprecision(precision) << std::fixed << value;
  return ss.str();
}

template<typename T>
static std::string describeArith(T min, T max)
{
  std::stringstream ss;
  ss << "[";
  ss << setPrecision(min);
  ss << ", ";
  ss << setPrecision(max);
  ss << "]";
  return ss.str();
}

template<typename T>
static std::string describeArithList(std::vector<T> List, int precision = 2)
{
  std::stringstream ss;
  ss << "{";
  std::string separator = "";

  for(auto it = List.begin(); it != List.end(); ++it)
  {
    ss << (separator + setPrecision(*it, precision));
    separator = ", ";
  }

  ss << "}";
  return ss.str();
}

template<typename T>
static std::vector<CallbackInfo> toCallbackInfo(std::vector<ArithInfo<T>> const& arithInfo)
{
  std::vector<CallbackInfo> callbackInfo {};

  for(auto const& info_per_codecs : arithInfo)
  {
    if(!info_per_codecs.codecs.empty())
      callbackInfo.push_back({ info_per_codecs.codecs, describeArith(info_per_codecs.min, info_per_codecs.max), {}
                             });
  }

  return callbackInfo;
}

template<typename T>
static std::vector<CallbackInfo> toCallbackInfo(std::vector<ArithInfoList<T>> const& arithInfo, int precision = 2)
{
  std::vector<CallbackInfo> callbackInfo {};

  for(auto const& info_per_codecs : arithInfo)
  {
    if(!info_per_codecs.codecs.empty())
      callbackInfo.push_back({ info_per_codecs.codecs, describeArithList(info_per_codecs.availableValuesList, precision), {}
                             });
  }

  return callbackInfo;
}

template<typename T>
static std::vector<CallbackInfo> toCallbackInfo(std::map<std::string, EnumDescription<T>> const& enumInfo)
{
  std::vector<CallbackInfo> callbackInfo {};

  for(auto const& enums : enumInfo)
  {
    auto sameCodecs = [&](CallbackInfo const& info) -> bool {
                        if(info.codecs.size() != enums.second.codecs.size())
                          return false;
                        return std::is_permutation(info.codecs.begin(), info.codecs.end(), enums.second.codecs.begin());
                      };

    if(!enums.second.codecs.empty())
    {
      for(auto& info: callbackInfo)
      {
        if(sameCodecs(info))
        {
          info.available += ", " + enums.first;
          info.availableDescription.insert(std::pair<std::string, std::string>(enums.first, enums.second.description));
        }
      }

      if(!(std::find_if(callbackInfo.begin(), callbackInfo.end(), sameCodecs) != callbackInfo.end()))
      {
        std::map<std::string, std::string> availableDescription;
        availableDescription.insert(std::pair<std::string, std::string>(enums.first, enums.second.description));
        callbackInfo.push_back({ enums.second.codecs, enums.first, availableDescription });
      }
    }
  }

  return callbackInfo;
}

static inline std::vector<CallbackInfo> mergeCallbackInfo(std::vector<CallbackInfo> const& arithCallbackInfo, std::vector<CallbackInfo> const& enumCallbackInfo)
{
  std::vector<CallbackInfo> callbackInfo {
    enumCallbackInfo
  };

  for(auto const& arith : arithCallbackInfo)
  {
    auto sameCodecs = [&](CallbackInfo const& info) -> bool {
                        if(info.codecs.size() != arith.codecs.size())
                          return false;
                        return std::is_permutation(info.codecs.begin(), info.codecs.end(), arith.codecs.begin());
                      };

    if(!arith.codecs.empty())
    {
      for(auto& info: callbackInfo)
      {
        if(sameCodecs(info))
        {
          info.available += " or " + arith.available;

          for(auto const& availableDescription: arith.availableDescription)
            info.availableDescription.insert(availableDescription);
        }
      }

      if(!(std::find_if(callbackInfo.begin(), callbackInfo.end(), sameCodecs) != callbackInfo.end()))
        callbackInfo.push_back({ arith.codecs, arith.available, arith.availableDescription });
    }
  }

  return callbackInfo;
}

static inline std::string tolowerStr(std::string name)
{
  std::transform(name.begin(), name.end(), name.begin(), safeToLower);
  return name;
}

struct ConfigParser
{
  Section curSection = Section::Global;

  void updateSection(std::string text);

  std::map<Section, std::map<std::string, Callback>> identifiers;
  enum class IdentifierValidation
  {
    NO_CODEC,
  };
  bool showAdvancedFeature = true;

  void removeIdentifierIf(std::vector<IdentifierValidation> conditions);
  void setAdvanced(Section section, char const* name)
  {
    identifiers[section][tolowerStr(name)].isAdvancedFeature = true;
  }

  void addNote(Section section, char const* name, std::string note)
  {
    identifiers[section][tolowerStr(name)].notes.push_back(note);
  }

  void addSeeAlso(Section section, char const* name, SeeAlsoInfo seealso)
  {
    identifiers[section][tolowerStr(name)].seealso.push_back(seealso);
  }

  template<typename T, typename U = long long int>
  void addArith(Section section, char const* name, T& t, std::string description, std::vector<ArithInfo<U>> info)
  {
    std::vector<ParameterType> types {
      ParameterType::ArithExpr
    };

    std::vector<CallbackInfo> callbackInfo {
      toCallbackInfo(info)
    };

    identifiers[section][tolowerStr(name)] =
    {
      [&](std::deque<Token>& tokens)
      {
        t = parseArithmetic<U>(tokens);
      }, name, description, types,
      [&t]() -> std::string
      {
        return std::to_string(t);
      }, callbackInfo
    };
  }

  template<typename T, typename U>
  void addArithList(Section section, char const* name, T& t, std::string description, std::vector<ArithInfoList<U>> info, int precision = 2)
  {
    std::vector<ParameterType> types {
      ParameterType::ArithExpr
    };

    std::vector<CallbackInfo> callbackInfo {
      toCallbackInfo(info, precision)
    };

    identifiers[section][tolowerStr(name)] =
    {
      [&](std::deque<Token>& tokens)
      {
        t = parseArithmetic<U>(tokens);
      }, name, description, types,
      [&t]() -> std::string
      {
        return std::to_string(t);
      }, callbackInfo
    };
  }

  template<typename T, typename U = long long int, typename Func, typename Func2>
  void addArithFuncList(Section section, char const* name, T& t, Func f, Func2 m, std::string description, std::vector<ArithInfoList<U>> info, int precision = 2)
  {
    std::vector<ParameterType> types {
      ParameterType::ArithExpr
    };
    std::vector<CallbackInfo> callbackInfo {
      toCallbackInfo(info, precision)
    };

    identifiers[section][tolowerStr(name)] =
    {
      [&](std::deque<Token>& tokens)
      {
        t = f(parseArithmetic<U>(tokens));
      }, name, description, types,
      [&t, m]() -> std::string
      {
        return std::to_string(m(t));
      }, callbackInfo
    };
  }

  template<typename T, typename U = long long int, typename Func, typename Func2>
  void addArithFunc(Section section, char const* name, T& t, Func f, Func2 m, std::string description, std::vector<ArithInfo<U>> info)
  {
    std::vector<ParameterType> types {
      ParameterType::ArithExpr
    };
    std::vector<CallbackInfo> callbackInfo {
      toCallbackInfo(info)
    };

    identifiers[section][tolowerStr(name)] =
    {
      [&](std::deque<Token>& tokens)
      {
        t = f(parseArithmetic<U>(tokens));
      }, name, description, types,
      [&t, m]() -> std::string
      {
        return std::to_string(m(t));
      }, callbackInfo
    };
  }

  /* Here we suppose type T and U are compatible for multiplication */
  template<typename T, typename U, typename V = long long int>
  void addArithMultipliedByConstant(Section section, char const* name, T& t, U const& u, std::string description, std::vector<ArithInfo<V>> info)
  {
    std::vector<ParameterType> types {
      ParameterType::ArithExpr
    };
    std::vector<CallbackInfo> callbackInfo {
      toCallbackInfo(info)
    };

    identifiers[section][tolowerStr(name)] =
    {
      [&t, u](std::deque<Token>& tokens)
      {
        t = parseArithmetic<V>(tokens) * u;
      }, name, description, types,
      [&t, u]() -> std::string
      {
        return std::to_string(t / u);
      }, callbackInfo
    };
  }

  /* we have to remove type safety on enums because the file takes 16 seconds to compile otherwise ...*/
  template<typename T>
  void addEnum(Section section, char const* name, T& t, std::map<std::string, EnumDescription<int>> availableEnums, std::string description)
  {
    std::vector<ParameterType> types {
      ParameterType::String
    };

    identifiers[section][tolowerStr(name)] =
    {
      [&t, availableEnums](std::deque<Token>& tokens)
      {
        t = static_cast<T>(parseEnum(tokens, availableEnums));
      }, name, description, types,
      [&t, availableEnums]() -> std::string
      {
        return getDefaultEnumValue(t, availableEnums);
      }, toCallbackInfo(availableEnums)
    };
  }

  template<typename T, typename U = long long int>
  void addArithOrEnum(Section section, char const* name, T& t, std::map<std::string, EnumDescription<int>> availableEnums, std::string description, std::vector<ArithInfo<U>> info)
  {
    std::vector<ParameterType> types {
      ParameterType::String, ParameterType::ArithExpr
    };

    identifiers[section][tolowerStr(name)] =
    {
      [&t, availableEnums](std::deque<Token>& tokens)
      {
        if(hasOnlyOneIdentifier(tokens))
        {
          t = static_cast<T>(parseEnum(tokens, availableEnums));
          return;
        }

        t = parseArithmetic<U>(tokens);
      }, name, description, types,
      [&t, availableEnums]() -> std::string
      {
        for(auto& it : availableEnums)
        {
          if(it.second.name == (int)t)
            return it.first;
        }

        return std::to_string(t);
      }, mergeCallbackInfo(toCallbackInfo(info), toCallbackInfo(availableEnums))
    };
  }

  template<typename T, typename U = long long int, typename Func, typename Func2>
  void addArithFuncOrEnum(Section section, char const* name, T& t, Func f, Func2 m, std::map<std::string, EnumDescription<int>> availableEnums, std::string description, std::vector<ArithInfo<U>> info)
  {
    std::vector<ParameterType> types {
      ParameterType::String, ParameterType::ArithExpr
    };

    identifiers[section][tolowerStr(name)] =
    {
      [&t, &f, availableEnums](std::deque<Token>& tokens)
      {
        if(hasOnlyOneIdentifier(tokens))
        {
          t = static_cast<T>(parseEnum(tokens, availableEnums));
          return;
        }

        t = static_cast<T>(f(parseArithmetic<U>(tokens)));
      }, name, description, types,
      [&t, m, availableEnums]() -> std::string
      {
        for(auto& it : availableEnums)
        {
          if(it.second.name == (int)t)
            return it.first;
        }

        return std::to_string(m(t));
      }, mergeCallbackInfo(toCallbackInfo(info), toCallbackInfo(availableEnums))
    };
  }

  template<typename T>
  void addBool(Section section, char const* name, T& t, std::string description, std::vector<Codec> codecs)
  {
    std::vector<ParameterType> types {
      ParameterType::String
    };
    auto availableEnums = createBoolEnums(codecs);

    identifiers[section][tolowerStr(name)] =
    {
      [&t, availableEnums](std::deque<Token>& tokens)
      {
        t = static_cast<T>(parseBoolEnum(tokens, availableEnums));
      }, name, description, types,
      [&t, availableEnums]() -> std::string
      {
        return getDefaultEnumValue(t, availableEnums);
      }, toCallbackInfo(availableEnums)
    };
  }

  template<typename T>
  void addFlagsFromEnum(Section section, char const* name, T& t, std::map<std::string, EnumDescription<int>> availableEnums, uint32_t uMask, std::string description)
  {
    std::vector<ParameterType> types {
      ParameterType::String
    };

    identifiers[section][tolowerStr(name)] =
    {
      [&t, uMask, availableEnums](std::deque<Token>& tokens)
      {
        /* T needs to be able to translate to an uint32_t (like an enum) */
        uint32_t uFlags = static_cast<T>(parseEnum(tokens, availableEnums));
        resetFlag(&t, uMask);
        setFlag(&t, uFlags);
      }, name, description, types,
      [&t, uMask, availableEnums]() -> std::string
      {
        T Flags = getFlags(&t, uMask);
        return getDefaultEnumValue(Flags, availableEnums);
      }, toCallbackInfo(availableEnums)
    };
  }

  template<typename T>
  void addFlag(Section section, char const* name, T& t, uint32_t uFlag, std::string description, std::vector<Codec> codecs)
  {
    std::vector<ParameterType> types {
      ParameterType::String
    };
    auto availableEnums = createBoolEnums(codecs);

    identifiers[section][tolowerStr(name)] =
    {
      [&t, uFlag, availableEnums](std::deque<Token>& tokens)
      {
        /* T needs to be able to translate to an uint32_t (like an enum) */
        bool hasFlag = parseBoolEnum(tokens, availableEnums);
        resetFlag(&t, uFlag);

        if(hasFlag)
          setFlag(&t, uFlag);
      }, name, description, types,
      [&t, uFlag, availableEnums]() -> std::string
      {
        bool hasFlag = getFlag(&t, uFlag);

        if(hasFlag)
          return "TRUE";

        return "FALSE";
      }, toCallbackInfo(availableEnums)
    };
  }

  void addString(Section section, char const* name, std::string& t, std::string description, std::vector<Codec> codecs)
  {
    std::vector<ParameterType> types {
      ParameterType::String
    };
    std::string available {};
    std::map<std::string, std::string> availableDescription {};
    identifiers[section][tolowerStr(name)] =
    {
      [&](std::deque<Token>& tokens)
      {
        t = parseString(tokens);
      }, name, description, types,
      [&t]() -> std::string
      {
        return t;
      },
      {
        { codecs, available, availableDescription }
      }
    };
  }

  void addPath(Section section, char const* name, std::string& t, std::string description, std::vector<Codec> codecs)
  {
    std::vector<ParameterType> types {
      ParameterType::String
    };
    std::string available {
      "<path>"
    };
    std::map<std::string, std::string> availableDescription {};
    identifiers[section][tolowerStr(name)] =
    {
      [&](std::deque<Token>& tokens)
      {
        t = parsePath(tokens);
      }, name, description, types,
      [&]() -> std::string
      {
        return t;
      },
      {
        { codecs, available, availableDescription }
      }
    };
  }

  template<typename T, size_t arraySize, typename U = long long int>
  void addArray(Section section, char const* name, T(&t)[arraySize], std::string description, std::vector<ArithInfo<U>> info)
  {
    std::vector<ParameterType> types {
      ParameterType::Array
    };
    std::vector<CallbackInfo> callbackInfo {
      toCallbackInfo(info)
    };
    identifiers[section][tolowerStr(name)] =
    {
      [&](std::deque<Token>& tokens)
      {
        auto array = parseArray<T>(tokens, arraySize);

        for(auto i = 0; i < (int)arraySize; ++i)
          t[i] = array[i];
      }, name, description, types,
      [&t]() -> std::string
      {
        return getDefaultArrayValue(t, arraySize);
      }, callbackInfo
    };
  }

  template<typename Func, typename Func2>
  void addCustom(Section section, char const* name, Func func, Func2 defaultVal, std::string description, std::vector<ParameterType> types, std::vector<CallbackInfo> info)
  {
    identifiers[section][tolowerStr(name)] =
    {
      func,
      name,
      description,
      types,
      defaultVal,
      info
    };
  }

  std::string nearestMatch(std::string const& wrong);
  void parseIdentifiers(Token const& ident, std::deque<Token>& tokens);
};
