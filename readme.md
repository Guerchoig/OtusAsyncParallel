## Outline
A command processor, receive commands from external connections and send them 
in an asynchroneous & multithread way to files and console

## Input of commands
Several threads, which produce commands, do connect to library core by calling 'connect()'
and then send commands to the input queue by calling 'receive()'.
Every connection has it's own commands queue, when a connection meet end-block condition
it pushes the block into the output blocks queue, which is one common queue for all the connections.

## Output of commands
The first call of 'connect()' starts three output threads 2-'to file' and 1 - 'to console'.
When an output thread receive a signal, that a new block is put into queue,
or that operation has to be stopped,
it peeks several blocks in queue, which meet output conditions, and outputs their copies to file or console.
The thread leaves blocks in queue, but marks them as 'output to file' or 'output to console'.

## Erasing blocks from queue
The blocks, which are marked as output both to file and to console are erased from queue at the moment, 
when a new block is pushed into queue.

## Stopping operation
When a connection call 'disconnect()', it drops the rest of commands into output blocks queue and disconnect from connections pool.

