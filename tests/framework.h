#pragma once

#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace aurora_test {

inline bool& current_test_failed() {
  static thread_local bool failed = false;
  return failed;
}

struct TestCase {
  std::string suite;
  std::string name;
  std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
  static std::vector<TestCase> tests;
  return tests;
}

struct Registrar {
  Registrar(const char* suite, const char* name, std::function<void()> fn) {
    registry().push_back({suite, name, std::move(fn)});
  }
};

inline void fail(const char* file, int line, const std::string& message) {
  current_test_failed() = true;
  std::cerr << "  FAIL " << file << ":" << line << " - " << message << std::endl;
}

template<typename Lhs, typename Rhs>
inline void assert_eq_impl(const char* file, int line, const char* lhs_expr, const char* rhs_expr, const Lhs& lhs, const Rhs& rhs) {
  if (lhs == rhs) {
    return;
  }
  std::ostringstream stream;
  stream << "Expected " << lhs_expr << " == " << rhs_expr << " (" << lhs << " vs " << rhs << ")";
  fail(file, line, stream.str());
}

inline void assert_streq_impl(const char* file, int line, const char* lhs_expr, const char* rhs_expr, const std::string& lhs, const char* rhs) {
  if (lhs == rhs) {
    return;
  }
  std::ostringstream stream;
  stream << "Expected " << lhs_expr << " == " << rhs_expr << " (\"" << lhs << "\" vs \"" << rhs << "\")";
  fail(file, line, stream.str());
}

inline void assert_true_impl(const char* file, int line, const char* expr, bool value) {
  if (value) {
    return;
  }
  fail(file, line, std::string("Expected true: ") + expr);
}

inline void assert_false_impl(const char* file, int line, const char* expr, bool value) {
  if (!value) {
    return;
  }
  fail(file, line, std::string("Expected false: ") + expr);
}

inline int run_all() {
  int passed = 0;
  int failed = 0;

  std::cout << "Running " << registry().size() << " test(s)...\n\n";

  for (const TestCase& test : registry()) {
    current_test_failed() = false;
    std::cout << test.suite << "." << test.name << " ... " << std::flush;
    test.fn();

    if (current_test_failed()) {
      failed++;
      std::cout << "FAILED\n" << std::flush;
    } else {
      passed++;
      std::cout << "OK\n" << std::flush;
    }
  }

  std::cout << "\n" << passed << " passed, " << failed << " failed\n";
  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

} // namespace aurora_test

#define AURORA_TEST_CONCAT_INNER(a, b) a##b
#define AURORA_TEST_CONCAT(a, b) AURORA_TEST_CONCAT_INNER(a, b)
#define AURORA_TEST_UNIQUE(name) AURORA_TEST_CONCAT(aurora_test_, name)

#define TEST(suite, name) \
  void AURORA_TEST_UNIQUE(name)(); \
  static aurora_test::Registrar AURORA_TEST_UNIQUE(registrar_##name)(#suite, #name, AURORA_TEST_UNIQUE(name)); \
  void AURORA_TEST_UNIQUE(name)()

#define ASSERT_TRUE(expr) \
  do { \
    aurora_test::assert_true_impl(__FILE__, __LINE__, #expr, static_cast<bool>(expr)); \
    if (aurora_test::current_test_failed()) { \
      return; \
    } \
  } while (0)

#define ASSERT_FALSE(expr) \
  do { \
    aurora_test::assert_false_impl(__FILE__, __LINE__, #expr, static_cast<bool>(expr)); \
    if (aurora_test::current_test_failed()) { \
      return; \
    } \
  } while (0)

#define ASSERT_EQ(lhs, rhs) \
  do { \
    aurora_test::assert_eq_impl(__FILE__, __LINE__, #lhs, #rhs, (lhs), (rhs)); \
    if (aurora_test::current_test_failed()) { \
      return; \
    } \
  } while (0)
