#ifndef FLOAT_VALUE_HPP
#define FLOAT_VALUE_HPP
#include "Value.hpp"
class FloatValue : public Value {
public:
    float value;
    explicit FloatValue(float v) : value(v) {}
    void print() const override { std::cout << value; }
};
#endif