// cmd_output.cpp
#include "async_internal.h"
#include "cmd_output.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <cassert>
#include <memory>

/// @brief Lazy launch of output threads
/// threads are immediately detached,
/// to terminate they check at is_finishing flag
void try_to_launch_output_threads()
{
    _DF;
    std::lock_guard<std::mutex> tread_lock(output_context.out_threads_mtx);
    if (!output_context.out_threads_started)
    {
        output_context.out_threads.emplace_back(std::thread(thread_to_console)).detach();
        output_context.out_threads.emplace_back(std::thread(thread_to_file)).detach();
        output_context.out_threads.emplace_back(std::thread(thread_to_file)).detach();
        output_context.out_threads_started = true;
        // output_context.finishing.store(false);
    }
}

/// @brief Interface function, called from async.cpp (input) module
///        in purpose to put a finished block from a local input_ctx-related cmd queue
///        into output q
/// @param _input_ctx a handle to the input context
void push_block_to_out_queue(void *_input_ctx)
{
    input_blk_handle_t input_ctx = static_cast<input_blk_handle_t>(_input_ctx);

    // if zero-length block
    if (!input_ctx->block_size)
        return;

    _DF;

    // Safely put the block to the output queue tail
    {
        // 'queue_is_empty' flag dependes on both input and output edges
        std::scoped_lock lock(output_context.front_mtx, output_context.back_mtx);

        output_context.cmd_blocks_q.emplace(cmd_block_t(clock(), input_ctx->local_cmd_input_q));
        input_ctx->local_cmd_input_q.clear();
        output_context.queue_is_empty.store(false);
    }

    // This notification is to unlock waites, pending on output_context.front_mtx
    output_context.front_cv.notify_all();
}

/// @brief Writes a block of cmd's to a stream.
/// Exceptions are treated and locks are performed up the call stack
/// @return void
void write_block_to_stream(const cmd_block_t &block, std::ostream &stream)
{
    _DF;

    std::string ss("block: ");
    bool start = true;
    for (auto s : block.cmds)
    {
        if (!start)
            ss += ", ";
        else
            start = false;
        ss += s;
    }
    ss += "\n";
}

/// @brief An output_block function specialization to output into file stream
/// @param block a block of cmds to output
/// @param w fake parameter to tell console specialization from file specialization
void output_block(const cmd_block_t &block, [[maybe_unused]] to_file_t w)
{
    // _DF;
    _DS("WRITE " + block.cmds[0] + " to file");
    std::string path = std::string(log_directory)        //
                       + std::string("/bulk")            //
                       + std::to_string(block.timestamp) //
                       + std::string("_")                //
                       + this_pid_to_string()            //
                       + std::string(".log");
    try
    {
        // Lock to prepare next operation with non- empty cmd q
        std::ofstream _file(path);
        assert(_file.is_open());
        _DS("File is open");
        write_block_to_stream(block, _file);
    }
    catch (const std::exception &e)
    {
        std::cerr << "file write error" << std::endl;
        std::quick_exit(2);
    }
}

/// @brief An output_block function specialization to output to std::cout
/// @param block a block of cmds to output
/// @param w fake parameter to tell console specialization from file specialization
void output_block(const cmd_block_t &block, [[maybe_unused]] to_console_t s)
{
    _DF;
    write_block_to_stream(block, std::cout);
}

/// @brief An output thread function template
/// @tparam OWN_FLAG_T - a type, related to this thread specialization (to_console_t or to_file_t)
/// @tparam FOREIGN_FLAG_T - a type, NOT related to the specialization of this thread - is also to_console_t or to_file_t
template <typename OWN_FLAG_T, typename FOREIGN_FLAG_T>
void output_tread()
{
    std::unique_lock<std::mutex> front_lock(output_context.front_mtx);

    _DF;
    _DS("Finishing: " + std::to_string(output_context.finishing.load()) + " Is empty" + std::to_string(output_context.queue_is_empty.load()));

    while (!output_context.finishing.load() || !output_context.queue_is_empty.load())
    {
        _DS(" Shall wait with front_lock");
        output_context.front_cv.wait(front_lock, []
                                     { return !(output_context.queue_is_empty.load()) || output_context.finishing.load(); });

        while (!output_context.queue_is_empty.load())
        {
            //---------Begin proceeding output queue
            // Copy the front block
            _DS(std::string("q_size: " + std::to_string(output_context.cmd_blocks_q.size())));
            auto block = output_context.cmd_blocks_q.front();

            // Check output flags
            bool dont_output = false;
            bool have_to_notify = false;
            switch (block.output_state)
            {
            case NOFLAGS:
                _DS("NOFLAGS");
                output_context.cmd_blocks_q.front().output_state |= OWN_FLAG_T::value;
                have_to_notify = output_context.cmd_blocks_q.size() > 1;
                break;
            case (OWN_FLAG_T::value | FOREIGN_FLAG_T::value):
            case FOREIGN_FLAG_T::value:
                _DS("FOREIGN_FLAG STATE:" + std::to_string(FOREIGN_FLAG_T::value) + " FULL STATE:" + std::to_string(block.output_state));
                output_context.cmd_blocks_q.pop();
                _DS("Front block popped, q_size: " + std::to_string(output_context.cmd_blocks_q.size()));
                {
                    std::scoped_lock back_lock(output_context.back_mtx);
                    auto empty = output_context.cmd_blocks_q.empty();
                    output_context.queue_is_empty.store(empty);
                    have_to_notify = true;
                    _DS("queue_is_empty = " + std::to_string(empty));
                }
                break;
            case OWN_FLAG_T::value:
                _DS("OWN_FLAG STATE:" + std::to_string(OWN_FLAG_T::value) + " FULL STATE:" + std::to_string(block.output_state));
                dont_output = true;
                have_to_notify = true;
                break;
            default:
                _DS("unexpected state: " + std::to_string(block.output_state));
                break;
            }
            //---------End proceeding output queue

            // Unlock while performing block output

            front_lock.unlock();
            _DS("Before notyfying and blok output");
            if (have_to_notify)
            {
                _DS("Before notyfying ");
                output_context.front_cv.notify_all();
            }

            if (dont_output)
            {
                _DS("Before lock before continue");
                front_lock.lock();
                continue;
            }

            // Output block, using specialized out_blok() function
            // with fake "a" argument
            _DS("Before output a block");
            OWN_FLAG_T a;
            output_block(block, a);

            front_lock.lock();
        }
    }
    _DS("Unlock before exit tread and notify");
    front_lock.unlock();
    output_context.front_cv.notify_all(); // I'm dead, folks!
}

// Thread enveloppe function for output to console
// ;it's impossible to instantiate an output_tread() template with arguments
// right in the std::thread definition
void thread_to_console()
{
    output_tread<to_console_t, to_file_t>();
}

// Thread enveloppe function for output to file
// ;it's impossible to instantiate an output_tread() template with arguments
// right in the std::thread definition
void thread_to_file()
{
    output_tread<to_file_t, to_console_t>();
}

std::string pid_to_string(std::thread *p)
{
    return std::to_string(std::hash<std::thread::id>{}(p->get_id()));
}

std::string this_pid_to_string()
{
    return std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}
