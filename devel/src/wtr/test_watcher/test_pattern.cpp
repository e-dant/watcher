#include "detail/wtr/watcher/pattern.hpp"
#include "snitch/snitch.hpp"
#include <filesystem>
#include <string>

TEST_CASE("Base Directory Extraction", "[pattern][not-perf]")
{
  REQUIRE(
    getBaseDirectoryToWatch("/home/user/src/**/*.cpp") == "/home/user/src");
  REQUIRE(getBaseDirectoryToWatch("/home/user/*.txt") == "/home/user");
  REQUIRE(getBaseDirectoryToWatch("/home/user/file?.txt") == "/home/user");
  REQUIRE(
    getBaseDirectoryToWatch("/home/user/{src,lib}/*.cpp") == "/home/user");
  REQUIRE(getBaseDirectoryToWatch("/home/user/file.txt") == "/home/user");
  REQUIRE(getBaseDirectoryToWatch("/tmp/**/*.log") == "/tmp");
  REQUIRE(getBaseDirectoryToWatch("/tmp") == "/tmp");
  REQUIRE(
    getBaseDirectoryToWatch("/home/user/project/src/**/*.{cpp,hpp}")
    == "/home/user/project/src");
}

TEST_CASE("Pattern Matching - Exact Match", "[pattern][not-perf]")
{
  REQUIRE(matchGlobPattern("test.txt", "test.txt"));
  REQUIRE(matchGlobPattern("dir/file.cpp", "dir/file.cpp"));
  REQUIRE_FALSE(matchGlobPattern("test.txt", "test.cpp"));
  REQUIRE_FALSE(matchGlobPattern("test.txt", "test.txt.bak"));
  REQUIRE_FALSE(matchGlobPattern("verylongpattern", "short"));
}

TEST_CASE("Pattern Matching - Question Mark Wildcard", "[pattern][not-perf]")
{
  // Single character wildcard
  REQUIRE(matchGlobPattern("test?.txt", "test1.txt"));
  REQUIRE(matchGlobPattern("test?.txt", "testa.txt"));
  REQUIRE(matchGlobPattern("?.txt", "a.txt"));
  REQUIRE_FALSE(matchGlobPattern("?.txt", "ab.txt"));

  // Multiple question marks
  REQUIRE(matchGlobPattern("test??.txt", "test12.txt"));
  REQUIRE(matchGlobPattern("???", "abc"));
  REQUIRE_FALSE(matchGlobPattern("???", "ab"));
}

TEST_CASE("Pattern Matching - Single Star Wildcard", "[pattern][not-perf]")
{
  // Star matches zero or more characters within a path segment
  REQUIRE(matchGlobPattern("*.txt", "test.txt"));
  REQUIRE(matchGlobPattern("*.txt", "file.txt"));
  REQUIRE(matchGlobPattern("test*.txt", "test123.txt"));
  REQUIRE(matchGlobPattern("test*.txt", "test.txt"));
  REQUIRE(matchGlobPattern("*test*", "mytest"));
  REQUIRE(matchGlobPattern("*test*", "test"));
  REQUIRE(matchGlobPattern("*test*", "testfile"));
  REQUIRE(matchGlobPattern("*test*", "mytestfile"));
  REQUIRE(matchGlobPattern("*/*/test.txt", "dir1/dir2/test.txt"));
  REQUIRE(matchGlobPattern("dir1/*/test.txt", "dir1/dir2/test.txt"));

  // Star should NOT match path separators
  REQUIRE_FALSE(matchGlobPattern("*.txt", "dir/test.txt"));
  REQUIRE_FALSE(matchGlobPattern("test*", "test/file.txt"));
  REQUIRE_FALSE(matchGlobPattern("*/*/test.txt", "dir1/dir2/test.json"));
  REQUIRE_FALSE(matchGlobPattern("dir1/*/test.txt", "dir2/dir1/test.txt"));
}

TEST_CASE("Pattern Matching - Double Star Globstar", "[pattern][not-perf]")
{
  // Globstar matches across directories
  REQUIRE(matchGlobPattern("**/test.txt", "test.txt"));
  REQUIRE(matchGlobPattern("**/test.txt", "dir/test.txt"));
  REQUIRE(matchGlobPattern("**/test.txt", "dir/subdir/test.txt"));
  REQUIRE(matchGlobPattern("dir/**/file.txt", "dir/file.txt"));
  REQUIRE(matchGlobPattern("dir/**/file.txt", "dir/sub/file.txt"));
  REQUIRE(matchGlobPattern("dir/**/file.txt", "dir/sub/deep/file.txt"));
  REQUIRE(matchGlobPattern("/root/**/*.txt", "/root/dir/file.txt"));
  REQUIRE(
    matchGlobPattern("dir/**/dir2/**/file.txt", "dir/sub/dir2/sub2/file.txt"));

  // Globstar at the beginning
  REQUIRE(matchGlobPattern("**/*.txt", "file.txt"));
  REQUIRE(matchGlobPattern("**/*.txt", "dir/file.txt"));
  REQUIRE(matchGlobPattern("**/*.txt", "dir/sub/file.txt"));

  // Negative cases
  REQUIRE_FALSE(
    matchGlobPattern("/false-root/**/dir/test.txt", "dir/test.txt"));
  REQUIRE_FALSE(matchGlobPattern("**/dir/test.txt", "test.txt"));
  REQUIRE_FALSE(
    matchGlobPattern("dir/**/dir2/**/file.txt", "dir/sub/sub2/file.txt"));
}

