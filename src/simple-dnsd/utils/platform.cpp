/**
 * @file platform.cpp
 * @brief Platform helpers
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/utils/platform.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace simple_dnsd {

std::string platformName() {
#ifdef SIMPLE_DNSD_WINDOWS
  return "Windows";
#elif defined(SIMPLE_DNSD_MACOS)
  return "macOS";
#elif defined(SIMPLE_DNSD_FREEBSD)
  return "FreeBSD";
#elif defined(SIMPLE_DNSD_LINUX)
  return "Linux";
#else
  return "Unknown";
#endif
}

std::string trim(std::string value) {
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

std::vector<std::string> split(const std::string &value, char delim) {
  std::vector<std::string> out;
  std::string cur;
  for (char ch : value) {
    if (ch == delim) {
      out.push_back(trim(cur));
      cur.clear();
    } else {
      cur.push_back(ch);
    }
  }
  out.push_back(trim(cur));
  return out;
}

std::vector<std::string> splitWs(const std::string &value) {
  std::vector<std::string> out;
  std::istringstream in(value);
  std::string tok;
  while (in >> tok) {
    out.push_back(tok);
  }
  return out;
}

bool parseBool(const std::string &value) {
  const std::string lower = toLower(value);
  return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

std::string join(const std::vector<std::string> &parts, const std::string &sep) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i != 0) {
      out += sep;
    }
    out += parts[i];
  }
  return out;
}

}  // namespace simple_dnsd
