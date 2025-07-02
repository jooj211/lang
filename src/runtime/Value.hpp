#ifndef VALUE_HPP
#define VALUE_HPP
#include <iostream>
class Value {
public:
    virtual ~Value() = default;
    virtual void print() const = 0;
};
#endif