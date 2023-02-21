// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "CfgParser.h"
#include <iostream>
#include <lib_app/CommandLineParser.h>
#include <stdexcept>

using std::cout;
using std::endl;

using std::cerr;
using std::cout;
using std::string;

struct Config
{
  string configFile = "";
  bool strict_mode = true;
  bool debug_token = false;
  bool dump_cfg = false;
  bool show_cfg = false;
};

static void Usage(CommandLineParser const& opt, char* ExeName)
{
  cout << "Usage: " << ExeName << " -cfg <configfile> [options]" << endl;
  cout << "Options:" << endl;

  opt.usage();

  cout << "Examples:" << endl;
  cout << "  " << ExeName << " -cfg test/config/encode_simple.cfg" << endl;
}

Config parseCommandLine(int argc, char* argv[], CfgParser& cfgParser)
{
  bool help = false;
  bool help_cfg = false;
  Config config {};
  auto opt = CommandLineParser();
  opt.addFlag("--help,-h", &help, "Show this help");
  opt.addFlag("--help-cfg", &help_cfg, "Show all options available in the cfg");
  opt.addFlag("--dump-cfg", &config.dump_cfg, "Dump the cfg structure");
  opt.addFlag("--show-cfg", &config.show_cfg, "show the cfg values");
  opt.addString("-cfg", &config.configFile, "configuration file to parse");
  opt.addFlag("--relax", &config.strict_mode, "errors become warnings", false);
  opt.addFlag("--debug", &config.debug_token, "debug token parsing");
  opt.parse(argc, argv);

  if(help)
  {
    Usage(opt, argv[0]);
    exit(0);
  }

  if(help_cfg)
  {
    cfgParser.PrintConfigFileUsage();
    exit(0);
  }

  return config;
}

void SafeMain(int argc, char* argv[])
{
  CfgParser cfgParser;
  auto config = parseCommandLine(argc, argv, cfgParser);
  ConfigFile cfg {};
  cfg.strict_mode = config.strict_mode;
  cfgParser.ParseConfigFile(config.configFile, cfg, cerr, config.debug_token);
  cfgParser.PostParsingConfiguration(cfg, cerr);

  if(config.dump_cfg)
  {
    throw std::runtime_error("introspection is not compiled in");
    exit(0);
  }

  if(config.show_cfg)
  {
    cfgParser.PrintConfig(cfg);
    exit(0);
  }
}

int main(int argc, char* argv[])
{
  try
  {
    SafeMain(argc, argv);
    return 0;
  }
  catch(std::runtime_error& e)
  {
    cerr << endl << "Error: " << e.what() << endl;
    return 1;
  }
}

