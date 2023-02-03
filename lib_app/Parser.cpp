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

#include "lib_app/Parser.h"
#include <limits>
#include <algorithm>

bool hasOnlyOneIdentifier(std::deque<Token>& tokens)
{
  return tokens.size() == 1 && tokens[0].type == TokenType::Identifier;
}

void adaptPath(std::string path)
{
  char pathSeparator = '/';
  char badPathSeparator = '\\';
#if defined(_WIN32)
  pathSeparator = '\\';
  badPathSeparator = '/';
#endif
  replace(path.begin(), path.end(), badPathSeparator, pathSeparator);
}

std::string parsePath(std::deque<Token>& tokens)
{
  char pathSeparator = '/';
  char badPathSeparator = '\\';
#if defined(_WIN32)
  pathSeparator = '\\';
  badPathSeparator = '/';
#endif
  std::stringstream acc {};

  for(auto& token : tokens)
  {
    if(token.text[0] == badPathSeparator)
      acc << pathSeparator;
    else
      acc << token.text;
  }

  return acc.str();
}

std::string parseString(std::deque<Token>& tokens)
{
  std::stringstream acc {};

  for(auto& token : tokens)
    acc << token.text;

  return acc.str();
}

int parseEnum(std::deque<Token>& tokens, std::map<std::string, EnumDescription<int>> const& availableEnums)
{
  int value {};
  bool lastIsOr = false;
  bool lastIsIdent = false;

  for(auto& token : tokens)
  {
    try
    {
      if(token.type == TokenType::Identifier)
      {
        if(lastIsOr)
          value = (int)((uint32_t)value | (uint32_t)availableEnums.at(token.text).name);
        else if(!lastIsIdent)
          value = availableEnums.at(token.text).name;
        else
          throw std::out_of_range("");
        lastIsOr = false;
        lastIsIdent = true;
      }
      else if(token.type == TokenType::Or)
        lastIsOr = true;
      else
        throw std::out_of_range("");
    }
    catch(std::out_of_range &)
    {
      std::stringstream acc {};

      for(auto const& e : availableEnums)
        acc << e.first << ", ";

      throw TokenError(token, "Failed to parse enum. Got (" + token.text + "), available values: " + acc.str());
    }
  }

  return value;
}

int parseBoolEnum(std::deque<Token>& tokens, std::map<std::string, EnumDescription<int>> boolEnums)
{
  /* support values 0 or 1 for boolean value */
  Token& token = tokens[0];

  if(isNumber(token))
    return get<int>(createArithToken(token, -1, false));

  return parseEnum(tokens, boolEnums);
}

std::map<std::string, EnumDescription<int>> createBoolEnums(std::vector<Codec> codecs)
{
  /* we need something that can take a bool */
  std::map<std::string, EnumDescription<int>> boolEnums;
  boolEnums["DISABLE"] = { false, "Disable", codecs };
  boolEnums["ENABLE"] = { true, "Enable", codecs };
  boolEnums["FALSE"] = { false, "Same as DISABLE", codecs };
  boolEnums["TRUE"] = { true, "Same as ENABLE", codecs };

  return boolEnums;
}

static void tryTransformUnaryOp(TokenType tokenType, TokenType unaryTokenType, Token& token, std::deque<Token>& treatedInputQueue)
{
  if(token.type == tokenType)
  {
    if(treatedInputQueue.empty())
      token.type = unaryTokenType;
    else
    {
      auto& lastToken = treatedInputQueue.back();

      if((lastToken.type == TokenType::OpenParen) || isOperator(lastToken))
        token.type = unaryTokenType;
    }
  }
}

std::string toIdentifierMultiplier(std::string identifier)
{
  if(identifier == "G")
    return std::string {
             "1000000000"
    };

  if(identifier == "Gi")
    return std::string {
             "1073741824"
    };

  if(identifier == "M")
    return std::string {
             "1000000"
    };

  if(identifier == "Mi")
    return std::string {
             "1048576"
    };

  if(identifier == "K")
    return std::string {
             "1000"
    };

  if(identifier == "Ki")
    return std::string {
             "1024"
    };
  return std::string {
           ""
  };
}

