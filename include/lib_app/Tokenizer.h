// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
