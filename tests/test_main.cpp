#include "test_framework.hpp"

#include <exception>
#include <iostream>

int main() {
    std::size_t failures = 0;

    for (const auto& test : sasd::ui::tests::registry()) {
        try {
            test.function();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << "\n       " << error.what() << '\n';
        } catch (...) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << "\n       unknown exception\n";
        }
    }

    const auto total = sasd::ui::tests::registry().size();
    std::cout << "\n" << (total - failures) << '/' << total << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