TEST_CASE("Pattern Matching - Complex Patterns", "[pattern][not-perf]")
{
  // Combining different wildcards
  REQUIRE(matchGlobPattern("test?.*.txt", "test1.file.txt"));
  REQUIRE(matchGlobPattern("**/src/*.cpp", "src/main.cpp"));
  REQUIRE(matchGlobPattern("**/src/*.cpp", "project/src/main.cpp"));
  REQUIRE(matchGlobPattern("dir/**/file?.txt", "dir/file1.txt"));
  REQUIRE(matchGlobPattern("dir/**/file?.txt", "dir/sub/file2.txt"));

  // Multiple stars
  REQUIRE(matchGlobPattern("*test*file*", "mytestfile"));
  REQUIRE(matchGlobPattern("*test*file*", "test_file_name"));
  REQUIRE(matchGlobPattern("*test*file*", "prefix_test_middle_file_suffix"));
}

TEST_CASE("Pattern Matching - Edge Cases", "[pattern][not-perf]")
{
  // Empty patterns and strings
  REQUIRE(matchGlobPattern("", ""));
  REQUIRE_FALSE(matchGlobPattern("", "test"));
  REQUIRE_FALSE(matchGlobPattern("test", ""));

  // Only wildcards
  REQUIRE(matchGlobPattern("*", "anything"));
  REQUIRE(matchGlobPattern("*", ""));
  REQUIRE(matchGlobPattern("**", "anything"));
  REQUIRE(matchGlobPattern("**", "dir/file"));
  REQUIRE(matchGlobPattern("***", "anything"));

  // Trailing wildcards
  REQUIRE(matchGlobPattern("src/**", "src/main.cpp"));
  REQUIRE(matchGlobPattern("src/**", "src/dir/main.cpp"));
  REQUIRE(matchGlobPattern("src/**", "src/dir/sub/main.cpp"));
}

TEST_CASE("Pattern Matching - Windows Paths", "[pattern][not-perf]")
{
  // Save original separator
  char original_sep = path_separator;

  // Test with Windows-style backslash separator
  path_separator = '\\';

  // Basic patterns with backslashes
  REQUIRE(matchGlobPattern("dir\\file.txt", "dir\\file.txt"));
  REQUIRE(matchGlobPattern("dir\\*.txt", "dir\\test.txt"));
  REQUIRE_FALSE(matchGlobPattern("dir\\*.txt", "dir\\sub\\test.txt"));

  // Single star should not cross directory boundaries on Windows
  REQUIRE(matchGlobPattern("src\\*", "src\\main.cpp"));
  REQUIRE_FALSE(matchGlobPattern("src\\*", "src\\dir\\main.cpp"));

  // Double star should cross directory boundaries on Windows
  REQUIRE(matchGlobPattern("src\\**", "src\\main.cpp"));
  REQUIRE(matchGlobPattern("src\\**", "src\\dir\\main.cpp"));
  REQUIRE(matchGlobPattern("src\\**", "src\\dir\\sub\\main.cpp"));

  // Globstar with backslash separators
  REQUIRE(matchGlobPattern("**\\*.cpp", "main.cpp"));
  REQUIRE(matchGlobPattern("**\\*.cpp", "src\\main.cpp"));
  REQUIRE(matchGlobPattern("**\\*.cpp", "src\\lib\\main.cpp"));

  // Question mark wildcard with Windows paths
  REQUIRE(matchGlobPattern("file?.txt", "file1.txt"));
  REQUIRE(matchGlobPattern("dir\\test?.cpp", "dir\\test1.cpp"));

  // Brace expansion with Windows paths
  REQUIRE(matchGlobPattern("*.{txt,cpp}", "test.txt"));
  REQUIRE(matchGlobPattern("*.{txt,cpp}", "main.cpp"));
  REQUIRE(matchGlobPattern("dir\\*.{h,cpp}", "dir\\main.h"));
  REQUIRE(matchGlobPattern("dir\\*.{h,cpp}", "dir\\main.cpp"));

  // Complex Windows patterns
  REQUIRE(matchGlobPattern("src\\**\\*.{cpp,hpp}", "src\\main.cpp"));
  REQUIRE(matchGlobPattern("src\\**\\*.{cpp,hpp}", "src\\lib\\util.hpp"));
  REQUIRE(matchGlobPattern("src\\**\\*.{cpp,hpp}", "src\\a\\b\\c\\test.cpp"));

  // Windows absolute paths (C:\...)
  REQUIRE(matchGlobPattern(
    "C:\\Users\\*\\Documents\\*.txt",
    "C:\\Users\\alex\\Documents\\test.txt"));
  REQUIRE_FALSE(matchGlobPattern(
    "C:\\Users\\*\\Documents\\*.txt",
    "C:\\Users\\alex\\Desktop\\test.txt"));

  // Restore original separator
  path_separator = original_sep;
}

