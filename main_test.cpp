// test_main.cpp
#include "async.h"
#include <thread>
#include <vector>
#include <cstring>
#include <iostream>
#include <fstream>
#include <cassert>
#include <filesystem>

constexpr auto log_directory = "../log";

int main()
{
    // Clear log directory
    const std::filesystem::directory_iterator _end;
    for (std::filesystem::directory_iterator it(log_directory); it != _end; ++it)
        std::filesystem::remove(it->path());

    auto handle = connect(5);
    std::vector<std::string> commands = {"command1", "command2", "command3", "command4", "command5", "command6", "command7"};
    for (auto cm : commands)
        receive(handle, cm.c_str(), cm.size());
    disconnect(handle);

    return 0;
}
