# System Emulator Lab

Authors: Joshua Yue and Abhinav Tiruveedhula

## Overview
This project is a system emulator that simulates the five-stage pipeline of an ARM processor. The emulator is designed to execute ARM assembly code in a cycle-accurate manner, replicating the core stages of an ARM CPU pipeline. The project allows users to simulate and observe the behavior of ARM system code on an emulated processor with full control over pipeline stages and execution flow.

The five stages of the CPU pipeline simulated are:
1. **IF (Instruction Fetch)** - Fetches the next instruction from memory.
2. **ID (Instruction Decode)** - Decodes the instruction and retrieves the necessary operands.
3. **EX (Execute)** - Executes the instruction, performing the necessary calculations or logic operations.
4. **M (Memory Access)** - Accesses memory if needed (for load/store instructions).
5. **WB (Write Back)** - Writes the result back to the register file.

## Features
- **Cycle-accurate simulation** of a five-stage ARM CPU pipeline.
- **Support for running ARM assembly code** directly, using a simplified ARM instruction set.
- **Detailed pipeline visualization** for tracking instruction flow through different stages.
- **Control over pipeline hazards** (data hazards, control hazards, structural hazards) and how they are handled (stalling, forwarding).
- **Step-by-step execution** to analyze the flow of individual instructions.
- **Debugging tools** to inspect registers, memory, and pipeline state.
- **Error handling** for unsupported instructions or out-of-bound memory accesses.
