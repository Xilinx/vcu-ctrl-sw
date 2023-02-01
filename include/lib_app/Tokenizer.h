/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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

#pragma once
#include <string>
#include <utility>

enum class TokenType
{
  Undefined,
  Identifier,
  OpenBracket,
  CloseBracket,
  OpenParen,
  CloseParen,
  Equal,
  EndOfLine,
  EndOfFile,
  HexIntegral,
  Integral,
  Float,
  Plus,
  Minus,
  UnaryMinus,
  UnaryPlus,
  Divide,
  Multiply,
  Or,
};

struct Token
{
  Token() : type{TokenType::Undefined}, text{""}, position{0, 0}
  {
  }

  Token(TokenType type, std::string text, std::pair<int, int> position) : type{type}, text{text}, position{position}
  {
  }

  TokenType type;
  std::string text;
  std::pair<int, int> position;
};

struct Tokenizer
{
  Tokenizer(const std::string& toParse, std::ostream* logger) : toParse{toParse}, logger{logger}
  {
  }

  Token getToken();

private:
  std::pair<int, int> getPosition();
  Token& tokenizeIdentifier(Token& token, int startPos);
  Token& tokenizeHexToken(Token& token, int startPos);
  Token& tokenizeNumberToken(Token& token, int startPos);
  char getChar(int pos);
  char getNextChar();

  std::string toParse;
  int line = 1;
  std::ostream* logger;
  int columnStartPos = 0;
  int curPos = 0;
};

static inline std::string toString(TokenType type)
{
  switch(type)
  {
  case TokenType::Identifier:
    return "Identifier";
  case TokenType::OpenBracket:
    return "OpenBracket";
  case TokenType::CloseBracket:
    return "CloseBracket";
  case TokenType::EndOfLine:
    return "EndOfLine";
  case TokenType::EndOfFile:
    return "EndOfFile";
  case TokenType::Equal:
    return "Equal";
  case TokenType::Integral:
    return "Integral";
  case TokenType::HexIntegral:
    return "HexIntegral";
  case TokenType::Float:
    return "Float";
  case TokenType::Plus:
    return "Plus";
  case TokenType::UnaryPlus:
    return "UnaryPlus";
  case TokenType::Minus:
    return "Minus";
  case TokenType::UnaryMinus:
    return "UnaryMinus";
  case TokenType::Divide:
    return "Divide";
  case TokenType::Multiply:
    return "Multiply";
  case TokenType::Or:
    return "Or";
  default:
    return "Undefined";
  }
}

static inline std::string toString(std::pair<int, int> p)
{
  return "(l:" + std::to_string(p.first) + ", c:" + std::to_string(p.second) + ")";
}

std::string escape(std::string const& text);
