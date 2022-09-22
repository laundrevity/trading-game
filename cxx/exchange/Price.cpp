#include "Price.h"

Price::Price(int num, size_t prec) : numerator(num), precision(prec) {
    ballast = 1;
    size_t counter = 0;
    while (counter < precision) {
        ballast = ballast * 10;
        counter += 1;
    }
    remainder = numerator % ballast;
    integer_part = (numerator - remainder) / ballast;
}

std::ostream& operator<<(std::ostream& os, const Price& price) {
    if (price.min) {
        os << "-infinity";
    } else if (price.max) {
        os << "infinity";
    } else {
        os << price.integer_part << "+";
        os << price.remainder << "/" << price.ballast;
    }
    return os;
}

bool operator==(const Price& p1, const Price& p2) {
    return (
        p1.integer_part == p2.integer_part and 
        p1.remainder == p2.remainder and 
        p1.ballast == p2.ballast
    );
}

bool operator!=(const Price& p1, const Price& p2) {
    return (
        p1.integer_part != p2.integer_part or 
        p1.remainder != p2.remainder or 
        p1.ballast != p2.ballast
    );
}

bool operator<(const Price& p1, const Price& p2) {
    if (p1.min) {
        return not p2.min;
    }
    if (p2.max) {
        return not p1.max;
    }
    if (p1.integer_part < p2.integer_part) {
        return true; // assuming remainder < ballast always
    } else if (p1.integer_part > p2.integer_part) {
        return false;
    } else {
        return p1.remainder * p2.ballast < p2.remainder * p1.ballast;
    }
}

Price& Price::operator=(const Price& other) {
    // guard self assignment
    if (this == &other) {
        return *this;
    }

    integer_part = other.integer_part;
    remainder = other.remainder;
    ballast = other.ballast;
    min = other.min;
    max = other.max;

    return *this;
}

double operator*(double scale, const Price& price) {
    return scale * price.get();
}