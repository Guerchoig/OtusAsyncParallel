// cmd_input.h
#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <thread>

#define DEBUG

std::string this_pid_to_string();
#ifdef DEBUG
std::string pid_to_string(std::thread *p);
#define _DS(s) std::cout << (this_pid_to_string() + " " + s + "\n")
#define _DF _DS(__PRETTY_FUNCTION__)

#else
#define _DPN ;
#define _DSN(s) ;
#endif

// special symbols
constexpr unsigned char end_sym = 0x04; //^D
constexpr unsigned char open_br_sym = '{';
constexpr unsigned char close_br_sym = '}';

/// Enum for lexemas
enum Lex : unsigned int
{
    OpenBr,  // open bracket received
    CloseBr, // close bracket received
    Cmd      // command
};

/// A type for lexemas
using lexema_t = std::pair<enum Lex, std::string>;

// Cmds input buffer type (commands are strings)
using cmds_t = std::vector<std::string>;

// An input context, providing input block size and input queue
struct input_context_t
{
    size_t block_size;        // command block size
    cmds_t local_cmd_input_q; // commands temporary buffer in which block cmds are being accumulated
    explicit input_context_t(size_t block_size) : block_size(block_size) {}
};
// A type for input context handle
using input_blk_handle_t = input_context_t *;

// A pool of input contexts
struct input_ctx_collection_t
{
    std::mutex mtx; // A mutex for calling input interface methods from multiple threads
    std::unordered_set<input_blk_handle_t> ctxs;
};

inline input_ctx_collection_t input_connections;

/// @brief Takes one command from buf;
///  accumulate commands in temporary ctx->cmds_buf
///  when meet a condition - forms a block of commands and put it into ctx->cmd_blocks_q
/// @param ctx
/// @param buf
void queue_a_command(input_blk_handle_t ctx, const std::string buf);

// Output interface declaration
void push_block_to_out_queue(void *_input_ctx);

lexema_t make_lexema(const std::string buf);
