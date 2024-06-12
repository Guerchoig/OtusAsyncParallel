
// libasync.cpp

#include "cmd_input.h"
#include "cmd_output.h"
#include "async.h"
#include <chrono>
#include <string>
#include <thread>

void *connect(std::size_t block_size)
{
    std::lock_guard<std::mutex> lock(input_ctx_collection.mtx);
    auto el = input_ctx_collection.ctxs.emplace(new input_context_t(block_size));
    return *(el.first);
}

void receive(void *_ctx, const char *data, std::size_t size)
{
    input_handle_t ctx = (input_handle_t)_ctx;
    std::lock_guard<std::mutex> lock(ctx->mtx);
    interpret_cmd_buf(ctx, std::string(data, size));
}

void disconnect(void *_ctx)
{
    input_handle_t inp_ctx = static_cast<input_handle_t>(_ctx);
    {
        std::lock_guard<std::mutex> lock(inp_ctx->mtx);
        interpret_cmd_buf(inp_ctx, std::string(1, end_sym));
        {
            std::lock_guard<std::mutex> llock(input_ctx_collection.mtx);
            input_ctx_collection.ctxs.erase(inp_ctx);
            delete inp_ctx;
        }
    }
    out_ctx.cv.notify_all();
}
