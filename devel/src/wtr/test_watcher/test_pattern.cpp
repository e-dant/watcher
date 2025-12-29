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

TEST_CASE("Pattern Matching - Consecutive Wildcards", "[pattern][not-perf]")
{
  // Multiple consecutive stars
  REQUIRE(matchGlobPattern("***", "anything"));
  REQUIRE(matchGlobPattern("***", "dir/file.txt"));
  REQUIRE(matchGlobPattern("****", "multiple/dir/file.txt"));

  // Multiple consecutive question marks
  REQUIRE(matchGlobPattern("????", "abcd"));
  REQUIRE_FALSE(matchGlobPattern("????", "abc"));
  REQUIRE_FALSE(matchGlobPattern("????", "abcde"));

  // Mixed consecutive wildcards
  REQUIRE(matchGlobPattern("*?*", "a"));
  REQUIRE(matchGlobPattern("*?*", "ab"));
  REQUIRE(matchGlobPattern("?*?", "ab"));
  REQUIRE_FALSE(matchGlobPattern("?*?", "a"));
  REQUIRE(matchGlobPattern("**?", "a"));
  REQUIRE(matchGlobPattern("**?", "dir/a"));
  REQUIRE(matchGlobPattern("?**", "a"));
  REQUIRE(matchGlobPattern("?**", "a/dir/file"));
}

TEST_CASE("Pattern Matching - Globstar Edge Cases", "[pattern][not-perf]")
{
  // Globstar at end of pattern
  REQUIRE(matchGlobPattern("dir/**", "dir/"));
  REQUIRE(matchGlobPattern("dir/**", "dir/file.txt"));
  REQUIRE(matchGlobPattern("dir/**", "dir/sub/file.txt"));

  // Multiple globstars in one pattern
  REQUIRE(matchGlobPattern("**/src/**/test.txt", "src/test.txt"));
  REQUIRE(matchGlobPattern("**/src/**/test.txt", "project/src/test.txt"));
  REQUIRE(matchGlobPattern("**/src/**/test.txt", "project/src/lib/test.txt"));
  REQUIRE(matchGlobPattern("**/src/**/test.txt", "src/lib/test.txt"));

  // Globstar with no separator after
  REQUIRE(matchGlobPattern("dir/**test.txt", "dir/test.txt"));
  REQUIRE(matchGlobPattern("dir/**test.txt", "dir/sub/test.txt"));

  // Globstar matching empty path
  REQUIRE(matchGlobPattern("a/**/b", "a/b"));
  REQUIRE(matchGlobPattern("a/**/b", "a/x/b"));
  REQUIRE(matchGlobPattern("a/**/b", "a/x/y/b"));
}

TEST_CASE(
  "Pattern Matching - Brace Expansion Edge Cases",
  "[pattern][not-perf]")
{
  // Empty alternative in braces
  REQUIRE(matchGlobPattern("", ""));
  REQUIRE(matchGlobPattern("{,test}", ""));
  REQUIRE(matchGlobPattern("{,test}", "test"));
  REQUIRE(matchGlobPattern("file{,.txt}", "file"));
  REQUIRE(matchGlobPattern("file{,.txt}", "file.txt"));

  // Single alternative
  REQUIRE(matchGlobPattern("{test}", "test"));
  REQUIRE_FALSE(matchGlobPattern("{test}", "other"));

  // Nested or malformed braces (should fail gracefully)
  REQUIRE_FALSE(matchGlobPattern("{test", "test"));
  REQUIRE_FALSE(matchGlobPattern("test}", "test"));

  // Braces with wildcards inside
  REQUIRE(matchGlobPattern("{*.txt,*.cpp}", "test.txt"));
  REQUIRE(matchGlobPattern("{*.txt,*.cpp}", "main.cpp"));
  REQUIRE_FALSE(matchGlobPattern("{*.txt,*.cpp}", "test.hpp"));

  // Multiple brace groups
  REQUIRE(matchGlobPattern("{a,b}{1,2}", "a1"));
  REQUIRE(matchGlobPattern("{a,b}{1,2}", "a2"));
  REQUIRE(matchGlobPattern("{a,b}{1,2}", "b1"));
  REQUIRE(matchGlobPattern("{a,b}{1,2}", "b2"));
  REQUIRE_FALSE(matchGlobPattern("{a,b}{1,2}", "a3"));
}

TEST_CASE("Pattern Matching - Special Characters", "[pattern][not-perf]")
{
  // Dots in filenames and patterns
  REQUIRE(matchGlobPattern("*.tar.gz", "archive.tar.gz"));
  REQUIRE(matchGlobPattern("file.*.txt", "file.backup.txt"));
  REQUIRE(matchGlobPattern(".*", ".hidden"));
  REQUIRE(matchGlobPattern(".*", ".bashrc"));
  REQUIRE_FALSE(matchGlobPattern(".*", "visible"));

  // Multiple dots
  REQUIRE(matchGlobPattern("file..txt", "file..txt"));
  REQUIRE(matchGlobPattern("...", "..."));

  // Question mark matching separator (should it?)
  // Based on implementation, ? matches any single character including separator
  REQUIRE(matchGlobPattern("dir?file", "dir/file"));

  // Trailing slashes
  REQUIRE(matchGlobPattern("dir/", "dir/"));
  REQUIRE_FALSE(matchGlobPattern("dir/", "dir"));
}

