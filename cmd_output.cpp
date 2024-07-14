/**
 * @brief cmd_output.cpp - realizes async output queue
 */
#include "async_internal.h"
#include "cmd_output.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <cassert>
#include <memory>

/**
 * @brief Lazy launch of output threads
 * @param log_dir Path for output files
 */
void out_threadpool_t::try_to_launch(const char *log_dir)
{
    // _DF;

    std::lock_guard tread_lock(output_context.th_pool.mtx);
    if (!output_context.th_pool.threads_started)
    {
        output_context.th_pool.log_dir = log_dir;
        output_context.th_pool.pool.emplace_back(std::thread(thread_to_console)).detach();
        output_context.th_pool.pool.emplace_back(std::thread(thread_to_file)).detach();
        output_context.th_pool.pool.emplace_back(std::thread(thread_to_file)).detach();

        output_context.th_pool.threads_started = true;
    }
}

/**
 * @brief Inprotectedly writes a block of cmd's to a stream
 * @param block The block to output
 * @param stream Output stream
 */
void write_block_to_stream(const cmd_block_t &block, std::ostream &stream)
{
    _DF;

    std::string ss("block: ");
    bool start = true;
    for (auto s = block.cmds.begin(); s != block.cmds.end(); s++)
    {
        if (!start)
            ss += ", ";
        else
            start = false;
        ss += *s;
    }
    ss += "\n";
    stream << ss;
}

/**
 * @brief Output several command blocks to console at a time
 */
void thread_to_console()
{
    // _DF;

    std::vector<cmd_block_t> blocks;
    while (output_context.blocks_q.fetch_blocks<TO_CONS>(blocks))
    {
        for (auto pblock = blocks.begin(); pblock != blocks.end(); ++pblock)
            if (pblock->cmds.size())
                write_block_to_stream(*pblock, std::cout);
        blocks.clear();
    }
}

/**
 * @brief Output several command blocks to file at a time
 */
void thread_to_file()
{
    // _DF;
    std::vector<cmd_block_t> blocks;
    while (output_context.blocks_q.fetch_blocks<TO_FILE>(blocks))
    {
        for (auto pblock = blocks.begin(); pblock != blocks.end(); ++pblock)
        {
            if (pblock->cmds.size())
            {
                std::string path = std::string(log_directory)          //
                                   + std::string("/bulk")              //
                                   + std::to_string(pblock->timestamp) //
                                   + std::string("_")                  //
                                   + this_pid_to_string()              //
                                   + std::string(".log");
                try
                {
                    // Lock to prepare next operation with non- empty cmd q
                    std::ofstream _file(path);
                    assert(_file.is_open());

                    write_block_to_stream(*pblock, _file);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "file write error" << std::endl;
                    std::quick_exit(2);
                }
            }
        }
        blocks.clear();
    }
}

/**
 * @brief Erases already output blocks from queue
 *        and pushs a new block into queue
 * @param cmds Block of commands to push
 */
void cmd_blocks_q_t::erase_push(cmds_t &cmds)
{
    // _DF;

    // Erase
    std::unique_lock lock(common_mtx);
    for (auto p = lst.begin(); p != lst.end(); ++p)
    {
        if (p->output_state.load() == (TO_FILE::value | TO_CONS::value))
        {
            lst.erase(p); // pop from queue
            break;
        }
    }

    if (!cmds.size())
        return;
    if (output_context.finishing.load())
        return;

    // Push
    lst.emplace_back(cmds); // timestamp is set in constructor
    cmds.clear();

    cv.notify_all();
}

template <typename T>
void filelock([[maybe_unused]] std::unique_lock<std::mutex> &fl) {}

template <TO_FILE>
void filelock(std::unique_lock<std::mutex> &fl)
{
    fl.lock();
}

/**
 * @brief Copy ready-to-be-written blocks for output them
 *        and establishes their output states in queue
 * @tparam T     - TO_FILE or TO_CONS
 * @param blocks a place where to copy found blocks
 * @return       true if the queue is still worth to be processed
 */
template <typename T>
bool cmd_blocks_q_t::fetch_blocks(std::vector<cmd_block_t> &blocks)
{
    // _DF;

    std::shared_lock<std::shared_mutex> shared_lock(common_mtx);
    cv.wait(shared_lock,
            []()
            {
                if (!output_context.blocks_q.lst.empty())
                    return true;
                if (output_context.finishing.load())
                    return true;
                return false;
            });
    if (!lst.empty())
    {
        for (auto p_block = lst.begin(); p_block != lst.end(); p_block++)
        {
            std::unique_lock fl(file_mtx, std::defer_lock);
            filelock<T>(fl);

            auto cur_state = p_block->output_state.load();
            if (cur_state & T::value)
                continue; //
            p_block->output_state.store(cur_state |= T::value);

            blocks.emplace_back(*p_block); // copy block
            break;
        }
    }
    shared_lock.unlock();
    cv.notify_all();
    return !(lst.empty() && output_context.finishing.load());
}
