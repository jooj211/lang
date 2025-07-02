#ifndef BOOL_VALUE_HPP
#define BOOL_VALUE_HPP
#include "Value.hpp"
class BoolValue : public Value {
public:
    bool value;
    explicit BoolValue(bool v) : value(v) {}
    void print() const override { std::cout << (value ? "true" : "false"); }
};
#endif