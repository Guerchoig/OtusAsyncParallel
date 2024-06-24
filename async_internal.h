/**
 * @brief async_internal.h - async library internal definitions for commands input
 */

#pragma once
#include "async.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <thread>

// #define DEBUG

std::string pid_to_string(std::thread *p);
std::string this_pid_to_string();

#ifdef DEBUG
std::string pid_to_string(std::thread *p);
#define _DS(s) std::cout << (this_pid_to_string() + " " + s + "\n");
#define _DF _DS(__PRETTY_FUNCTION__)
#else
#define _DS(s) ;
#define _DF ;
#endif

// Special symbols for input cmd stream
constexpr unsigned char open_br_sym = '{';
constexpr unsigned char close_br_sym = '}';

/**
 * @brief Tokens for lexemas
 */
enum Lex : unsigned int
{
    OpenBr,  // open bracket received
    CloseBr, // close bracket received
    Cmd      // command
};

/**
 * @brief Lexema type
 */
using lexema_t = std::pair<enum Lex, std::string>;

/**
 * @brief Cmds input buffer type (commands are strings)
 */
using cmds_t = std::vector<std::string>;

/**
 * @brief An input context; provide input block size and realize input queue
 */
struct input_context_t
{
    size_t block_size; // command block size
    cmds_t cmds;       // commands temporary buffer in which block cmds are being accumulated
    int dynamic_depth; // needed to follow using of brackets
    explicit input_context_t(size_t block_size) : block_size(block_size), dynamic_depth{0} {}
};

/**
 * @brief A ptr to input context
 */
using sp_input_context_t = std::shared_ptr<input_context_t>;

/**
 * @brief A type for pool of input connections
 */
struct input_connections_t
{
    std::mutex mtx; // A mutex for calling input interface methods from multiple threads
    std::unordered_map<connection_handle_t, sp_input_context_t> ctxs;
    bool delete_connection(connection_handle_t ch);
    bool empty();
};

/**
 * @brief An pool of input connections
 */
inline input_connections_t input_connections;

lexema_t make_lexema(const std::string buf);

///  Auxillaries  //////////
inline std::string pid_to_string(std::thread *p)
{
    return std::to_string(std::hash<std::thread::id>{}(p->get_id()));
}

inline std::string this_pid_to_string()
{
    return std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}
