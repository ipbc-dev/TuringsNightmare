## NOTE
Everything here is subject to change. It should give a good overview of the principle of the idea, though.  
The code here is already slightly out of date.

## Core Idea
The basic idea of TuringsNightmare (TN) as a PoW is that instead of always taking the same steps, the path of execution in the algorithm (what gets executed, in what order, and what changes because of it) is completely dependent on the input data in unpredictable ways.   
The reasoning behind this is that GPU's are not great at diverging branches and threads taking different amount of steps to complete, and ASIC's implementing all the dynamics of the algorithm would be exceedingly expensive to produce, and likely inefficient, while CPU's do just fine.   

## General outline of algorithm
Start by allocating VM_State to hold all state information and the memory buffer (currently fixed size of 1MB).  
Fill the memory buffer with input data (currently just copied into the buffer as often as it will fit, will likely be changed to AES generation like in cryptonight or something similar).  
Initialize the keccak part of VM_State by hashing the current content of the memory buffer. These keccak state values will be used later.  
The execution loop will run a certain amount of steps. This amount is not fixed, so we set up a minimum, maximum, and starting step limit. The current step limit will change during execution, staying beetween min and max.  
Exact numbers on how these limits will be calculated are not decided yet.  
Enter the execution loop:  
* Check that state.step_counter <= state.step_limit, exit loop if not.
* Grab a byte from the location in the memory buffer indexed by state.instruction_ptr (starts at 0).
* Based on the byte (xored with parts of the state), decide which instruction to execute.
* Apply the instruction to the state, modifying the memory buffer and potentially varying parts of state. (Pseudorandom memory changes, state registers altered, instruction_ptr changed to cause jumps, step_limit increased/decreased, etc).
* Increment state.instruction_ptr.
* Increment state.step_counter.
After the execution loop is finished, hash VM_State + Memory buffer with one of a few (currently 3) hashing algorithms (like in cryptonight), choosing of which is dependent on VM_State, and return the result.  

## Annotated Code Walkthrough (Code as of time of writing)
* Here is the main function you would call to calculate the TN hash of an input:

```c++
// Takes a pointer to input buffer, input buffer length, and a pointer to where the 32 byte hash should be left (allocated by caller). Return 1 for success, 0 for error.
int Turings_Nightmare(const char* in, const size_t in_len, char* out) {
  if (in_len == 0) { return 0; }                  // Sanity check, cannot with with 0 length data
  else if (in_len >= MEMORY_SIZE) { return 0; }   // Sanity check, since memory size is fixed, and we fill it with the input data, input length cannot exceed MEMORY_SIZE (as it stands now)

  // Initialize VM_State and memory
  VM_State *mem = new VM_State;
  VM_State &state = *mem;

  memset(mem, 0, sizeof(VM_State)); // Set everything in VM_State (and memory) to 0
  state.memory_size = MEMORY_SIZE;  // Let state know the memory_size
  state.input_size = in_len;        // Let state know the original input's size

  // Fill memory with input data (currently very simplistic)
  int times = MEMORY_SIZE / in_len;                   // Calculate how often input fits into memory
  for (int x = 0; x < times; x++) {                   // For how often it fits in memory...
    memcpy(state.memory + (x * in_len), in, in_len);  // ...copy input data into memory
  }

  size_t filled = times * in_len;      // Check if there is still space for data
  if (filled != state.memory_size) {   // If so...
    memcpy(state.memory + filled, in, state.memory_size - filled); // ...fill the remaining space with a chunk of input data
  }

  // Run keccak hash over the memory buffer, storing the state of the hashing algorithm in state.hs
  keccak1600(state.memory, state.memory_size, state.hs.b);

  // Set step limits (see below for definitions)
  state.step_limit_max = state.memory_size * MAX_CYCLES; // Maximum number of steps 
  state.step_limit = state.memory_size * NRM_CYCLES;     // Starting step limit
  state.step_limit_min = state.memory_size * MIN_CYCLES; // Minimum number of steps

  // Execution Loop
  for (; state.step_counter <= state.step_limit; state.step_counter++) {     // At the start of each loop, check we are below step_limit. And the end of each loop, increment step_counter
    VM_Instruction inst = TN_GetInstruction(state);                          // Get instruction for current state, based on memory location indexed by instruction_ptr (Detailed below)
    TN_ParseInstruction(state, inst);                                        // Parse instruction (Detailed below)
    state.instruction_ptr = (state.instruction_ptr + 1) % state.memory_size; // Increment instruction_ptr for the next loop, bound by memory_size
  }

  // Hash state (and memory) with one of 3 hashing algorithms, chosen based on state, and leave the resulting 32 byte hash in out
  TN_FinalHash(state, (uint8_t*)mem, sizeof(VM_State), (uint8_t*)out);

  delete mem; // Cleanup state + memory
  return 1;   // Return 1 for successfull operation
}

```

