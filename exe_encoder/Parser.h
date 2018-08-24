/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include <deque>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

std::deque<Token> toReversePolish(std::deque<Token>& tokens);
std::string parseString(std::deque<Token>& tokens);
std::string parsePath(std::deque<Token>& tokens);
int parseEnum(std::deque<Token>& tokens, std::map<std::string, int> const& availableEnums);
std::map<std::string, int> createBoolEnums();
int parseBoolEnum(std::deque<Token>& tokens, std::map<std::string, int> boolEnums);
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

enum class Section
{
  Global,
  Unknown,
  Input,
  Output,
  Settings,
  Run,
  Trace,
  RateControl,
  Gop,
  Hardware,
};

static inline std::string toString(Section section)
{
  switch(section)
  {
  case Section::Input:
    return "INPUT";
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
  if(!arith.valid)
  {
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
  else
    return arith.value;
}

template<>
double get(ArithToken<double> const& arith);

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
static std::string describeEnum(std::map<std::string, T>& availableEnums)
{
  std::string desc = "";

  for(auto it : availableEnums)
    desc += it.first + ", ";

  return desc;
}

static inline std::string startValueDesc()
{
  return ", available values: ";
}

static inline std::string describeArithExpr()
{
  return "<Arithmetic expression>";
}

struct Callback
{
  std::function<void(std::deque<Token> &)> func;
  std::string desc;
};

struct ConfigParser
{
  Section curSection = Section::Global;

  void updateSection(std::string text);

  std::map<Section, std::map<std::string, Callback>> identifiers;

  template<typename T, typename U = long long int>
  void addArith(Section section, char const* name, T& t, std::string desc = "no description")
  {
    desc += startValueDesc() + describeArithExpr();
    identifiers[section][name] =
    {
      [&](std::deque<Token>& tokens)
      {
        t = parseArithmetic<U>(tokens);
      }, desc
    };
  };

  template<typename T, typename U = long long int, typename Func>
  void addArithFunc(Section section, char const* name, T& t, Func f, std::string desc = "no description")
  {
    desc += startValueDesc() + describeArithExpr();
    identifiers[section][name] =
    {
      [&](std::deque<Token>& tokens)
      {
        t = f(parseArithmetic<U>(tokens));
      }, desc
    };
  };

  /* Here we suppose type T and U are compatible for multiplication */
  template<typename T, typename U, typename V = long long int>
  void addArithMultipliedByConstant(Section section, char const* name, T& t, U const& u, std::string desc = "no description")
  {
    desc += startValueDesc() + describeArithExpr();
    identifiers[section][name] =
    {
      [&t, u](std::deque<Token>& tokens)
      {
        t = parseArithmetic<V>(tokens) * u;
      }, desc
    };
  }

  /* we have to remove type safety on enums because the file takes 16 seconds to compile otherwise ...*/
  template<typename T>
  void addEnum(Section section, char const* name, T& t, std::map<std::string, int> availableEnums, std::string desc = "no description")
  {
    desc += startValueDesc() + describeEnum(availableEnums);
    identifiers[section][name] =
    {
      [&t, availableEnums](std::deque<Token>& tokens)
      {
        t = static_cast<T>(parseEnum(tokens, availableEnums));
      }, desc
    };
  };

  template<typename T, typename U = long long int>
  void addArithOrEnum(Section section, char const* name, T& t, std::map<std::string, int> availableEnums, std::string desc = "no description")
  {
    desc += startValueDesc() + describeArithExpr() + " or " + describeEnum(availableEnums);
    identifiers[section][name] =
    {
      [&t, availableEnums](std::deque<Token>& tokens)
      {
        if(hasOnlyOneIdentifier(tokens))
        {
          t = static_cast<T>(parseEnum(tokens, availableEnums));
          return;
        }

        t = parseArithmetic<U>(tokens);
      }, desc
    };
  }

  template<typename T, typename Func, typename U = long long int>
  void addArithFuncOrEnum(Section section, char const* name, T& t, Func f, std::map<std::string, int> availableEnums, std::string desc = "no description")
  {
    desc += startValueDesc() + describeArithExpr() + " or " + describeEnum(availableEnums);
    identifiers[section][name] =
    {
      [&t, &f, availableEnums](std::deque<Token>& tokens)
      {
        if(hasOnlyOneIdentifier(tokens))
        {
          t = static_cast<T>(parseEnum(tokens, availableEnums));
          return;
        }

        t = static_cast<T>(f(parseArithmetic<U>(tokens)));
      }, desc
    };
  }

  template<typename T>
  void addBool(Section section, char const* name, T& t, std::string desc = "no description")
  {
    std::map<std::string, int> availableEnums = createBoolEnums();
    desc += startValueDesc() + describeEnum(availableEnums);
    identifiers[section][name] =
    {
      [&t, availableEnums](std::deque<Token>& tokens)
      {
        t = static_cast<T>(parseBoolEnum(tokens, availableEnums));
      }, desc
    };
  }

  template<typename T>
  void addFlag(Section section, char const* name, T& t, uint32_t uFlag, std::string desc = "no description")
  {
    std::map<std::string, int> availableEnums = createBoolEnums();
    desc += startValueDesc() + describeEnum(availableEnums);
    identifiers[section][name] =
    {
      [&t, name, uFlag, availableEnums](std::deque<Token>& tokens)
      {
        /* T needs to be able to translate to an uint32_t (like an enum) */
        bool hasFlag = parseBoolEnum(tokens, availableEnums);
        resetFlag(&t, uFlag);

        if(hasFlag)
          setFlag(&t, uFlag);
      }, desc
    };
  }

  void addString(Section section, char const* name, std::string& t, std::string desc = "no description")
  {
    identifiers[section][name] =
    {
      [&](std::deque<Token>& tokens)
      {
        t = parseString(tokens);
      }, desc
    };
  }

  void addPath(Section section, char const* name, std::string& t, std::string desc = "no description")
  {
    identifiers[section][name] =
    {
      [&](std::deque<Token>& tokens)
      {
        t = parsePath(tokens);
      }, desc
    };
  }

  template<typename Func>
  void addCustom(Section section, char const* name, Func func, std::string desc = "no description")
  {
    identifiers[section][name] = { func, desc };
  }

  std::string nearestMatch(std::string const& wrong);
  void parseIdentifiers(Token const& ident, std::deque<Token>& tokens);
};

