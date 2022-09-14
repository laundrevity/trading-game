#pragma once
#include <map>
#include <cstddef>
#include <string>

class Account {
public:
    Account(const std::string& name_in,  double balance_in)
    : name(name_in), balance(balance_in) {}

    void debit(double nlv_change);
    
    void credit(double nlv_change);
    
    inline std::string get_account_name() const {
        return name;
    }

    int get_position(size_t instrument_key);

    void set_position(size_t instrument_key, int position) {
        positions[instrument_key] = position;
    }

    inline std::map<size_t, int> get_positions_map() const {
        return positions;
    }

    inline double get_balance() const {
        return balance;
    }

private:
    std::string name;
    double balance;
    std::map<size_t, int> positions;
};