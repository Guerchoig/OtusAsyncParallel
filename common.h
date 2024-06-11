#pragma once
#include <ctime>
#include <vector>
#include <string>

#include <condition_variable>

#include <memory>

#define DEBUG



// Debug-prints s-string preceeded by the current thread id
void debug_print(std::string s);

// Debug print macro
#ifdef DEBUG
#define DEBUG_PRINT debug_print(__PRETTY_FUNCTION__);
#else
#define DEBUG_PRINT ;
#endif
