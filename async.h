// async.h
#pragma once
#include <memory>
#include <queue>
#include <condition_variable>

/// @brief Connect function receive command block size
/// @param block_size
/// @return context
void *connect(std::size_t block_size);

/// @brief Receive cmd_blocks
/// @param ctx Context handle
/// @param data Command text (one command)
/// @param size Command string length
void receive(void *_ctx, const std::string data);

/// @brief Disconnect func
/// @param ctx
void disconnect(void *_ctx);