* Here is the struct that defines VM_State:

```c++
#define MIN_CYCLES 1                // This * Memory_Size == Minimum steps
#define NRM_CYCLES 2                // This * Memory_Size == Starting step_limit
#define MAX_CYCLES 10               // This * Memory_Size == Maximum steps
#define MEMORY_SIZE 1024 * 1024 * 1 // Memory buffer size (fixed, subject to change)

typedef struct {
  uint64_t instruction_ptr;    // Where in memory should we look for the next instruction to execute / what are we currently executing?
  size_t step_counter;         // How many steps have we taken? (Loops of executor function)
  size_t step_limit;           // How many steps are we currently allowed to take?

  size_t step_limit_max;       // Maximum number of step_limit
  size_t step_limit_min;       // Minimum number of step_limit

  size_t input_size;           // How big was the input given to us at the start?
  size_t memory_size;          // How big is our memory buffer?
  hash_state hs;               // Where keccak stores it's resulting state
  
  uint64_t register_a;         // Registers, used for ENTANGLED_UINT and various instructions
  uint64_t register_b;
  uint64_t register_c;
  uint64_t register_d;

  uint8_t memory[MEMORY_SIZE]; // The memory buffer itself.
} VM_State;
```

* The function that decided the instruction, given current VM_State. In essence, take the byte at memory[instruction_ptr], Xor it with ENTANGLED_UINT64 (see below), bounded by how many instructions there are. The resulting number is the index of the instruction to execute.

```c++
inline VM_Instruction TN_GetInstruction(const VM_State& state) {
  return (VM_Instruction)((state.memory[state.instruction_ptr] ^ ENTANGLED_UINT64) % _LAST);
}
```

* The enum listing all available instructions. Each is assigned a number (0, 1, 2...). LAST is special, to indicate the last used number, for bounding. See below for effect of each instruction.
* Note: More instructions will be added, up to 255. Multibyte instructions (instruction that leads to another function table) are also being considered.

```c++
typedef enum {
  NOOP = 0,
  XOR,
  XOR2,
  XOR3,
  DIV,
  ADD,
  SUB,
  INSTPTR,
  JUMP,
  REGA_XOR,
  REGB_XOR,
  REGC_XOR,
  REGD_XOR,
  CYCLEADD,
  CYCLESUB,
  _LAST
} VM_Instruction;
```

* Entangled unsigned integers, used all over the algorithm. Essentially returns a value cast to the desired size, based on many factors in state, including keccak state, step_limit, input_size...

```c++
// Template function, to return the xored number cast to any desired type.
template<typename T>
inline T TN_GetEntangledType(const VM_State& state) {
  return (T)(state.step_counter ^ state.register_a ^ state.register_b ^ state.register_c ^ state.register_d ^ state.hs.w[state.step_counter % 25] ^ state.hs.b[state.step_counter % 200] ^ state.step_limit ^ state.input_size);
}

// Shortcut defines to hand over state and get an entangled number of the desired type.
#define ENTANGLED_UINT8 TN_GetEntangledType<uint8_t>(state)    // Unsigned Int 8
#define ENTANGLED_UINT32 TN_GetEntangledType<uint32_t>(state)  // Unsigned Int 32
#define ENTANGLED_UINT64 TN_GetEntangledType<uint64_t>(state)  // Unsigned Int 64
```

* Memory addressing by instructions is relative to the position of the instruction (Relative addressing instead of absolute addressing). This function and helper define form the shorthand for this.
* So, MEM(0) is the byte that was parsed to decide the instruction, MEM(1) would be the byte after, MEM(-1) the one before, etc.

```c++
// Given state and a relative position, get a reference to the byte located a given distance from the instruction_ptr
inline uint8_t& TN_AtRelPos(VM_State& state, int position) {
  long int pos = state.instruction_ptr;
  pos += position;
  if (pos >= state.memory_size) pos = pos % state.memory_size; // TODO: Sign mismatch :< Better wrapping!
  while (pos < 0) { pos += state.memory_size; }                // TODO: Better wrapping!

  if (pos < 0 || pos > state.memory_size) {
    print("AtRelPos fucked up! " << pos);
  }

  return state.memory[pos];
}

#define MEM(relpos) TN_AtRelPos(state, relpos) // Helper shorthand to automagically hand over state
```

