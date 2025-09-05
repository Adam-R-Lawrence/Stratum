#pragma once

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Stratum
{

// An argument can now hold either a double or a string.
struct Arg
{
  char letter {};
  std::variant<double, std::string> value;
};

struct GCodeCommand
{
  std::string command;
  std::vector<Arg> arguments;
};

// This is a completely new, more robust parsing implementation that handles
// commands, numbers, and quoted strings without relying on simple token
// splitting.
inline GCodeCommand parseLine(std::string_view view)
{
  GCodeCommand cmd;

  // 1. Find the first space to isolate the command (e.g., "G1", "M701")
  auto first_space = view.find(' ');
  if (first_space == std::string_view::npos) {
    // Line is only a command, no arguments (e.g., "G28")
    cmd.command = std::string(view);
    return cmd;
  }

  cmd.command = std::string(view.substr(0, first_space));
  view.remove_prefix(first_space + 1);

  // 2. Parse the arguments
  size_t pos = 0;
  while (pos < view.length()) {
    // Skip whitespace
    while (pos < view.length()
           && std::isspace(static_cast<unsigned char>(view[pos])))
    {
      pos++;
    }
    if (pos >= view.length())
      break;

    // An argument must start with a letter
    if (!std::isalpha(static_cast<unsigned char>(view[pos]))) {
      throw std::runtime_error("Invalid G-code argument format.");
    }

    Arg current_arg;
    current_arg.letter = view[pos];
    pos++;

    if (pos >= view.length()) {  // Letter-only argument (e.g., M5)
      cmd.arguments.push_back(current_arg);
      break;
    }

    // Check for a quoted string value
    if (view[pos] == '"') {
      pos++;  // Skip the opening quote
      auto end_quote = view.find('"', pos);
      if (end_quote == std::string_view::npos) {
        throw std::runtime_error("Mismatched quote in G-code argument.");
      }
      current_arg.value = std::string(view.substr(pos, end_quote - pos));
      pos = end_quote + 1;
    } else {  // Assume numeric value
      size_t num_end = pos;
      while (num_end < view.length()
             && (std::isdigit(static_cast<unsigned char>(view[num_end]))
                 || view[num_end] == '.' || view[num_end] == '-'))
      {
        num_end++;
      }
      std::string num_str = std::string(view.substr(pos, num_end - pos));
      try {
        current_arg.value = std::stod(num_str);
      } catch (const std::exception&) {
        throw std::runtime_error("Invalid numeric value in argument: "
                                 + num_str);
      }
      pos = num_end;
    }
    cmd.arguments.push_back(current_arg);
  }
  return cmd;
}

// Parses a G-code file and writes each command to the output iterator.
// Throws std::runtime_error if the file cannot be opened.
template<typename OutputIt>
void parseFile(const std::filesystem::path& path, OutputIt out)
{
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open " + path.string());
  }

  std::string line;
  while (std::getline(file, line)) {
    // Trim leading whitespace
    std::string_view view(line);
    size_t start = view.find_first_not_of(" \t");
    if (start != std::string_view::npos) {
      view = view.substr(start);
    } else {
      continue;  // Line is all whitespace
    }

    // Skip empty lines or comments
    if (view.empty() || view.front() == ';') {
      continue;
    }

    // Trim trailing comments
    auto comment_pos = view.find(';');
    if (comment_pos != std::string_view::npos) {
      view = view.substr(0, comment_pos);
    }

    // Trim trailing whitespace
    size_t end = view.find_last_not_of(" \t");
    if (end != std::string_view::npos) {
      view = view.substr(0, end + 1);
    }

    if (view.empty()) {
      continue;
    }

    *out++ = parseLine(view);
  }
}

}  // namespace Stratum