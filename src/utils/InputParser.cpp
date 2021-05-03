#include "InputParser.hpp"

InputParser::InputParser(int argc, const char * argv[])
{
  readInput(argv[1]);

  std::vector<std::string> cliargs(argv + 1, argv + argc);
  replaceCLIArgs(cliargs);

  expandInput();
}

void
InputParser::readInput(const char * filename)
{
  std::ifstream file(filename);
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string input = buffer.str();
  _root.reset(hit::parse("Hit parser", input));
}

void
InputParser::replaceCLIArgs(const std::vector<std::string> & cliargs)
{
  std::string cli_input = hitCLIFilter(cliargs);
  _cli_root.reset(hit::parse("Hit cliargs", cli_input));
  hit::explode(_cli_root.get());
  hit::explode(_root.get());
  hit::merge(_cli_root.get(), _root.get());
}

void
InputParser::expandInput()
{
  hit::BraceExpander expander("Hit expander");
  hit::EnvEvaler env;
  hit::RawEvaler raw;
  expander.registerEvaler("env", env);
  expander.registerEvaler("raw", raw);
  _root->walk(&expander);
}

std::string
InputParser::hitCLIFilter(const std::vector<std::string> & cliargs) const
{
  std::string hit_text;
  bool afterDoubleDash = false;
  for (std::size_t i = 1; i < cliargs.size(); i++)
  {
    std::string arg(cliargs[i]);

    // all args after a "--" are hit parameters
    if (arg == "--")
    {
      afterDoubleDash = true;
      continue;
    } // otherwise try to guess if a hit params have started by looking for "=" and "/"
    else if (arg.find("=", 0) != std::string::npos)
      afterDoubleDash = true;

    // skip over args that don't look like or are before hit parameters
    if (!afterDoubleDash)
      continue;
    // skip arguments with no equals sign
    if (arg.find("=", 0) == std::string::npos)
      continue;
    // skip cli flags (i.e. start with dash)
    if (arg.find("-", 0) == 0)
      continue;

    auto pos = arg.find(":", 0);
    if (pos == 0) // trim leading colon
      arg = arg.substr(pos + 1, arg.size() - pos - 1);
    else if (pos != std::string::npos && pos < arg.find("=", 0)) // param is for non-main subapp
      continue;

    hit::check("CLI_ARG", arg);
    hit_text += " " + arg;
    // handle case where bash ate quotes around an empty string after the "="
    if (arg.find("=", 0) == arg.size() - 1)
      hit_text += "''";
  }
  return hit_text;
}
