/// @brief async.cpp
///        Contains library interface implementation,
///        specifically: the 'input' part of commands processor,
///        which serve to groups commands from multiple connections into blocks
///        and put them into a single output queue -block-by-block

#include "async_internal.h"
#include "cmd_output.h"
#include "async.h"
#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <mutex>
#include <queue>
#include <vector>
#include <iostream>
#include <cstdlib>

// Just a shortcut for a long type
using inp_ctxs_iterator_t = decltype(input_connections.ctxs.emplace(input_context_t(size_t())));

/// @brief Create a new connection to commands processor
///        include a new local commands queue
/// @param block_size - size of commands block for this connection
/// @return 'depersonalized' (for incapsulation sake) pointer to new context
void *connect(std::size_t block_size)
{
    _DF;
    inp_ctxs_iterator_t elem;
    {
        std::lock_guard<std::mutex> lock(input_connections.mtx);
        if (output_context.finishing.load())
            // Can't connect anymore
            return nullptr;

        // Add new connection handle to the set of connections
        elem = input_connections.ctxs.emplace(new input_context_t(block_size));
    }

    // Launches output threads if they haven't been launched yet
    // If  a block is pushed to out queue - OK it will remain there until
    // at least one output thread is launcded
    try_to_launch_output_threads();

    return *(elem.first);
}

/// @brief Receive and parse a command to store in local commands queue
/// @param _ctx -connection handle
/// @param buf  - contains one command ctring or one bracket
void receive(void *_ctx, const std::string buf)
{

    if (!buf.size())
        return;
    input_blk_handle_t inp_ctx = static_cast<const input_blk_handle_t>(_ctx);

    static int dynamic_depth{0};
    // _DF;
    auto lexema = make_lexema(buf);
    int lex_id = lexema.first; // lexema.first: Lex enum,
                               // lexema.second: command string, if any, or ""
    switch (lex_id)
    {
    case Cmd:                                                   // command received
        inp_ctx->local_cmd_input_q.emplace_back(lexema.second); // put it into local inp_ctx's queue
        if (dynamic_depth == 0)
            if (inp_ctx->local_cmd_input_q.size() == inp_ctx->block_size)
                push_block_to_out_queue(inp_ctx); // Put into output q and clear it
        break;
    case OpenBr:         // '{'
        dynamic_depth++; // nested '{' are accounted to errorlessly accept nested '}'
        break;
    case CloseBr: // '}'
        if (--dynamic_depth < 0)
        { // unpair close bracket!
            std::cerr << "Unpair close bracket" << std::endl;
            std::quick_exit(2);
        }
        if (dynamic_depth == 0) // dynamic block is finishing
            push_block_to_out_queue(inp_ctx); // Put into output q and clear it
        break;
    default:
        _DS("Unknown cmd");
        std::cerr << "Unknown command" << std::endl;
        std::quick_exit(2);
        break;
    };
}

void disconnect(void *_ctx)
{

    input_blk_handle_t inp_ctx = static_cast<input_blk_handle_t>(_ctx);

    // Push the last block to output queue
    push_block_to_out_queue(inp_ctx);

    // Delete connection
    std::unique_lock<std::mutex> inp_lock(input_connections.mtx);
    input_connections.ctxs.erase(inp_ctx);
    delete inp_ctx;
    _DF;

    // If it was the last connection
    if (input_connections.ctxs.empty())
    {
        output_context.finishing.store(true);
        // std::lock_guard<std::mutex> lock(output_context.front_mtx);
        // inp_lock.unlock();

        // for (auto pprc = output_context.out_threads.begin(); pprc != output_context.out_threads.end(); pprc++)
        // {
        //     if (pprc->joinable())
        //     {
        //         _DS(" joinable");
        //         pprc->join();
        //     }
        // }
    }
}

lexema_t make_lexema(const std::string buf)
{
    if (buf.find(open_br_sym) != buf.npos)
        return std::make_pair(OpenBr, std::string(1, open_br_sym));
    if (buf.find(close_br_sym) != buf.npos)
        return std::make_pair(CloseBr, std::string(1, close_br_sym));
    return std::make_pair(Cmd, buf);
}
