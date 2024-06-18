// cmd_output.h
#pragma once
#include "async_internal.h"
#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <atomic>

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

// Thread enveloppe function for output to console
// needed because it's impossible to instantiate an output_tread() template
// rigth in the std::thread definition
void thread_to_console();

// Thread enveloppe function for output to file
// needed because it's impossible to instantiate an output_tread() template
// rigth in the std::thread definition
void thread_to_file();

/// Output processes context
struct output_context_t
{
    output_context_t()
    {
        out_threads_started = false;
        finishing.store(false);
        queue_is_empty.store(true);
    }
    // Output threads pool
    std::vector<std::thread> out_threads; // the pool of output threads: one console thread and two file threads
    std::mutex out_threads_mtx;           // Output threads mutex
    bool out_threads_started;             // the flag helps to lazy start output threads with first 'receive' call
    std::atomic_bool finishing;           // a flag to finish output threads

    // Output cmd blocks queue
    std::queue<cmd_block_t> cmd_blocks_q; // cmd_blocks FIFO queue
    std::mutex front_mtx;                 // mutex for poping from queue
    std::mutex back_mtx;                  // mutex for pushing to queue
    std::condition_variable front_cv;     // condition variable to proceed the head of output queue
    std::atomic_bool queue_is_empty;      // quick 'empty' flag; doesn't reuire both head and tail locking for checking
};

inline output_context_t output_context;

using out_handle_t = std::unique_ptr<output_context_t>;

// Thread template
// block's of commands output_state == OWN_FLAG_T::value means,
// that we have already written this block into current channel (file or console)
// block's of commands output_state == FOREIGN_FLAG_T::value means,
// that we have already written this block into another, not current channel
template <typename OWN_FLAG_T, typename FOREIGN_FLAG_T>
void output_tread();

void try_to_launch_output_threads();
