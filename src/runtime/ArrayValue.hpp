#ifndef ARRAY_VALUE_HPP
#define ARRAY_VALUE_HPP
#include "Value.hpp"
#include <vector>

class ArrayValue : public Value
{
public:
    std::vector<Value *> elements;
    void print() const override { std::cout << "array@" << (void *)this; }
};
#endif