#include "ref_cell.hpp"
#include <iostream>
#include <string>

int main() {
    // The OJ will run compiled binary; here we perform minimal sanity.
    // Read optional input but do nothing; problem focuses on class behavior checked by tests.
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Minimal usage to ensure template instantiation with int and std::string
    RefCell<int> ci(42);
    auto r = ci.borrow();
    (void)*r;
    {
        auto opt = ci.try_borrow();
        if (opt) {
            (void)**opt;
        }
    }
    {
        auto optm = ci.try_borrow_mut();
        if (optm) {
            **optm = 43;
        }
    }

    RefCell<std::string> cs(std::string("hello"));
    auto rm = cs.borrow_mut();
    *rm = "world";

    std::cout << "ok\n";
    return 0;
}