* TN_ParseInstruction is where the chosen instruction is actually "executed". Switch case based on the ENUM names of the instructions.
* Most instructions just mess with "random" locations in memory, xoring, adding, substracting things together. Some alter state variables like "registers", instruction_ptr, step_limit, etc.

```c++
inline void TN_ParseInstruction(VM_State& state, VM_Instruction inst) {
  switch (inst) {
  case XOR:
    MEM(0) ^= MEM(MEM(MEM(-1))); // Xor the byte located by instruction_ptr with the byte offset found after a chain of jumps based on memory locations, starting with the one before.
    break;
  case XOR2:
    MEM(1) ^= MEM(2); // (memory[instruction_ptr + 1] ^= memory[instruction_ptr + 2])
    MEM(0) ^= MEM(1); // (memory[instruction_ptr + 0] ^= memory[instruction_ptr + 1])
    break;
  case XOR3:
    MEM(0) ^= MEM(ENTANGLED_UINT8); 
    break;
  case DIV:
    MEM(0) = MEM(0) ^ (MEM(1) / (MEM(ENTANGLED_UINT8) + 1));
    break;
  case ADD:
    MEM(1) += MEM(2);
    MEM(0) += MEM(1);
    break;
  case SUB:
    MEM(1) -= MEM(2);
    MEM(0) -= MEM(1);
    break;
  case INSTPTR:
    state.instruction_ptr = state.instruction_ptr * ENTANGLED_UINT64 % state.memory_size;  // Corrupt the instruction ptr
    break;
  case JUMP:
    state.instruction_ptr = ((state.instruction_ptr + ((ENTANGLED_UINT8 % 200) - 100)) % state.memory_size); // Jump the instruction ptr
    break;
  case REGA_XOR:
    MEM(0) ^= state.register_a;
    state.register_a ^= MEM(MEM(1)) ^ ENTANGLED_UINT64; // Registers!
    break;
  case REGB_XOR:
    MEM(0) ^= state.register_b;
    state.register_b ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
    break;
  case REGC_XOR:
    MEM(0) ^= state.register_c;
    state.register_c ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
    break;
  case REGD_XOR:
    MEM(0) ^= state.register_d;
    state.register_d ^= MEM(MEM(1)) ^ ENTANGLED_UINT64;
    break;
  case CYCLEADD:
    MODCYCLES(1 * ENTANGLED_UINT8);  // Increment step limit by ENTANGLED_UINT8. (ModCycles is another helper define, see below)
    break;
  case CYCLESUB:
    MODCYCLES(-1 * ENTANGLED_UINT8); // Decrement step limit by ENTANGLED_UINT8.
    break;
  default:
    print("Illegal Instruction " << (int)inst); // Just in case nothing gets parsed, throw a warning. Should not occur.
  case NOOP: // Do nothing at all.
    break;
  }
}
```

* The MODCYCLES helper, to modify the step limit while not overstepping any limits.

```c++
inline void TN_AdjustCycleLimit(VM_State& state, int change) {
  state.step_limit += change; // Modify the step_limit by this much

  // Ensure step_limit remains above minimum and below maximum
  if (state.step_limit < state.step_limit_min) state.step_limit = state.step_limit_min;
  else if (state.step_limit > state.step_limit_max) state.step_limit = state.step_limit_max;
}

#define MODCYCLES(change) TN_AdjustCycleLimit(state, change) // Another helper define so one doesnt have to write out the whole thing each time
```

* The final function that decides on and uses the hashing function to return the final result.

```c++
inline void TN_FinalHash(const VM_State& state, const uint8_t* data, const size_t data_len, uint8_t* out) {
  switch (ENTANGLED_UINT8 % 3) { // Get an entangled number, bound by number of functions available
  default:
    print("Hash WTF???"); // Just in case, should not occur.
  case 0:
    jh_hash(HASH_SIZE * 8, data, 8 * data_len, (uint8_t*)out);
    break;
  case 1:
    blake256_hash((uint8_t*)out, data, data_len);
    break;
  case 2:
    groestl(data, (data_len * 8), (uint8_t*)out);
    break;
  }
}
```

* And that is all the relevant code. Hope it was understandable. ~F.Harms and M.Behm
