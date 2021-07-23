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

Config parseCommandLine(int argc, char* argv[])
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
    PrintConfigFileUsage();
    exit(0);
  }

  return config;
}

void SafeMain(int argc, char* argv[])
{
  auto config = parseCommandLine(argc, argv);
  ConfigFile cfg {};
  cfg.strict_mode = config.strict_mode;
  ParseConfigFile(config.configFile, cfg, cerr, config.debug_token);

  if(config.dump_cfg)
  {
    throw std::runtime_error("introspection is not compiled in");
    exit(0);
  }

  if(config.show_cfg)
  {
    PrintConfig(cfg);
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

