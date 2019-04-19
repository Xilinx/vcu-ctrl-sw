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
#include <cassert>
#include <stdexcept>
#include <iostream>

struct CommandLineParser
{
  CommandLineParser() = default;
  CommandLineParser(std::function<void(std::string)> onOptionParsed_) : onOptionParsed{onOptionParsed_} {};

  struct Option
  {
    std::function<void(std::string)> parser;
    std::string desc; /* full formated description with the name of the option */
    std::string desc_; /* description provided by the user verbatim */
  };

  void parse(int argc, char* argv[])
  {
    for(int i = 1; i < argc; ++i)
      words.push(argv[i]);

    while(!words.empty())
    {
      auto const word = popWord();

      if(isOption(word))
      {
        auto i_func = options.find(word);

        if(i_func == options.end())
          throw std::runtime_error("Unknown option: '" + word + "', use --help to get help");

        i_func->second.parser(word);
      }
      else /* positional argument */
      {
        if(positionals.empty())
          throw std::runtime_error("Too many positional arguments. Can't interpret '" + word + "', use -h to get help");
        auto& positional = positionals.front();
        positional.parser(word);
        positionals.pop_front();
      }
    }
  }

  template<typename T>
  T readVal(std::string word)
  {
    std::stringstream ss(word);
    ss.unsetf(std::ios::dec);
    ss.unsetf(std::ios::hex);

    T value;
    ss >> value;

    if(ss.fail() || ss.tellg() != std::streampos(-1))
      throw std::runtime_error("Expected an integer, got '" + word + "'");

    return value;
  }

  int readInt(std::string word)
  {
    return readVal<int>(word);
  }

  double readDouble(std::string word)
  {
    return readVal<double>(word);
  }

  int popInt()
  {
    auto word = popWord();
    return readInt(word);
  }

  double popDouble()
  {
    auto word = popWord();
    return readDouble(word);
  }

  std::string popWord()
  {
    if(words.empty())
      throw std::runtime_error("Unexpected end of command line, use -h to get help");
    auto word = words.front();
    words.pop();
    return word;
  }

  void addOption(std::string name, std::function<void(std::string)> func, std::string desc_)
  {
    Option o;
    o.parser = func;
    o.desc_ = desc_;
    o.desc = makeDescription(name, "", desc_);
    insertOption(name, o);
  }

  template<typename VariableType, typename Func>
  void setCustom_(std::string name, VariableType* value, Func customParser, std::string desc_)
  {
    Option o;
    o.parser = [=](std::string word)
               {
                 if(isOption(word))
                   *value = (VariableType)customParser(popWord());
                 else
                   *value = customParser(word);
               };
    o.desc_ = desc_;
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
    o.desc_ = desc_;
    o.desc = makeDescription(name, "", desc_);
    o.parser = [=](std::string word)
               {
                 assert(isOption(word));
                 * flag = value;
               };
    insertOption(name, o);
  }

  template<typename T>
  void addInt(std::string name, T* number, std::string desc_)
  {
    Option o;
    o.desc_ = desc_;
    o.desc = makeDescription(name, "number", desc_);
    o.parser = [=](std::string word)
               {
                 if(isOption(word))
                   *number = popInt();
                 else
                   *number = readInt(word);
               };
    insertOption(name, o);
  }

  template<typename T>
  void addDouble(std::string name, T* number, std::string desc_)
  {
    Option o;
    o.desc_ = desc_;
    o.desc = makeDescription(name, "number", desc_);
    o.parser = [=](std::string word)
               {
                 if(isOption(word))
                   *number = popDouble();
                 else
                   *number = readDouble(word);
               };
    insertOption(name, o);
  }

  void addString(std::string name, std::string* value, std::string desc_)
  {
    Option o;
    o.desc_ = desc_;
    o.desc = makeDescription(name, "string", desc_);
    o.parser = [=](std::string word)
               {
                 if(isOption(word))
                   *value = popWord();
                 else
                   *value = word;
               };
    insertOption(name, o);
  }

  std::map<std::string, Option> options;
  std::map<std::string, std::string> descs;
  std::vector<std::string> displayOrder;
  std::deque<Option> positionals;

  void usage() const
  {
    for(auto& command: displayOrder)
      std::cerr << "  " << descs.at(command) << std::endl;
  }

  /* not a complete escape function. This just validates our usecases. */
  std::string jsonEscape(std::string const& desc) const
  {
    std::stringstream ss {};

    for(auto& c : desc)
    {
      if(c == '"')
        ss << "\\";
      ss << c;
    }

    return ss.str();
  }

  void usageJson() const
  {
    bool first = true;
    std::cerr << "[";

    for(auto& o_: options)
    {
      auto name = o_.first;
      auto& o = o_.second;

      if(!first)
        std::cerr << ", " << std::endl;
      first = false;
      std::cerr << "{ \"name\":\"" << name << "\", \"desc\":\"" << jsonEscape(o.desc_) << "\" }";
    }

    std::cerr << "]" << std::endl;
  }

private:
  void insertOption(std::string name, Option o)
  {
    std::string item;
    std::stringstream ss(name);

    if(isOption(name))
    {
      while(getline(ss, item, ','))
        options[item] = o;
    }
    else
      positionals.push_back(o);

    descs[name] = o.desc;
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

  bool isOption(std::string word)
  {
    return word[0] == '-';
  }

  std::queue<std::string> words;
  std::function<void(std::string)> onOptionParsed = [](std::string) {};
};

