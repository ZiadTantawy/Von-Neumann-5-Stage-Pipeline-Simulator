# 5-Stage Von Neumann CPU Simulator

## Overview
A cycle-accurate **5-stage Von Neumann CPU pipeline** implemented in **C**, built as part of a computer architecture project.  
The simulator demonstrates instruction-level parallelism, hazard handling, and pipeline debugging.

---

## Features
- **Pipeline Stages**: Instruction Fetch (IF), Decode (ID), Execute (EX), Memory (MEM), Write-Back (WB).
- **Hazard Handling**: Data forwarding and hazard detection for resolving RAW conflicts.
- **Control Flow**: Branching, jumps, and immediate instructions with sign-extension.
- **Assembler**: Custom assembler/parser to convert assembly into 32-bit binary machine code.
- **Memory & Registers**: Simulated memory subsystem, register file, safe reads/writes, memory dump.
- **Debugging**: Cycle-by-cycle pipeline trace logs for visualization and analysis.
