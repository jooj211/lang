#ifndef INT_VALUE_HPP
#define INT_VALUE_HPP
#include "Value.hpp"
class IntValue : public Value {
public:
    int value;
    explicit IntValue(int v) : value(v) {}
    void print() const override { std::cout << value; }
};
#endif