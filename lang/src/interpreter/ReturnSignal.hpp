#ifndef RETURN_SIGNAL_HPP
#define RETURN_SIGNAL_HPP
#include "../runtime/Value.hpp"
#include <vector>
#include <stdexcept>
class ReturnSignal : public std::exception {
public:
    std::vector<Value*> values;
};
#endif