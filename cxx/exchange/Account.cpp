#include "Account.h"

void Account::debit(double nlv_change) {
    balance -= nlv_change;
}

void Account::credit(double nlv_change) {
    balance += nlv_change;
}

int Account::get_position(size_t instrument_key) {
    if (positions.find(instrument_key) == positions.end()) {
        return 0;
    } else {
        return positions[instrument_key];
    }
}