TEST_CASE(
  "Pattern Matching - Windows vs Unix Separators",
  "[pattern][not-perf]")
{
  // Save original separator
  char original_sep = path_separator;

  // Test that changing separator affects pattern matching
  path_separator = '/';
  REQUIRE(matchGlobPattern("dir/sub/*.txt", "dir/sub/test.txt"));
  REQUIRE_FALSE(matchGlobPattern("dir/sub/*.txt", "dir/sub/deep/test.txt"));

  path_separator = '\\';
  REQUIRE(matchGlobPattern("dir\\sub\\*.txt", "dir\\sub\\test.txt"));
  REQUIRE_FALSE(
    matchGlobPattern("dir\\sub\\*.txt", "dir\\sub\\deep\\test.txt"));

  // Restore original separator
  path_separator = original_sep;
}

TEST_CASE("Pattern Matching - Real World Examples", "[pattern][not-perf]")
{
  // Basic brace expansion
  REQUIRE(matchGlobPattern("{file1,file2}", "file1"));
  REQUIRE(matchGlobPattern("{file1,file2}", "file2"));
  REQUIRE_FALSE(matchGlobPattern("{file1,file2}", "file3"));

  // Brace expansion with extensions
  REQUIRE(matchGlobPattern("test.{txt,cpp}", "test.txt"));
  REQUIRE(matchGlobPattern("test.{txt,cpp}", "test.cpp"));
  REQUIRE_FALSE(matchGlobPattern("test.{txt,cpp}", "test.hpp"));

  // Brace expansion with directories
  REQUIRE(matchGlobPattern("{src,lib}/main.cpp", "src/main.cpp"));
  REQUIRE(matchGlobPattern("{src,lib}/main.cpp", "lib/main.cpp"));
  REQUIRE_FALSE(matchGlobPattern("{src,lib}/main.cpp", "bin/main.cpp"));

  // Multiple alternatives
  REQUIRE(matchGlobPattern("file.{h,c,cpp,hpp}", "file.h"));
  REQUIRE(matchGlobPattern("file.{h,c,cpp,hpp}", "file.c"));
  REQUIRE(matchGlobPattern("file.{h,c,cpp,hpp}", "file.cpp"));
  REQUIRE(matchGlobPattern("file.{h,c,cpp,hpp}", "file.hpp"));
  REQUIRE_FALSE(matchGlobPattern("file.{h,c,cpp,hpp}", "file.txt"));

  // Brace expansion with wildcards
  REQUIRE(matchGlobPattern("*.{txt,cpp}", "test.txt"));
  REQUIRE(matchGlobPattern("*.{txt,cpp}", "main.cpp"));
  REQUIRE_FALSE(matchGlobPattern("*.{txt,cpp}", "test.hpp"));

  // Brace expansion with globstar
  REQUIRE(matchGlobPattern("**/*.{js,ts}", "main.js"));
  REQUIRE(matchGlobPattern("**/*.{js,ts}", "src/main.ts"));
  REQUIRE(matchGlobPattern("**/*.{js,ts}", "src/lib/util.js"));
  REQUIRE_FALSE(matchGlobPattern("**/*.{js,ts}", "main.cpp"));
}

TEST_CASE("Pattern Matching - Path Separator Handling", "[pattern][not-perf]")
{
  // Test with Unix-style paths
  REQUIRE(matchGlobPattern("dir/file.txt", "dir/file.txt"));
  REQUIRE(matchGlobPattern("dir/*.txt", "dir/test.txt"));
  REQUIRE_FALSE(matchGlobPattern("dir/*.txt", "dir/sub/test.txt"));

  // Single star should not cross directory boundaries
  REQUIRE(matchGlobPattern("src/*", "src/main.cpp"));
  REQUIRE_FALSE(matchGlobPattern("src/*", "src/dir/main.cpp"));

  // Double star should cross directory boundaries
  REQUIRE(matchGlobPattern("src/**", "src/main.cpp"));
  REQUIRE(matchGlobPattern("src/**", "src/dir/main.cpp"));
  REQUIRE(matchGlobPattern("src/**", "src/dir/sub/main.cpp"));
}
