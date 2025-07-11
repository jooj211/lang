#pragma once
// Ative definindo -DDEBUG_PRINT no cmake ou g++
#ifdef DEBUG_PRINT
#include <iostream>
#define DBG(msg) (std::cerr << "[DBG] " << msg << '\n')
#else
#define DBG(msg) ((void)0)
#endif