/* https://en.wikipedia.org/wiki/Shunting-yard_algorithm */
std::deque<Token> toReversePolish(std::deque<Token>& tokens)
{
  std::deque<Token> outputQueue;
  std::deque<Token> operatorStack;

  std::map<TokenType, int> precedences;
  precedences[TokenType::Plus] = 0;
  precedences[TokenType::Minus] = 0;
  precedences[TokenType::Multiply] = 5;
  precedences[TokenType::Divide] = 5;
  precedences[TokenType::UnaryMinus] = 7;
  precedences[TokenType::UnaryPlus] = 7;

  std::deque<Token> treatedInputQueue;

  while(!tokens.empty())
  {
    auto token = tokens.front();
    tokens.pop_front();

    if(token.type == TokenType::Identifier)
    {
      std::string multiplier {
        toIdentifierMultiplier(token.text)
      };

      if(multiplier.empty())
        throw TokenError(token, "arithmetic expression do not support identifiers others than 'K', 'Ki', 'M', 'Mi', 'G', 'Gi'");

      std::pair<int, int> position {
        -1, -1
      };
      Token multiply {
        TokenType::Multiply, std::string {
          "*"
        }, position
      };
      Token multiplierToken {
        TokenType::Integral, multiplier, position
      };

      if(treatedInputQueue.empty())
        throw TokenError(token, "arithmetic expression identifiers 'K', 'Ki', 'M', 'Mi', 'G', 'Gi' should be prefixed by a number");

      auto const& lastToken = treatedInputQueue.back();

      if(!isNumber(lastToken))
        throw TokenError(token, "arithmetic expression do not support identifiers others than 'K', 'Ki', 'M', 'Mi', 'G', 'Gi'");

      outputQueue.push_back(multiplierToken);
      outputQueue.push_back(multiply);
    }

    if(isNumber(token))
      outputQueue.push_back(token);

    if(isOperator(token))
    {
      tryTransformUnaryOp(TokenType::Minus, TokenType::UnaryMinus, token, treatedInputQueue);
      tryTransformUnaryOp(TokenType::Plus, TokenType::UnaryPlus, token, treatedInputQueue);

      while(!operatorStack.empty() && operatorStack.back().type != TokenType::OpenParen)
      {
        auto& topToken = operatorStack.back();

        if((isLeftAssociative(token) && precedences[token.type] <= precedences[topToken.type]) ||
           (isRightAssociative(token) && precedences[token.type] < precedences[topToken.type]))
        {
          operatorStack.pop_back();
          outputQueue.push_back(topToken);
        }
        else
        {
          break;
        }
      }

      operatorStack.push_back(token);
    }
    else if(token.type == TokenType::OpenParen)
      operatorStack.push_back(token);
    else if(token.type == TokenType::CloseParen)
    {
      Token operatorToken {
        TokenType::Undefined, "", token.position
      };

      while(!operatorStack.empty() && operatorToken.type != TokenType::OpenParen)
      {
        Token operatorToken = operatorStack.back();
        operatorStack.pop_back();

        if(operatorToken.type != TokenType::OpenParen)
          outputQueue.push_back(operatorToken);
      }

      if(!operatorStack.empty())
        operatorStack.pop_back();
    }
    else if(token.type == TokenType::Undefined)
    {
      throw TokenError(token, "invalid arithmetic expression");
    }

    treatedInputQueue.push_back(token);
  }

  while(!operatorStack.empty())
  {
    outputQueue.push_back(operatorStack.back());
    operatorStack.pop_back();
  }

  return outputQueue;
}

static Section toSection(std::string const& section)
{
  if(section == "INPUT")
    return Section::Input;
  else if(section == "DYNAMIC_INPUT")
    return Section::DynamicInput;
  else if(section == "OUTPUT")
    return Section::Output;
  else if(section == "SETTINGS")
    return Section::Settings;
  else if(section == "RUN")
    return Section::Run;
  else if(section == "TRACE")
    return Section::Trace;
  else if(section == "RATE_CONTROL")
    return Section::RateControl;
  else if(section == "GOP")
    return Section::Gop;
  else if(section == "HARDWARE")
    return Section::Hardware;
  else
    return Section::Unknown;
}

