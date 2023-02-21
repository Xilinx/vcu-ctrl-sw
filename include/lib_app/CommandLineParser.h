// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>
#include <string>
#include <sstream>
#include <iomanip>
#include <queue>
#include <map>
#include <stdexcept>
#include <iostream>
#include "lib_rtos/types.h"

static inline bool ShouldShowAdvancedFeatures()
{
  return true;
}

struct CommandLineParser
{
public:
  CommandLineParser() = default;
  explicit CommandLineParser(bool showAdvancedFeatures_) : showAdvancedFeatures{showAdvancedFeatures_} {};
  explicit CommandLineParser(std::function<void(std::string)> onOptionParsed_, bool showAdvancedFeatures_) : onOptionParsed{onOptionParsed_}, showAdvancedFeatures{showAdvancedFeatures_} {};
  void setErrorOnUnknown(bool errorOut)
  {
    errorOnUnknown = errorOut;
  }

  struct Option
  {
    std::function<void(std::string)> parser;
    std::string type; /* type of the option */
    std::string desc; /* full formatted description with the name of the option */
    std::string desc_; /* description provided by the user verbatim */
    bool advancedFeature = false;
    bool repeat = false;
  };

  bool parse(int argc, char* argv[])
  {
    deprecatedWordsParsed.clear();

    for(int i = 1; i < argc; ++i)
      words.push(argv[i]);

    while(!words.empty())
    {
      auto const word = popWord();

      if(isOption(word))
      {
        auto i_func = options.find(word);

        if(i_func == options.end())
        {
          if(!errorOnUnknown)
            continue;

          throw std::runtime_error("Unknown option: '" + word + "', use --help to get help");
        }

        checkDeprecated(word);

        i_func->second.parser(word);
      }
      else /* positional argument */
      {
        if(positionals.empty())
        {
          if(!errorOnUnknown)
            continue;

          throw std::runtime_error("Too many positional arguments. Can't interpret '" + word + "', use -h to get help");
        }

        checkDeprecated(word);

        auto& positional = positionals.front();
        positional.parser(word);

        if(!positional.repeat)
          positionals.pop_front();
      }
    }

    return !deprecatedWordsParsed.empty();
  }

  void startSection(std::string const& sectionName)
  {
    curSection = sectionName;
  }

  void startDeprecatedSection()
  {
    startSection(DEPRECATED_SECTION);
  }

