// cmd_output.h
#pragma once
#include "cmd_output.h"
#include "cmd_input.h"
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>

// A type containing flag 'to_file' for output templates
struct to_file_t
{
    static const unsigned int value = 0xFFFF0000;
};

// A type containing flag 'to_console' for output templates
struct to_console_t
{
    static const unsigned int value = 0x0000FFFF;
};

constexpr unsigned int NOFLAGS = 0x00000000;

/// Stores a block of cmds
struct cmd_block_t
{

    clock_t timestamp = 0;     // time stamp for naming a file
    unsigned int output_state; // one of flags: to_file_t, to_console_t or NOFLAGS
    cmds_t cmds;               // Commands queue

    // Copy constructor
    cmd_block_t(clock_t timestamp, const cmds_t _cmds) : timestamp(timestamp), output_state(NOFLAGS)
    {
        for (auto c : _cmds)
            cmds.emplace_back(c);
    }
};

constexpr auto log_directory = "../log";

/// Async context block
inline struct output_context_t
{
    output_context_t() {}

    std::queue<cmd_block_t> cmd_blocks_q; // cmd_blocks FIFO queue
    std::mutex mtx;                       // mutex for context
    std::condition_variable cv;           // conditional variable for context
} out_ctx;

// Thread template
// block's of commands output_state == OWN_FLAG_T::value means,
// that we have already written this block into current channel (file or console)
// block's of commands output_state == FOREIGN_FLAG_T::value means,
// that we have already written this block into another, not current channel
template <typename OWN_FLAG_T, typename FOREIGN_FLAG_T>
void output_tread();

// Thread enveloppe function for output to console
// needed because it's impossible to instantiate an output_tread() template
// rigth in the std::thread definition
void thread_to_console();

// Thread enveloppe function for output to file
// needed because it's impossible to instantiate an output_tread() template
// rigth in the std::thread definition
void thread_to_file();
