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

constexpr int block_size = 3;           // nof commands in block
constexpr int nof_cmds_per_writer = 5; // total number of cmds which one connection pushes
constexpr int nof_writers = 100;         // nof concurrent pushing threads

void writer(const char *log_dir, int start_num)
{
    auto handle = connect(block_size, log_dir);

    for (int i = start_num; i < start_num + nof_cmds_per_writer; ++i)
    {
        std::string cm;
        cm = "command" + std::to_string(i);

        receive(handle, cm);
    }

    disconnect(handle);
}

int main()
{
    // Clear log directory and console
    auto log_dir = "../log";
    auto res = system("clear");
    (void)res;
    const std::filesystem::directory_iterator _end;
    for (std::filesystem::directory_iterator it(log_dir); it != _end; ++it)
        std::filesystem::remove(it->path());

    // Start writers
    for (int i = 0; i < nof_writers; i++)
    {
        std::jthread(writer, log_dir, i * nof_cmds_per_writer);
    }

    return 0;
}