void ConfigParser::parseIdentifiers(Token const& ident, std::deque<Token>& tokens)
{
  try
  {
    identifiers.at(curSection).at(tolowerStr(ident.text)).func(tokens);
  }
  catch(std::out_of_range &)
  {
    throw TokenError(ident, "unknown identifier", nearestMatch(ident.text));
  }
  catch(std::runtime_error& e)
  {
    throw std::runtime_error("while parsing identifier " + ident.text + ": " + e.what());
  }
}

void ConfigParser::updateSection(std::string text)
{
  curSection = toSection(text);

  if(curSection == Section::Unknown)
    throw std::runtime_error("Section [" + text + "] doesn't exist");
}

static size_t levenshteinDistance(const std::string& s1, const std::string& s2)
{
  size_t const m = s1.size();
  size_t const n = s2.size();

  if(m == 0 || n == 0)
    return std::max(m, n);

  std::vector<size_t> costs {};

  for(size_t k = 0; k <= n; k++)
    costs.push_back(k);

  size_t i = 0;

  for(auto it1 = s1.begin(); it1 != s1.end(); ++it1, ++i)
  {
    costs[0] = i + 1;
    size_t corner = i;

    size_t j = 0;

    for(auto it2 = s2.begin(); it2 != s2.end(); ++it2, ++j)
    {
      size_t upper = costs[j + 1];

      if(*it1 == *it2)
        costs[j + 1] = corner;
      else
        costs[j + 1] = std::min(costs[j], std::min(upper, corner)) + 1;

      corner = upper;
    }
  }

  return costs[n];
}

std::string ConfigParser::nearestMatch(std::string const& wrong)
{
  std::string match = "";
  std::string otherSectionMatch = "";
  size_t minDistance = std::numeric_limits<std::size_t>::max();
  try
  {
    for(auto const& section : identifiers)
      for(auto const & right : identifiers.at(section.first))
        if(wrong == right.second.showName)
          otherSectionMatch = wrong + " but in section [" + toString(section.first) + "]";

    for(auto const& right : identifiers.at(curSection))
    {
      auto distance = levenshteinDistance(right.second.showName, wrong);

      if(right.second.isAdvancedFeature && !showAdvancedFeature)
        continue;

      if(distance < minDistance)
      {
        match = right.second.showName;
        minDistance = distance;
      }
    }
  }
  catch(std::out_of_range& e)
  {
    throw std::runtime_error("The section (" + toString(curSection) + ") doesn't have any identifiers");
  }

  if(otherSectionMatch != "")
    match = match + ", or " + otherSectionMatch;

  return match;
}

static void removeIdentifierInSectionCallbackInfoIfNoCodec(Callback& callback)
{
  auto& info = callback.info;
  info.erase(std::remove_if(info.begin(), info.end(), [&](CallbackInfo const& elem) -> bool
  {
    return elem.codecs.empty();
  }
                            )
             , info.end()
             );
}

static void removeIdentifierInSectionIfNoCallbackInfo(std::map<std::string, Callback>& identifiersInSection)
{
  for(auto it = identifiersInSection.begin(), it_next = it; it != identifiersInSection.end(); it = it_next)
  {
    ++it_next;

    if(it->second.info.empty())
      identifiersInSection.erase(it);
  }
}

static void removeSectionIfNoIdentifier(std::map<Section, std::map<std::string, Callback>>& identifiers)
{
  for(auto it = identifiers.begin(), it_next = it; it != identifiers.end(); it = it_next)
  {
    ++it_next;

    if(it->second.empty())
      identifiers.erase(it);
  }
}

static void removeIdentifierIfNoCodec(std::map<Section, std::map<std::string, Callback>>& identifiers)
{
  for(auto& identifiersInSection: identifiers)
  {
    for(auto& identifierInSection: identifiersInSection.second)
      removeIdentifierInSectionCallbackInfoIfNoCodec(identifierInSection.second);
  }

  for(auto& identifiersInSection: identifiers)
    removeIdentifierInSectionIfNoCallbackInfo(identifiersInSection.second);

  removeSectionIfNoIdentifier(identifiers);
}

void ConfigParser::removeIdentifierIf(std::vector<IdentifierValidation> conditions)
{
  for(auto const& condition: conditions)
  {
    switch(condition)
    {
    case IdentifierValidation::NO_CODEC:
    {
      removeIdentifierIfNoCodec(this->identifiers);
      break;
    }
    default: break;
    }
  }
}
