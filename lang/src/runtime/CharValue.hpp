#ifndef CHAR_VALUE_HPP
#define CHAR_VALUE_HPP
#include "Value.hpp"
class CharValue : public Value {
public:
    char value;
    explicit CharValue(char v) : value(v) {}
    void print() const override { std::cout << value; }
};
#endif