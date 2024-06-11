## Architecture
The system is based on several input queue and one output queue
- input queue ("cmd_input.h" struct input_context_t.inp_cmd_q) - a vector of string commands
- output queue ("cmd_output.h" struct output_context_t.cmd_blocks_q) - a FIFO std::queue of blocks of commands

Output queue and the collection of input queues are singletons.

Every call for 'connect' create a new input queue, which is supposed to contain some consequtive commands flow.\
The order of commands in one command block is preserved up to the output and can't interleave with commands of other blocks\
The order of blocks of one queue is also preserved, but those blocks can interleave with blocks from other input queues.\

Input queues are processed by the command input threads; one input context handle corresponds to one input queue 

queue context types are
- struct 'input_context_t' with 'input_handle_t' as handle
- struct 'output_context_t' with  with 'output_handle_t' as handle

## Scenario
Thread-safe 'receive' function accumulates commands in input queue.\
When it meet an appropriate condition, it writes block of accumulated commands into output queue and clears input queue\

Two file threads (thread_to_file) and one console thread (thread_to_console) are set to awake, when a new block appears in the output queue and output it to file and to console,\

then the front block of output queue, which is marqued by both
- to_file_t::value flag 
- and to_console_t::value flag 
is pop-ed from queue, because it's already written to file and is shown to console

## Initialization
When an input thread calls 'connect' func it creates
- a new input context block, containing input queue
- and the output context block if is was not created earlier, which contains output queue 

## Finaization
The 'disconnect' function send termination command into the corresponding input queue.\ That command put the last commands block into output queue and delete input context
