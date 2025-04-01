### 📁 `riscv-vm-in-c`

#### 📝 Description
A virtual machine written in C that interprets a RISC-V-inspired instruction set. Supports a fetch-decode-execute cycle, register emulation, memory-mapped I/O, and a custom heap allocator.

#### 💾 Features
- Instruction memory + data memory
- 32 general-purpose registers
- Virtual I/O operations (print/read int/char)
- Linked-list based heap allocation
- Supports R, I, S, SB, U, and UJ instruction types

#### 🚀 Tech Stack
- C (C99)
- Standard libraries: stdio, stdlib, math

#### 🧠 How to Run
```bash
gcc -o riscv_vm vm_riskxvii.c
./riscv_vm program.bin
```

#### 📂 Structure
```
vm_riskxvii.c    # Entire VM logic
program.bin      # Binary instruction file (input)
```

#### 📜 License
MIT

