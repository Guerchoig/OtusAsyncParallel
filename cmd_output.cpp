
// #define LOCAL_INOUT
#include "cmd_input.h"
#include "cmd_output.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <cassert>

// Push into queue the block, constructed from commands in ctx->cmds_buf;
// called from threadsafe scope
void push_block(void *_input_ctx, const size_t cmd_buf_size)
{
    input_handle_t input_ctx = static_cast<input_handle_t>(_input_ctx);
    output_handle_t out_ctx = static_cast<output_handle_t>(input_ctx->out_handle);
    if (!cmd_buf_size)
        return;
    {
        std::lock_guard<std::mutex> lock(out_ctx->mtx);
        out_ctx->cmd_blocks_q.emplace(cmd_block_t(clock(), input_ctx->inp_cmd_q));
        input_ctx->inp_cmd_q.clear();
    }
    out_ctx->cv.notify_all();
}

/// @brief Writes a block of cmd's to a stream.
/// Exceptions are treated and locks are performed uper on the call stack
/// @return void
void write_block_to_stream(const cmd_block_t &block, std::ostream &stream)
{
    stream << "block: ";
    bool start = true;
    for (auto s : block.cmds)
    {
        if (!start)
            stream << ", ";
        else
            start = false;
        stream << s;
    }
    stream << '\n';
}

// Does prepare file to output a blok to it
void output_block(const cmd_block_t &block, [[maybe_unused]] to_file_t w)
{
    std::string path = std::string(log_directory)                                                 //
                       + std::string("/bulk")                                                     //
                       + std::to_string(block.timestamp)                                          //
                       + std::string("_")                                                         //
                       + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) //
                       + std::string(".log");
    std::ofstream _file(path);
    assert(_file.is_open());
    write_block_to_stream(block, _file);
}

// Perform zero-preparation to write to std::cout
void output_block(const cmd_block_t &block, [[maybe_unused]] to_console_t s)
{
    write_block_to_stream(block, std::cout);
}

/// Output a block in blocks queue to console
/// @param ctx - async control handle
template <typename OWN_FLAG_T, typename FOREIGN_FLAG_T>
void output_tread(output_handle_t ctx)
{
    std::unique_lock<std::mutex> blocks_queue_lock(ctx->mtx);
    while (!ctx->finished)
    {
        ctx->cv.wait(blocks_queue_lock, [ctx]
                     { return !ctx->cmd_blocks_q.empty() || ctx->finished; });

        while (!ctx->cmd_blocks_q.empty())
        {
            // Copy the front block
            auto block = ctx->cmd_blocks_q.front();

            // Check output flags
            switch (block.output_state)
            {
            case NOFLAGS:
                // leave it in queue and output to curr channel
                ctx->cmd_blocks_q.front().output_state |= OWN_FLAG_T::value;
                break;
            case FOREIGN_FLAG_T::value:
                // Delete from queue: output to foreign channel is already done,
                // output to curr channel will be done soon
                ctx->cmd_blocks_q.pop();
                break;
            case OWN_FLAG_T::value:
                // Output to current channel is already done, nothing to do here
                return;
            }
            // The block we do output is copied,
            // so we can unlock queue for other threads
            blocks_queue_lock.unlock();

            try
            {
                // Output block, using specialized out_blok() function
                // with fake "a" argument
                OWN_FLAG_T a;
                output_block(block, a);
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << std::endl;
            }
            // lock mutex to verify ctx->finished and for ctx->cv.wait
            // if ctx->finished == true, the mutex will unlock when leaving
            blocks_queue_lock.lock();
        }
    }
}

// Thread enveloppe function for output to console
// ;it's impossible to instantiate an output_tread() template with arguments
// right in the std::thread definition
void thread_to_console(output_handle_t ctx)
{
    output_tread<to_console_t, to_file_t>(ctx);
}

// Thread enveloppe function for output to file
// ;it's impossible to instantiate an output_tread() template with arguments
// right in the std::thread definition
void thread_to_file(output_handle_t ctx)
{
    output_tread<to_file_t, to_console_t>(ctx);
}

void debug_print(std::string s)
{
    std::cout << std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) << " " << s << '\n';
    return;
}
