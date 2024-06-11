// cmd_input.h
#pragma once
#include <string>
#include <vector>
#include <mutex>

// special symbols
constexpr unsigned char end_sym = 0x04; //^D
constexpr unsigned char open_br_sym = '{';
constexpr unsigned char close_br_sym = '}';

/// Enum for lexemas
enum Lex : unsigned int
{
    Nope,    // no command
    OpenBr,  // open bracket
    CloseBr, // close bracket
    Cmd,     // command
    CmdEnd   // End of commands
};

/// @brief A type for lexemas: Enum + string
using lexema_t = std::pair<enum Lex, std::string>;

// Cmds buffer type (commands are strings)
using cmds_t = std::vector<std::string>;

struct input_context_t
{
    size_t block_size; // command block size
    cmds_t inp_cmd_q;  // commands temporary buffer in which block cmds are being accumulated
    std::mutex mtx;    // mutex for context
    void *out_handle;  // output context
    explicit input_context_t(size_t block_size) : block_size(block_size) {}
};
using input_handle_t = input_context_t *;

/// @brief Takes one command from buf;
///  accumulate commands in temporary ctx->cmds_buf
///  when meet a condition - forms a block of commands and put it into ctx->cmd_blocks_q
/// @param ctx
/// @param buf
void interpret_cmd_buf(input_handle_t ctx, const std::string buf);
