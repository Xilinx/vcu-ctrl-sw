/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

#include <functional>
#include <string>
#include <sstream>
#include <iomanip>
#include <queue>
#include <map>
#include <stdexcept>

struct CommandLineParser
{
  CommandLineParser() = default;
  CommandLineParser(std::function<void(std::string)> onOptionParsed_) : onOptionParsed{onOptionParsed_} {};

  struct Option
  {
    std::function<void(void)> parser;
    std::string desc;
  };

  void parse(int argc, char* argv[])
  {
    for(int i = 1; i < argc; ++i)
      words.push(argv[i]);

    while(!words.empty())
    {
      auto const word = popWord();

      auto i_func = options.find(word);

      onOptionParsed(word);

      if(i_func == options.end())
        throw std::runtime_error("Unknown option: '" + word + "', use -h to get help");

      i_func->second.parser();
    }
  }

  int popInt()
  {
    auto word = popWord();
    std::stringstream ss(word);
    ss.unsetf(std::ios::dec);
    ss.unsetf(std::ios::hex);

    int value;
    ss >> value;

    if(ss.fail() || ss.tellg() != std::streampos(-1))
      throw std::runtime_error("Expected an integer, got '" + word + "'");

    return value;
  }

  std::string popWord()
  {
    if(words.empty())
      throw std::runtime_error("Unexpected end of command line, use -h to get help");
    auto word = words.front();
    words.pop();
    return word;
  }

  void addOption(std::string name, std::function<void(void)> func, std::string desc_)
  {
    Option o;
    o.parser = func;
    o.desc = makeDescription(name, "", desc_);
    insertOption(name, o);
  }

  template<typename VariableType, typename Func>
  void setCustom_(std::string name, VariableType* value, Func customParser, std::string desc_)
  {
    Option o;
    o.parser = [=]()
               {
                 * value = (VariableType)customParser(popWord());
               };
    o.desc = makeDescription(name, "value", desc_);
    insertOption(name, o);
  }

  template<typename VariableType, typename ParserRetType>
  void addCustom(std::string name, VariableType* value, ParserRetType (* customParser)(std::string const &), std::string desc_)
  {
    setCustom_(name, value, customParser, desc_);
  }

  // add an option with a user-provided value parsing function
  template<typename VariableType, typename ParserRetType>
  void addCustom(std::string name, VariableType* value, std::function<ParserRetType(std::string const &)> customParser, std::string desc_)
  {
    setCustom_(name, value, customParser, desc_);
  }

  template<typename T>
  void addFlag(std::string name, T* flag, std::string desc_, T value = (T) 1)
  {
    Option o;
    o.desc = makeDescription(name, "", desc_);
    o.parser = [=]()
               {
                 * flag = value;
               };
    insertOption(name, o);
  }

  template<typename T>
  void addInt(std::string name, T* number, std::string desc_)
  {
    Option o;
    o.desc = makeDescription(name, "number", desc_);
    o.parser = [=]()
               {
                 * number = popInt();
               };
    insertOption(name, o);
  }

  void addString(std::string name, std::string* value, std::string desc_)
  {
    Option o;
    o.desc = makeDescription(name, "string", desc_);
    o.parser = [=]()
               {
                 * value = popWord();
               };
    insertOption(name, o);
  }

  std::map<std::string, Option> options;
  std::vector<std::string> displayOrder;

private:
  void insertOption(std::string name, Option o)
  {
    std::string item;
    std::stringstream ss(name);

    while(getline(ss, item, ','))
    {
      options[item] = o;
      options[name] = o;
    }

    displayOrder.push_back(name);
  }

  static std::string makeDescription(std::string word, std::string type, std::string desc)
  {
    std::string s;
    s += word;

    if(!type.empty())
      s += " <" + type + ">";

    std::stringstream ss;
    ss << std::setfill(' ') << std::setw(24) << std::left << s << " " << desc;

    return ss.str();
  }

  std::queue<std::string> words;
  std::function<void(std::string)> onOptionParsed = [](std::string) {};
};

