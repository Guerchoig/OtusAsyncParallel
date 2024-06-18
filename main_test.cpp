// main_test.cpp
#include "async.h"
#include <thread>
#include <vector>
#include <cstring>
#include <iostream>
#include <fstream>
#include <cassert>
#include <filesystem>
#include <stdlib.h>
#include <chrono>

constexpr auto log_directory = "../log";
constexpr int block_size = 3;
constexpr int nof_cmds = 7;

int main()
{
    // Clear log directory and console
    system("clear");
    const std::filesystem::directory_iterator _end;
    for (std::filesystem::directory_iterator it(log_directory); it != _end; ++it)
        std::filesystem::remove(it->path());

    auto handle = connect(block_size);
    // std::vector<std::string> commands = {"command1", "command2", "command3", "command4", "command5", "command6", "command7"};

    for (int i = 1; i <= nof_cmds; ++i)
    {
        std::string cm;
        cm = "command" + std::to_string(i);

        receive(handle, cm);
    }

    // using namespace std::this_thread;     // sleep_for, sleep_until
    // using namespace std::chrono_literals; // ns, us, ms, s, h, etc.
    // using std::chrono::system_clock;

    // sleep_until(system_clock::now() + 15s);

    disconnect(handle);

    return 0;
}