TEST_CASE(
  "Pattern Matching - Long Patterns and Filenames",
  "[pattern][not-perf]")
{
  // Very long patterns
  std::string longPattern = "dir/";
  for (int i = 0; i < 100; ++i) { longPattern += "sub/"; }
  longPattern += "file.txt";

  std::string longFilename = "dir/";
  for (int i = 0; i < 100; ++i) { longFilename += "sub/"; }
  longFilename += "file.txt";

  REQUIRE(matchGlobPattern(longPattern, longFilename));

  // Long pattern with globstar
  REQUIRE(matchGlobPattern("dir/**/file.txt", longFilename));

  // Many wildcards
  std::string manyWildcards;
  for (int i = 0; i < 50; ++i) { manyWildcards += "*"; }
  REQUIRE(matchGlobPattern(manyWildcards, "anything"));
}

TEST_CASE("Pattern Matching - Case Sensitivity", "[pattern][not-perf]")
{
  // The implementation is case-sensitive
  REQUIRE(matchGlobPattern("Test.txt", "Test.txt"));
  REQUIRE_FALSE(matchGlobPattern("Test.txt", "test.txt"));
  REQUIRE_FALSE(matchGlobPattern("test.txt", "Test.txt"));
  REQUIRE_FALSE(matchGlobPattern("TEST.TXT", "test.txt"));

  // Case sensitivity with wildcards
  REQUIRE(matchGlobPattern("*.TXT", "FILE.TXT"));
  REQUIRE_FALSE(matchGlobPattern("*.TXT", "file.txt"));
  REQUIRE(matchGlobPattern("Test*", "TestFile"));
  REQUIRE_FALSE(matchGlobPattern("Test*", "testFile"));
}

TEST_CASE(
  "Pattern Matching - Numeric and Special Filenames",
  "[pattern][not-perf]")
{
  // Numeric filenames
  REQUIRE(matchGlobPattern("123", "123"));
  REQUIRE(matchGlobPattern("*123*", "test123file"));
  REQUIRE(matchGlobPattern("file?.txt", "file1.txt"));

  // Filenames with dashes and underscores
  REQUIRE(matchGlobPattern("test-file.txt", "test-file.txt"));
  REQUIRE(matchGlobPattern("test_file.txt", "test_file.txt"));
  REQUIRE(matchGlobPattern("*-*", "test-file"));
  REQUIRE(matchGlobPattern("*_*", "test_file"));

  // Filenames with spaces (if supported)
  REQUIRE(matchGlobPattern("my file.txt", "my file.txt"));
  REQUIRE(matchGlobPattern("my*.txt", "my file.txt"));
}

TEST_CASE("Base Directory - Edge Cases", "[pattern][not-perf]")
{
  // Pattern with only wildcards
  REQUIRE(
    getBaseDirectoryToWatch("*") == std::filesystem::absolute(".").string());
  REQUIRE(
    getBaseDirectoryToWatch("**") == std::filesystem::absolute(".").string());
  REQUIRE(
    getBaseDirectoryToWatch("?") == std::filesystem::absolute(".").string());

  // Root directory patterns
  REQUIRE(getBaseDirectoryToWatch("/*.txt") == "/");
  REQUIRE(getBaseDirectoryToWatch("/**/*.txt") == "/");

  // Multiple consecutive separators
  REQUIRE(getBaseDirectoryToWatch("/home//user/**/*.txt") == "/home//user");

  // Trailing separator
  REQUIRE(getBaseDirectoryToWatch("/home/user/") == "/home/user");

  // Question mark in path
  REQUIRE(getBaseDirectoryToWatch("/home/user?/file.txt") == "/home");

  // Brace in path
  REQUIRE(getBaseDirectoryToWatch("/home/{a,b}/file.txt") == "/home");
}

TEST_CASE("Pattern Matching - Star at Pattern End", "[pattern][not-perf]")
{
  // Single star at end
  REQUIRE(matchGlobPattern("test*", "test"));
  REQUIRE(matchGlobPattern("test*", "test123"));
  REQUIRE(matchGlobPattern("test*", "testfile"));
  REQUIRE_FALSE(matchGlobPattern("test*", "test/file"));

  // Globstar at end
  REQUIRE(matchGlobPattern("test/**", "test/"));
  REQUIRE(matchGlobPattern("test/**", "test/file"));
  REQUIRE(matchGlobPattern("test/**", "test/dir/file"));
}