  template<typename T>
  T readVal(std::string word)
  {
    bool hex = (word[0] == '0' && word[1] == 'x');

    std::stringstream ss(hex ? word.substr(2) : word);
    ss.unsetf(std::ios::dec);
    ss.unsetf(std::ios::hex);

    if(hex)
      ss.setf(std::ios_base::hex, std::ios_base::basefield);

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

  uint32_t readUint(std::string word)
  {
    return readVal<uint32_t>(word);
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

  uint32_t popUint()
  {
    auto word = popWord();
    return readUint(word);
  }

  AL_PADDR popPAddr()
  {
    auto word = popWord();
    return readVal<AL_PADDR>(word);
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

  void addOption(std::string name, std::function<void(std::string)> func, std::string desc_, std::string const& type = "")
  {
    Option o;
    o.type = type;
    o.parser = func;
    o.desc_ = desc_;
    o.desc = makeDescription(name, type, desc_);
    insertOption(name, o);
  }

  template<typename VariableType, typename Func>
  void setCustom_(std::string name, VariableType* value, Func customParser, std::string desc_, std::string const& type)
  {
    Option o;
    o.parser = [=](std::string word)
               {
                 if(isOption(word))
                   *value = (VariableType)customParser(popWord());
                 else
                   *value = customParser(word);
               };
    o.type = type;
    o.desc_ = desc_;
    o.desc = makeDescription(name, type, desc_);
    insertOption(name, o);
  }

  template<typename VariableType, typename ParserRetType>
  void addCustom(std::string name, VariableType* value, ParserRetType (* customParser)(std::string const &), std::string desc_, std::string const& type = "")
  {
    setCustom_(name, value, customParser, desc_, type);
  }

  // add an option with a user-provided value parsing function
  template<typename VariableType, typename ParserRetType>
  void addCustom(std::string name, VariableType* value, std::function<ParserRetType(std::string const &)> customParser, std::string desc_, std::string const& type = "")
  {
    setCustom_(name, value, customParser, desc_, type);
  }

  template<typename T>
  void addFlag(std::string name, T* flag, std::string desc_, T value = (T) 1)
  {
    Option o;
    o.type = "";
    o.desc_ = desc_;
    o.desc = makeDescription(name, o.type, desc_);
    o.parser = [=](std::string word)
               {
                 if(!isOption(word))
                   throw std::runtime_error("word must be an option");
                 *flag = value;
               };
    insertOption(name, o);
  }

  template<typename T>
  void addInt(std::string name, T* number, std::string desc_)
  {
    Option o;
    o.type = "number";
    o.desc_ = desc_;
    o.desc = makeDescription(name, o.type, desc_);
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
  void addUint(std::string name, T* number, std::string desc_)
  {
    Option o;
    o.type = "number";
    o.desc_ = desc_;
    o.desc = makeDescription(name, o.type, desc_);
    o.parser = [=](std::string word)
               {
                 if(isOption(word))
                   *number = popUint();
                 else
                   *number = readUint(word);
               };
    insertOption(name, o);
  }

  void addPAddr(std::string name, AL_PADDR* addr, std::string desc_)
  {
    Option o;
    o.type = "address";
    o.desc_ = desc_;
    o.desc = makeDescription(name, o.type, desc_);
    o.parser = [=](std::string word)
               {
                 if(isOption(word))
                   *addr = popPAddr();
                 else
                   *addr = readVal<AL_PADDR>(word);
               };
    insertOption(name, o);
  }

  template<typename T>
  void addDouble(std::string name, T* number, std::string desc_)
  {
    Option o;
    o.type = "number";
    o.desc_ = desc_;
    o.desc = makeDescription(name, o.type, desc_);
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
    o.type = "string";
    o.desc_ = desc_;
    o.desc = makeDescription(name, o.type, desc_);
    o.parser = [=](std::string word)
               {
                 if(isOption(word))
                   *value = popWord();
                 else
                   *value = word;
               };
    insertOption(name, o);
  }

  void addStringVector(std::string name, std::vector<std::string>* vec, std::string desc_)
  {
    Option o;
    o.type = "string (+)";
    o.desc_ = desc_;
    o.desc = makeDescription(name, o.type, desc_);
    o.parser = [=](std::string word)
               {
                 if(isOption(word))
                   vec->push_back(popWord());
                 else
                   vec->push_back(word);
               };
    o.repeat = true;
    insertOption(name, o);
  }

  std::map<std::string, Option> options;
  std::map<std::string, std::string> descs;
  std::map<std::string, std::string> sections;
  std::map<std::string, std::string> types;
  std::string curSection = "";
  std::vector<std::string> displayOrder;
  std::deque<Option> positionals;

  void usage() const
  {
    std::string section {};

    for(auto& command: displayOrder)
    {
      if(sections.at(command) != section)
      {
        section = sections.at(command);

        if(section == "")
          throw std::runtime_error("If a section has been started, you can't go back to the general section. (For display reasons)");
        std::cout << std::endl;
        std::cout << "  [" << section << "]" << std::endl;
      }

      std::string item;
      std::stringstream ss(command);

      if(getline(ss, item, ',') && options.at(item).advancedFeature && !showAdvancedFeatures)
        continue;

      std::cout << "  " << descs.at(command) << std::endl;
    }
  }

  void usageDeprecated() const
  {
    std::cout << "Warning, usage of the following deprecated options:" << std::endl;

    for(auto& w : deprecatedWordsParsed)
      std::cout << "  " << descs.at(w) << std::endl;

    std::cout << std::endl;
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
    std::cout << "[" << std::endl;

    for(auto& o_: options)
    {
      auto name = o_.first;
      auto& o = o_.second;
      auto section = sections.at(name);

      if(o.advancedFeature && !showAdvancedFeatures)
        continue;

      if(!first)
        std::cout << ", " << std::endl;
      first = false;
      std::cout << "{" << std::endl;
      std::cout << "\"name\":\"" << name << "\"," << std::endl;
      std::cout << "\"type\":\"" << jsonEscape(o.type) << "\"," << std::endl;
      std::cout << "\"description\":\"" << jsonEscape(o.desc_) << "\"," << std::endl;
      std::cout << "\"section\":\"" << section << "\"" << std::endl;
      std::cout << "}";
    }

    std::cout << std::endl << "]" << std::endl;
  }

  // You can also use "X,Y,Z unrelated option to set multiple option as advanced at the same time"
  void setAdvanced(std::string name)
  {
    std::string item;
    std::stringstream ss(name);

    if(isOption(name))
    {
      int num_opt = 0;

      while(getline(ss, item, ','))
      {
        ++num_opt;
        options[item].advancedFeature = true;
      }

      if(num_opt == 0)
        throw std::runtime_error("You need to call setAdvanced on an existing option");
    }
    else
      throw std::runtime_error("You can't set a positional argument as advanced as it would change the place of all the other positional arguments.");
  }

private:
  void insertOption(std::string name, Option o)
  {
    std::string item;
    std::stringstream ss(name);

    if(isOption(name))
    {
      while(getline(ss, item, ','))
      {
        options[item] = o;
        sections[item] = curSection;
      }
    }
    else
      positionals.push_back(o);

    descs[name] = o.desc;
    types[name] = o.type;
    sections[name] = curSection;
    displayOrder.push_back(name);
  }

  std::string makeDescription(std::string word, std::string type, std::string desc)
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

  bool checkDeprecated(std::string const& word)
  {
    auto curSec = sections.find(word);

    if(curSec != sections.end() && curSec->second == DEPRECATED_SECTION)
    {
      deprecatedWordsParsed.push_back(word);
      return true;
    }
    return false;
  }

  std::queue<std::string> words;
  std::function<void(std::string)> onOptionParsed = [](std::string) {};
  std::vector<std::string> deprecatedWordsParsed;
  bool errorOnUnknown = true;
  bool showAdvancedFeatures = true;
  static const std::string DEPRECATED_SECTION;
};

const std::string CommandLineParser::DEPRECATED_SECTION = "Deprecated";
