#pragma once

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sasd::ui::tests {

struct TestCase {
    std::string name;
    std::function<void()> function;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

class Registrar {
public:
    Registrar(std::string name, std::function<void()> function) {
        registry().push_back({std::move(name), std::move(function)});
    }
};

[[noreturn]] inline void fail(std::string_view expression, std::string_view file, int line) {
    std::ostringstream message;
    message << file << ':' << line << ": CHECK failed: " << expression;
    throw std::runtime_error(message.str());
}

} // namespace sasd::ui::tests

#define SASD_UI_TEST_JOIN_IMPL(left, right) left##right
#define SASD_UI_TEST_JOIN(left, right) SASD_UI_TEST_JOIN_IMPL(left, right)

#define TEST_CASE(name)                                                                            \
    static void SASD_UI_TEST_JOIN(test_function_, __LINE__)();                                      \
    static ::sasd::ui::tests::Registrar SASD_UI_TEST_JOIN(test_registrar_, __LINE__){              \
        name, SASD_UI_TEST_JOIN(test_function_, __LINE__)};                                         \
    static void SASD_UI_TEST_JOIN(test_function_, __LINE__)()

#define CHECK(...)                                                                                 \
    do {                                                                                            \
        if (!(__VA_ARGS__)) {                                                                       \
            ::sasd::ui::tests::fail(#__VA_ARGS__, __FILE__, __LINE__);                              \
        }                                                                                           \
    } while (false)