TEST_CASE(
  "Pattern Matching - Brace Expansion with Trailing Content",
  "[pattern][not-perf]")
{
  // Brace expansion followed by more pattern
  REQUIRE(matchGlobPattern("{src,lib}/*.cpp", "src/main.cpp"));
  REQUIRE(matchGlobPattern("{src,lib}/*.cpp", "lib/util.cpp"));
  REQUIRE_FALSE(matchGlobPattern("{src,lib}/*.cpp", "bin/test.cpp"));

  // Brace expansion with literal text after
  REQUIRE(matchGlobPattern("prefix{A,B}suffix", "prefixAsuffix"));
  REQUIRE(matchGlobPattern("prefix{A,B}suffix", "prefixBsuffix"));
  REQUIRE_FALSE(matchGlobPattern("prefix{A,B}suffix", "prefixCsuffix"));
}

TEST_CASE(
  "Pattern Matching - Empty Filenames and Patterns",
  "[pattern][not-perf]")
{
  // Empty strings
  REQUIRE(matchGlobPattern("", ""));
  REQUIRE_FALSE(matchGlobPattern("", "nonempty"));
  REQUIRE_FALSE(matchGlobPattern("nonempty", ""));

  // Star matching empty string
  REQUIRE(matchGlobPattern("*", ""));
  REQUIRE(matchGlobPattern("**", ""));
  REQUIRE(matchGlobPattern("prefix*", "prefix"));
  REQUIRE(matchGlobPattern("*suffix", "suffix"));
}

TEST_CASE("Pattern Matching - Question Mark Edge Cases", "[pattern][not-perf]")
{
  // Question mark matching path separator
  REQUIRE(matchGlobPattern("a?b", "a/b"));
  REQUIRE(matchGlobPattern("test?file", "test/file"));

  // Question mark at end
  REQUIRE(matchGlobPattern("test?", "test1"));
  REQUIRE(matchGlobPattern("test?", "testa"));
  REQUIRE_FALSE(matchGlobPattern("test?", "test"));
  REQUIRE_FALSE(matchGlobPattern("test?", "test12"));

  // Question mark at start
  REQUIRE(matchGlobPattern("?test", "atest"));
  REQUIRE(matchGlobPattern("?test", "1test"));
  REQUIRE_FALSE(matchGlobPattern("?test", "test"));
}

TEST_CASE(
  "Pattern Matching - Mixed Wildcard Combinations",
  "[pattern][not-perf]")
{
  // Star and question mark together
  REQUIRE(matchGlobPattern("*?.txt", "a.txt"));
  REQUIRE(matchGlobPattern("*?.txt", "ab.txt"));
  REQUIRE_FALSE(matchGlobPattern("*?.txt", ".txt"));

  REQUIRE(matchGlobPattern("?*.txt", "a.txt"));
  REQUIRE(matchGlobPattern("?*.txt", "ab.txt"));
  REQUIRE_FALSE(matchGlobPattern("?*.txt", ".txt"));

  // Globstar and star together
  REQUIRE(matchGlobPattern("**/*.txt", "file.txt"));
  REQUIRE(matchGlobPattern("**/*.txt", "dir/file.txt"));
  REQUIRE(matchGlobPattern("**/*/*.txt", "dir/file.txt"));
  REQUIRE(matchGlobPattern("**/*/*.txt", "a/b/c/file.txt"));

  // All three wildcard types
  REQUIRE(matchGlobPattern("**/src/?*.{cpp,hpp}", "src/a.cpp"));
  REQUIRE(matchGlobPattern("**/src/?*.{cpp,hpp}", "project/src/ab.hpp"));
  REQUIRE_FALSE(matchGlobPattern("**/src/?*.{cpp,hpp}", "src/.cpp"));
}

TEST_CASE("Pattern Matching - Backtracking Scenarios", "[pattern][not-perf]")
{
  // Patterns that require backtracking to match correctly
  REQUIRE(matchGlobPattern("a*a*a*a*b", "aaaaaaaaab"));
  REQUIRE(matchGlobPattern("a*a*a*a*b", "aaaaaaaab"));
  REQUIRE_FALSE(matchGlobPattern("a*a*a*a*b", "aaaaaaaaa"));

  // Complex backtracking with multiple wildcards
  REQUIRE(matchGlobPattern("*test*file*", "prefix_test_middle_file_suffix"));
  REQUIRE(matchGlobPattern("*a*b*c*", "xaxbxcx"));
  REQUIRE_FALSE(matchGlobPattern("*a*b*c*", "xaxbx"));

  // Globstar backtracking
  REQUIRE(matchGlobPattern("**/a/**/b", "a/b"));
  REQUIRE(matchGlobPattern("**/a/**/b", "x/a/y/b"));
  REQUIRE(matchGlobPattern("**/a/**/b", "x/y/a/b"));
  REQUIRE(matchGlobPattern("**/a/**/b", "a/x/y/b"));
}
