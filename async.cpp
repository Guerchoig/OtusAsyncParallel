/**
 * @brief async.cpp - contains the input part of commands processing
 *                    which serve to group commands from multiple connections into blocks
 *                    and put them into a single output queue -block-by-block
 */

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
#include <utility>

/**
 * @brief Creates new connection to input commands queue
 * @param block_size - nof cmds in command block
 * @return a handle to the created connection
 */
connection_handle_t connect(std::size_t block_size, const char *log_dir = log_directory)
{
    // _DF;

    connection_handle_t handle;
    {
        std::lock_guard lock(input_connections.mtx);

        // Add new connection handle to the set of connections
        handle = input_connections.ctxs.emplace(std::make_pair(input_connections.ctxs.size(), new input_context_t(block_size))).first->first;
    }

    // Launch output threads if they are not launched yet
    output_context.th_pool.try_to_launch(log_dir);

    return handle;
}

/**
 * @brief Receives exactly one command and put it into cmd input queue
 * @param ch Handle for connection, created by connect
 * @param buf Buffer containing the command
 */
void receive(connection_handle_t ch, const std::string buf)
{

    // _DF;

    if (!buf.size())
        return;
    auto inp_ctx = input_connections.ctxs[ch];

    auto lexema = make_lexema(buf);
    int lex_id = lexema.first; // lexema.first: Lex enum,
                               // lexema.second: command string, if any, or ""
    switch (lex_id)
    {
    case Cmd:                                      // command received
        inp_ctx->cmds.emplace_back(lexema.second); // put it into local inp_ctx's queue
        if (inp_ctx->dynamic_depth == 0)
            if (inp_ctx->cmds.size() == inp_ctx->block_size)
                output_context.blocks_q.erase_push(inp_ctx->cmds); // Put into output q and clear it
        break;
    case OpenBr:                    // '{'
        (inp_ctx->dynamic_depth)++; // nested '{' are accounted to errorlessly accept nested '}'
        break;
    case CloseBr: // '}'
        if ((--(inp_ctx->dynamic_depth)) < 0)
        { // unpair close bracket!
            std::cerr << "Unpair close bracket" << std::endl;
            std::quick_exit(2);
        }
        if ((inp_ctx->dynamic_depth) == 0)                     // dynamic block is finishing
            output_context.blocks_q.erase_push(inp_ctx->cmds); // Put into output q and clear it
        break;
    default:
        std::cerr << "Unknown command" << std::endl;
        std::quick_exit(2);
        break;
    };
}

/**
 * @brief Thread-safe delete of connection
 * @param ch connection handle
 * @return true if the connection was in pool and was deleted
 */
bool input_connections_t::delete_connection(connection_handle_t ch)
{
    std::lock_guard(input_connections.mtx);
    if (input_connections.ctxs.contains(ch))
    {
        input_connections.ctxs.erase(ch);

        return true;
    }
    return false;
}

/**
 * @brief tread-safe empty() function for input connections pool
 * @return
 */
bool input_connections_t::empty()
{
    std::lock_guard(input_connections.mtx);
    return input_connections.ctxs.empty();
}

/**
 * @brief Delete connection corresponding to the given handle,
 *        form a block from the rest of input cmd queue
 *        and pushes it into output blocks queue
 *        If that was the last connection, then
 *        - initiate termination of output threads
 * @param ch Handle for connection, created by connect
 */
void disconnect(connection_handle_t ch)
{
    // _DF

    auto inp_ctx = input_connections.ctxs[ch];

    // Push the last block to output queue
    output_context.blocks_q.erase_push(inp_ctx->cmds);

    // Delete connection
    auto res = input_connections.delete_connection(ch);
    (void)res;
}

/**
 * @brief Process input command to create lexema (token + command)
 * @param buf The buffer, which command comes from
 * @return Created lexema
 */
lexema_t make_lexema(const std::string buf)
{
    if (buf.find(open_br_sym) != buf.npos)
        return std::make_pair(OpenBr, std::string(1, open_br_sym));
    if (buf.find(close_br_sym) != buf.npos)
        return std::make_pair(CloseBr, std::string(1, close_br_sym));
    return std::make_pair(Cmd, buf);
}
