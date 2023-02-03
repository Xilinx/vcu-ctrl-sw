/******************************************************************************
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

#include "lib_app/Tokenizer.h"
#include <cctype>
#include <sstream>

using std::endl;
using std::string;

std::string escape(std::string const& text)
{
  std::stringstream ss;

  for(auto c : text)
  {
    /* this isn't expected to be exhaustive, it's just for printing */
    if(c == '\n')
      ss << "\\n";
    else if(c == '\r')
      ss << "\\r";
    else
      ss << c;
  }

  return ss.str();
}

static bool isComma(char c)
{
  return c == ',' || c == '.';
}

Token Tokenizer::getToken()
{
  Token token {};
  char c = getNextChar();
  int startPos = curPos - 1;
  token.text = string(1, c);
  token.position = getPosition();

  if(c == '[')
    token.type = TokenType::OpenBracket;
  else if(c == ']')
    token.type = TokenType::CloseBracket;
  else if(c == '\r')
    token = getToken();
  else if(c == '\n')
  {
    token.type = TokenType::EndOfLine;
    ++line;
    columnStartPos = curPos;
  }
  else if(c == '\0')
    token.type = TokenType::EndOfFile;
  else if(c == '=')
    token.type = TokenType::Equal;
  else if(c == ':')
  {
    token.type = TokenType::Equal;
    *logger << "WARNING: implicitly converting : to =" << endl;
  }
  else if(c == '+')
    token.type = TokenType::Plus;
  else if(c == '-')
    token.type = TokenType::Minus;
  else if(c == '/')
    token.type = TokenType::Divide;
  else if(c == '*')
    token.type = TokenType::Multiply;
  else if(c == '(')
    token.type = TokenType::OpenParen;
  else if(c == ')')
    token.type = TokenType::CloseParen;
  else if(c == '|')
    token.type = TokenType::Or;
  else if(std::isalpha(c))
    token = tokenizeIdentifier(token, startPos);
  else if(std::isdigit(c))
    token = tokenizeNumberToken(token, startPos);
  else
    token.type = TokenType::Undefined;

  *logger << toString(token.position) << " token type: " << toString(token.type) << ", token data: (" << escape(token.text) << ")" << endl;
  return token;
}

std::pair<int, int> Tokenizer::getPosition()
{
  return {
           line, curPos - columnStartPos
  };
}

Token & Tokenizer::tokenizeIdentifier(Token& token, int startPos)
{
  while(std::isalnum(getChar(curPos)) || getChar(curPos) == '_' || getChar(curPos) == '.')
    ++curPos;

  token.text = string(toParse.substr(startPos, curPos - startPos));
  token.type = TokenType::Identifier;
  return token;
}

Token & Tokenizer::tokenizeHexToken(Token& token, int startPos)
{
  while(std::isxdigit(getChar(curPos)))
    ++curPos;

  token.text = string(toParse.substr(startPos, curPos - startPos));
  token.type = TokenType::HexIntegral;
  return token;
}

Token & Tokenizer::tokenizeNumberToken(Token& token, int startPos)
{
  token.type = TokenType::Integral;

  if(toParse[startPos] == '0')
  {
    if(std::tolower(toParse[startPos + 1]) == 'x') /* hexadecimal */
    {
      ++curPos;
      return tokenizeHexToken(token, startPos);
    }
    else if(isComma(toParse[startPos + 1]))
    {
      /* we do not advance curPos to skip the integer portion parsing
       * (already parsed, it is 0) */
      token.type = TokenType::Float;
    }
    else if(std::isdigit(getChar(curPos))) /* no support for octal */
      token.type = TokenType::Undefined;
  }

  while(std::isdigit(getChar(curPos)))
    ++curPos;

  if(isComma(getChar(curPos)))
  {
    ++curPos;

    while(std::isdigit(getChar(curPos)))
      ++curPos;

    token.type = TokenType::Float;
  }

  token.text = string(toParse.substr(startPos, curPos - startPos));
  return token;
}

char Tokenizer::getChar(int pos)
{
  if(pos > (int)toParse.length())
    return '\0';
  return toParse[pos];
}

char Tokenizer::getNextChar()
{
  while(std::isblank(getChar(curPos)))
    ++curPos;

  if(getChar(curPos) == '#')
    while(getChar(curPos) != '\n')
      ++curPos;

  return getChar(curPos++);
}

