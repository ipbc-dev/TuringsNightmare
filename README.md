# TuringsNightmare (TN)
An experimental PoW algorithm developed by IPBC
Intended to be ASIC resistant and to favor CPU over GPU

Idea based on (Core War)[https://en.wikipedia.org/wiki/Core_War].

## Concept
The idea of the algorithm is to have a "large" memory-pad, into which input data is loaded, and then using a "Virtual Machine" to interprete each byte in memory as an "instruction".
The instructions themselves all do different things to alter the state of the memory-pad and of the VM ("registers", instruction pointer, steplimit...)
At the end of the execution loop, all VM State and the memory-pad are hashed with one of 3 hashing algorithms, giving the end result.

Reading the code should give you a good idea of the intent.
