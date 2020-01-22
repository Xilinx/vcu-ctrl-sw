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

