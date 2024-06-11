/// @brief parse_cmd.cpp This module contains command processing, performed in the main pool
///

#include "cmd_input.h"
#include <utility>
#include <stack>
#include <mutex>
#include <queue>
#include <thread>
#include <queue>
#include <vector>
#include <iostream>
#include <cstdlib>

/// Makes lexema from buf content:
/// std::pair<enum Lex, std::string>

lexema_t make_lexema(const std::string buf)
{
    if (buf.find(open_br_sym) != buf.npos)
        return std::make_pair(OpenBr, std::string(1, open_br_sym));
    if (buf.find(close_br_sym) != buf.npos)
        return std::make_pair(CloseBr, std::string(1, close_br_sym));
    if (buf.find(end_sym) != buf.npos)
        return std::make_pair(CmdEnd, std::string(1, end_sym));
    return std::make_pair(Cmd, buf);
}

// Output function declaration
void push_block(void *_input_ctx, const size_t cmd_buf_size);

/// Groupps commands from buf into blocks and put blocks to queue;
/// called from lockguard protected scope in receive func
void interpret_cmd_buf(input_handle_t ctx, const std::string buf)
{
    // Finite automate only state value
    static int dynamic_depth{0};

    auto lexema = make_lexema(buf);
    int lex_id = lexema.first; // lexema.first: Lex enum,
                               // lexema.second: command string, if any, or ""
    switch (lex_id)
    {
    case Cmd: // command
        ctx->inp_cmd_q.emplace_back(lexema.second);
        if (dynamic_depth == 0)
            if (ctx->inp_cmd_q.size() == ctx->block_size)
                push_block(ctx, ctx->inp_cmd_q.size());
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
        if (dynamic_depth == 0) // dynamic block is finished
            push_block(ctx, ctx->inp_cmd_q.size());
        break;
    case CmdEnd:
        push_block(ctx, ctx->inp_cmd_q.size());
        break;
    };
}
