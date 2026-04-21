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

// Header-only implementation of a RefCell-like runtime borrow checker.
#pragma once

#include <optional>
#include <stdexcept>

class RefCellError : public std::runtime_error {
public:
    explicit RefCellError(const std::string &message) : std::runtime_error(message) {}
    ~RefCellError() override = default;
};

class BorrowError : public RefCellError {
public:
    explicit BorrowError(const std::string &message) : RefCellError(message) {}
};

class BorrowMutError : public RefCellError {
public:
    explicit BorrowMutError(const std::string &message) : RefCellError(message) {}
};

class DestructionError : public RefCellError {
public:
    explicit DestructionError(const std::string &message) : RefCellError(message) {}
};

template <typename T>
class RefCell {
private:
    T value_;
    mutable int borrow_count_ = 0;     // number of immutable borrows
    mutable bool mut_borrowed_ = false; // whether a mutable borrow is active

    void incr_borrow() const {
        if (mut_borrowed_) throw BorrowError("immutable borrow while mutable active");
        ++borrow_count_;
    }
    void decr_borrow() const {
        if (borrow_count_ > 0) --borrow_count_;
    }
    void start_mut_borrow() {
        if (mut_borrowed_ || borrow_count_ != 0) throw BorrowMutError("mutable borrow conflict");
        mut_borrowed_ = true;
    }
    void end_mut_borrow() {
        mut_borrowed_ = false;
    }

public:
    class Ref;
    class RefMut;

    explicit RefCell(const T &initial_value) : value_(initial_value) {}
    explicit RefCell(T &&initial_value) : value_(std::move(initial_value)) {}

    RefCell(const RefCell &) = delete;
    RefCell &operator=(const RefCell &) = delete;
    RefCell(RefCell &&) = delete;
    RefCell &operator=(RefCell &&) = delete;

    class Ref {
    private:
        const RefCell<T> *cell_ = nullptr;

        explicit Ref(const RefCell<T> *cell) : cell_(cell) {}
        friend class RefCell<T>;

    public:
        Ref() = default;
        ~Ref() {
            if (cell_) cell_->decr_borrow();
        }
        const T &operator*() const { return cell_->value_; }
        const T *operator->() const { return &cell_->value_; }

        // Copying allowed: increases borrow count
        Ref(const Ref &other) : cell_(other.cell_) {
            if (cell_) cell_->incr_borrow();
        }
        Ref &operator=(const Ref &other) {
            if (this == &other) return *this;
            if (cell_) cell_->decr_borrow();
            cell_ = other.cell_;
            if (cell_) cell_->incr_borrow();
            return *this;
        }

        // Move allowed: transfer without changing counts
        Ref(Ref &&other) noexcept : cell_(other.cell_) { other.cell_ = nullptr; }
        Ref &operator=(Ref &&other) noexcept {
            if (this == &other) return *this;
            if (cell_) cell_->decr_borrow();
            cell_ = other.cell_;
            other.cell_ = nullptr;
            return *this;
        }
    };

    class RefMut {
    private:
        RefCell<T> *cell_ = nullptr;

        explicit RefMut(RefCell<T> *cell) : cell_(cell) {}
        friend class RefCell<T>;

    public:
        RefMut() = default;
        ~RefMut() {
            if (cell_) cell_->end_mut_borrow();
        }
        T &operator*() { return cell_->value_; }
        T *operator->() { return &cell_->value_; }

        // Non-copyable
        RefMut(const RefMut &) = delete;
        RefMut &operator=(const RefMut &) = delete;

        // Movable
        RefMut(RefMut &&other) noexcept : cell_(other.cell_) { other.cell_ = nullptr; }
        RefMut &operator=(RefMut &&other) noexcept {
            if (this == &other) return *this;
            if (cell_) cell_->end_mut_borrow();
            cell_ = other.cell_;
            other.cell_ = nullptr;
            return *this;
        }
    };

    Ref borrow() const {
        incr_borrow();
        return Ref(this);
    }
    std::optional<Ref> try_borrow() const {
        try {
            incr_borrow();
        } catch (const BorrowError &) {
            return std::nullopt;
        }
        return Ref(this);
    }
    RefMut borrow_mut() {
        start_mut_borrow();
        return RefMut(this);
    }
    std::optional<RefMut> try_borrow_mut() {
        try {
            start_mut_borrow();
        } catch (const BorrowMutError &) {
            return std::nullopt;
        }
        return RefMut(this);
    }

    ~RefCell() noexcept(false) {
        if (borrow_count_ != 0 || mut_borrowed_) {
            throw DestructionError("RefCell destroyed while borrowed");
        }
    }
};
