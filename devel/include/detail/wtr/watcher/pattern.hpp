#pragma once

#include <filesystem>
#include <iostream>
#include <string>

// Global path separator - defaults to system preferred, but can be changed for
// testing
inline char path_separator = std::filesystem::path::preferred_separator;

// Returns the uppermost directory path without any wildcards (as absolute path)
//   "/home/user/src/**/*.cpp" -> "/home/user/src"
//   "src/*/test.txt" -> "/current/working/dir/src"
//   "**/*.txt" -> "/current/working/dir"
//   "/absolute/path/*.txt" -> "/absolute/path"
//   "C:\\Users\\name\\src\\**\\*.cpp" -> "C:\\Users\\name\\src"
inline std::string getBaseDirectoryToWatch(std::string const& pattern)
{
  size_t wildcardPos = pattern.find_first_of("*?{");

  if (wildcardPos == std::string::npos) {
    // No wildcards, return the directory of the pattern
    std::filesystem::path p(pattern);
    if (std::filesystem::is_directory(p)) {
      return std::filesystem::absolute(p).string();
    }
    std::filesystem::path parent = p.parent_path();
    if (parent.empty()) { parent = "."; }
    return std::filesystem::absolute(parent).string();
  }

  // Find the last path separator before the wildcard (both / and \)
  size_t lastSep = pattern.find_last_of("/\\", wildcardPos);

  if (lastSep == std::string::npos) {
    // No separator before wildcard, watch current directory
    return std::filesystem::absolute(".").string();
  }

  // Extract the base path up to the separator
  std::string basePath = pattern.substr(0, lastSep);

  // Handle empty path or root path
  if (basePath.empty()) { basePath = std::string(1, path_separator); }

  // Use filesystem::path to normalize and make absolute
  std::filesystem::path result(basePath);
  return std::filesystem::absolute(result).string();
}

inline bool matchGlobPattern(
  std::string const& pattern,
  std::string const& filename,
  size_t p = 0,
  size_t f = 0)
{
  while (f < filename.size()) {
    // Pattern exhausted but filename remains
    if (p >= pattern.size()) { return false; }

    switch (pattern[p]) {
      case '*' :
        // Handle ** globstar
        if (p + 1 < pattern.size() && pattern[p + 1] == '*') {
          p += 2;  // skip **

          // Skip optional path separator after **
          if (p < pattern.size() && pattern[p] == path_separator) { ++p; }

          // Try matching ** with zero or more characters
          for (size_t i = f; i <= filename.size(); ++i) {
            if (matchGlobPattern(pattern, filename, p, i)) { return true; }
          }
          return false;
        }

        // Handle single * wildcard
        ++p;  // skip * in pattern

        // Try matching * with zero or more characters except path separators
        for (size_t match = f;
             match <= filename.size()
             && (match == f || filename[match - 1] != path_separator);
             ++match) {
          if (matchGlobPattern(pattern, filename, p, match)) { return true; }
        }
        return false;

      case '?' :
        // Single character wildcard, skip one character always
        ++p;
        ++f;
        continue;

      case '{' :
        // Shell brace pattern, find closing brace
        {
          size_t close = pattern.find('}', p + 1);
          if (close == std::string::npos) {
            return false;  // malformed pattern, no closing brace
          }

          // Try each comma-separated alternative
          for (size_t start = p + 1, i = start; i < close; ++i) {
            if (pattern[i] != ',' && i != close - 1) { continue; }
            size_t end = (pattern[i] == ',') ? i : close;
            std::string alt = pattern.substr(0, p)
                            + pattern.substr(start, end - start)
                            + pattern.substr(close + 1);
            if (matchGlobPattern(alt, filename, 0, 0)) { return true; }
            start = i + 1;
          }
          return false;
        }

      default :
        // Literal character match
        if (pattern[p] == filename[f]) {
          ++p;
          ++f;
          continue;
        }
        return false;
    }
  }

  // Skip remaining stars in pattern
  while (p < pattern.size() && pattern[p] == '*') { ++p; }

  return p == pattern.size();
}
