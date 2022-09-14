#pragma once
#include <cmath>
#include <iostream>

enum class Limit {
    MIN = 0,
    MAX = 1
};

class Price {
    int numerator{0};
    size_t precision{2}; // should be based on tick size 
    static const size_t tick_size{1};
    int integer_part{0};
    int remainder{0};
    int ballast{1};
    bool min{false};
    bool max{false};

public:
    explicit Price(int num);
    explicit Price(Limit limit) {
        min = (limit == Limit::MIN);
        max = (limit == Limit::MAX);
    }

    Price() : Price(0) {};

    [[nodiscard]]
    double get() const {
        return static_cast<double>(integer_part) + static_cast<double>(remainder) / static_cast<double>(ballast);
    }

    [[nodiscard]]
    int get_int() const {
        return integer_part * ballast + remainder;
    }

    // assignment
    Price& operator=(const Price& other);
    // addition
    Price operator+(const Price&) const;

    friend std::ostream& operator<<(std::ostream& os, const Price& p);
    friend bool operator==(const Price& p1, const Price& p2);
    friend bool operator!=(const Price& p1, const Price& p2);
    friend bool operator<(const Price& p1, const Price& p2);
    friend bool operator>(const Price& p1, const Price& p2) {
        return p2 < p1;
    }
    friend bool operator<=(const Price& p1, const Price& p2) {
        return not (p1 > p2);
    }
    friend bool operator>=(const Price& p1, const Price& p2) {
        return not (p1 < p2);
    }
    friend double operator*(double scale, const Price& p);
};