#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define INST_MEM_SIZE 1024
#define DATA_MEM_SIZE 1024
#define HEAP_SIZE 64
#define NUM_BANKS 128
#define NUM_REGS 32

int debugga = 1;
int regs[NUM_REGS] = {0};
struct Node *head = NULL;
int pc = 0;
unsigned int allocCounter = 1;
unsigned int rawInstruction;

struct Node
{
    unsigned int startingAddress;
    int allocated;
    unsigned char heap[HEAP_SIZE];
    struct Node *next;
};

typedef struct
{
    unsigned char inst_mem[INST_MEM_SIZE];
    unsigned char data_mem[DATA_MEM_SIZE];
} MachineInstructions;

typedef struct
{
    unsigned int opcode;
    unsigned int rd;
    unsigned int rs1;
    unsigned int rs2;
    unsigned int func3;
    unsigned int func7;

    int immI;
    int immS;
    int immSB;
    int immU;
    int immUJ;
} Instruction;

typedef union
{
    int s;
    unsigned int u;
} Immediate;

void dumpRegisters()
{
    printf("PC = 0x%08x;\n", pc);
    for (int i = 0; i < NUM_REGS; i++)
    {
        printf("R[%d] = 0x%08x;\n", i, regs[i]);
    }
}

void notImplemented(unsigned int rawInstruction)
{
    printf("Instruction Not Implemented: 0x%08x\n", rawInstruction);
    dumpRegisters();
    exit(0);
}

void illegalOperation(unsigned int rawInstruction)
{
    printf("Illegal Operation: 0x%08x\n", rawInstruction);
    dumpRegisters();
    exit(0);
}

int virtualWriteCheck(unsigned int memAdress, unsigned int value)
{
    switch (memAdress)
    {
    case 2048: // Console Write Character
        printf("%c", value);
        return 1;
    case 2052: // Console Write Signed Integer
        printf("%d", value);
        return 1;
    case 2056: // Console Write Unsigned Integer
        printf("%x", value);
        return 1;
    case 2060: // HALT
        printf("CPU Halt Requested\n");
        exit(1);
    case 2080: // Dump PC
        printf("%x\n", pc);
        return 1;
    case 2084: // Dump Register Banks
        dumpRegisters();
        return 1;
    case 2088: // Dump Memory Word
    {
        struct Node *current = head;
        for (int i = 0; i < NUM_BANKS; i++)
        {
            if (current->next->startingAddress > value)
            {
                unsigned int memWord = current->heap[value - current->startingAddress];
                printf("%x\n", memWord);
                return 1;
            }
            current = current->next;
        }
        return 0;
    }
    case 2096: // malloc
    {
        struct Node *current = head;
        int banksRequired = (int)ceil(sizeof(value) / 64);
        for (int i = 0; i < NUM_BANKS; i++)
        {
            if (!current->allocated) // if the current is not allocated...
            {
                if (banksRequired == 1) // and the number of banks required is 1, we have found space
                {
                    current->allocated = allocCounter++;
                    regs[28] = current->startingAddress;
                    return 1;
                }
                struct Node *tempCurrent = current;
                for (int j = 1; j < banksRequired; j++) // otherwise check that there is space in adjacent banks
                {
                    if (tempCurrent->next != NULL) // if the next bank exists...
                    {
                        if (!tempCurrent->next->allocated) // and it is not allocated
                        {
                            if (banksRequired == j + 1) // and this is enough banks
                            {
                                regs[28] = current->startingAddress;
                                for (int k = 0; k < banksRequired; k++) // allocate banks equal to the number of banks required
                                {
                                    current->allocated = allocCounter++;
                                    current = current->next;
                                }
                                return 1;
                            }
                            tempCurrent = tempCurrent->next; // set tempCurrent to its own next node if we havn't enough banks yet
                        }
                        else
                        {
                            current = tempCurrent; // if the node isn't allocated, continue the outer loop from tempCurrent
                            break;
                        }
                    }
                    else
                    {
                        regs[28] = 0; // if there is no next node (we already checked if the banks required was 1) there is not enough space
                        return 0;     // failure to allocate  will need error handling
                    }
                }
            }
        }
        return 1;
    }
    case 2100: // free
    {
        struct Node *current = head;
        unsigned int uniqueAlocNum = 0;
        for (int i = 0; i < NUM_BANKS; i++)
        {
            if (current->next->startingAddress > value)
            {
                uniqueAlocNum = current->allocated;
                break;
            }
            current = current->next;
        }
        if (uniqueAlocNum)
        {
            for (int j = 0; j < NUM_BANKS; j++)
            {
                if (current->allocated == uniqueAlocNum)
                {
                    current->allocated = 0;
                    current = current->next;
                }
                else
                {
                    break;
                }
            }
        }
        break;
    }
    default:
        return 0;
    }
    return 0;
}
unsigned int virtualReadCheck(unsigned int memAdress)
{
    switch (memAdress)
    {
    case 2066: // Console Read Character
    {
        char c;
        scanf(" %c", &c);  // Added space before %c to avoid reading a newline
        unsigned int castc = (unsigned int)c;
        return castc;
    }
    
    case 2070: // Console Read Signed Integer
    {
        int d;
        scanf("%d", &d);
        unsigned int castd = (unsigned int)d;
        return castd;
    }

    default:
        return 0;
    }
}

void execute(Instruction instr, unsigned int rawInstruction)
{
    switch (instr.opcode)
    {
    case 0b0110011: // Type: R (add, sub, xor, or, and, sll, srl, sra, slt, sltu)
        switch (instr.func3)
        {
        case 0b000: // func3 000
            switch (instr.func7)
            {
            case 0b0000000: // add
                if (debugga)
                {
                    printf("add, pc = %d\n", pc);
                }
                if (instr.rd == 0)
                {
                    pc += 4;
                    return;
                }
                regs[instr.rd] = regs[instr.rs1] + regs[instr.rs2];
                pc += 4;
                return;
            case 0b0100000: // sub
                if (debugga)
                {
                    printf("sub, pc = %d\n", pc);
                }
                if (instr.rd == 0)
                {
                    pc += 4;
                    return;
                }
                regs[instr.rd] = regs[instr.rs1] - regs[instr.rs2];
                pc += 4;
                return;
            default:
                notImplemented(rawInstruction);
                exit(1);
            }
        case 0b100: // xor func3 100
            if (debugga)
            {
                printf("xor, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            regs[instr.rd] = regs[instr.rs1] ^ regs[instr.rs2];
            pc += 4;
            return;
        case 0b110: // or func3 110
            if (debugga)
            {
                printf("or, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            regs[instr.rd] = regs[instr.rs1] | regs[instr.rs2];
            pc += 4;
            return;
        case 0b111: // and func3 111
            if (debugga)
            {
                printf("and, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            regs[instr.rd] = regs[instr.rs1] & regs[instr.rs2];
            pc += 4;
            return;
        case 0b001: // sll func3 001
            if (debugga)
            {
                printf("sll, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            regs[instr.rd] = regs[instr.rs1] << regs[instr.rs2];
            pc += 4;
            return;
        case 0b101: // func3 101
            switch (instr.func7)
            {
            case 0b0000000: // srl
                if (debugga)
                {
                    printf("srl, pc = %d\n", pc);
                }
                if (instr.rd == 0)
                {
                    pc += 4;
                    return;
                }
                regs[instr.rd] = regs[instr.rs1] >> regs[instr.rs2];
                pc += 4;
                return;
            case 0b0100000: // sra
                if (debugga)
                {
                    printf("sra, pc = %d\n", pc);
                }
                if (instr.rd == 0)
                {
                    pc += 4;
                    return;
                }
                regs[instr.rd] = regs[instr.rs1];
                for (int i = 0; i < regs[instr.rs2]; i++)
                {
                    unsigned int lastBit = regs[instr.rd] & 0b1;      // extract the rightmost bit
                    unsigned int shiftedLastBit = lastBit << 31;      // move it to the front (31 bits to the left)
                    regs[instr.rd] = regs[instr.rd] >> 1;             // bitshift the target register
                    regs[instr.rd] = regs[instr.rd] | shiftedLastBit; // place the shifted last bit in the front
                }
                pc += 4;
                return;
            default:
                notImplemented(rawInstruction);
                exit(1);
            }
        case 0b010: // slt func3 010
        {
            if (debugga)
            {
                printf("slt, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            int rs1 = (int)regs[instr.rs1]; // to treat as signed
            int rs2 = (int)regs[instr.rs2];
            regs[instr.rd] = (rs1 < rs2) ? 1 : 0;
            pc += 4;
            return;
        }
        case 0b011: // sltu func3 011
        {
            if (debugga)
            {
                printf("sltu, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            regs[instr.rd] = (regs[instr.rs1] < regs[instr.rs2]) ? 1 : 0; // registers store unsigned data, so this should already be treated as such
            pc += 4;
            return;
        }
        default:
            notImplemented(rawInstruction);
            exit(1);
        }
    case 0b0010011: // Type: I (addi, xori, ori, andi, slti, sltiu)
        switch (instr.func3)
        {
        case 0b000: // addi
            if (debugga)
            {
                printf("addi, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            regs[instr.rd] = regs[instr.rs1] + instr.immI;
            pc += 4;
            return;
        case 0b100: // xori
            if (debugga)
            {
                printf("xori, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            regs[instr.rd] = regs[instr.rs1] ^ instr.immI;
            pc += 4;
            return;
        case 0b110: // ori
            if (debugga)
            {
                printf("ori, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            regs[instr.rd] = regs[instr.rs1] | instr.immI;
            pc += 4;
            return;
        case 0b111: // andi
            if (debugga)
            {
                printf("andi, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            regs[instr.rd] = regs[instr.rs1] & instr.immI;
            pc += 4;
            return;
        case 0b010: // slti
            if (debugga)
            {
                printf("slti, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            regs[instr.rd] = (regs[instr.rs1] < instr.immI) ? 1 : 0;
            pc += 4;
            return;
        case 0b011: // sltiu
            if (debugga)
            {
                printf("sltiu, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            unsigned int unsignedImmI = (unsigned int)instr.immI; // cast to treat the imm as unsigned
            regs[instr.rd] = (regs[instr.rs1] < unsignedImmI) ? 1 : 0;
            pc += 4;
            return;
        default:
            notImplemented(rawInstruction);
            exit(1);
        }
    case 0b0000011: // Type: I (lb, lh, lw, lbu, lhu)
        switch (instr.func3)
        {
        case 0b000: // lb
        {
            if (debugga)
            {
                printf("lb, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            if ((regs[instr.rs1] + instr.immI) == 2066 || (regs[instr.rs1] + instr.immI) == 2070)
            {
                regs[instr.rd] = virtualReadCheck(regs[instr.rs1] + instr.immI);
                pc += 4;
                return;
            }
            if ((regs[instr.rs1] + instr.immI) < 46848 || (regs[instr.rs1] + instr.immI) > 55040)
            {
                illegalOperation(rawInstruction);
            }
            struct Node *current = head;
            for (int i = 0; i < NUM_BANKS; i++)
            {
                if (current->next->startingAddress > (regs[instr.rs1] + instr.immI))
                {
                    regs[instr.rd] = (int)current->heap[(regs[instr.rs1] + instr.immI) - current->startingAddress]; // cast to int to ensure C sign extends the byte
                }
                current = current->next;
            }
            pc += 4;
            return;
        }
        case 0b001: // lh
        {
            if (debugga)
            {
                printf("lh, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            if ((regs[instr.rs1] + instr.immI) == 2066 || (regs[instr.rs1] + instr.immI) == 2070)
            {
                regs[instr.rd] = virtualReadCheck(regs[instr.rs1] + instr.immI);
                pc += 4;
                return;
            }
            if ((regs[instr.rs1] + instr.immI) < 46848 || (regs[instr.rs1] + instr.immI) > 55040)
            {
                illegalOperation(rawInstruction);
            }
            struct Node *current = head;
            for (int i = 0; i < NUM_BANKS; i++)
            {
                if (current->next->startingAddress > (regs[instr.rs1] + instr.immI))
                {
                    unsigned int firstHalf = current->heap[(regs[instr.rs1] + instr.immI) - current->startingAddress];
                    unsigned int secondHalf = current->heap[((regs[instr.rs1] + instr.immI) - current->startingAddress) + 1];
                    unsigned int halfWord = firstHalf | secondHalf;
                    int castedHalfWord = (int)halfWord; // cast to int to ensure C sign extends the byte
                    regs[instr.rd] = castedHalfWord;
                }
                current = current->next;
            }
            pc += 4;
            return;
        }
        case 0b010: // lw
        {
            if (debugga)
            {
                printf("lw, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            if ((regs[instr.rs1] + instr.immI) == 2066 || (regs[instr.rs1] + instr.immI) == 2070)
            {
                regs[instr.rd] = virtualReadCheck(regs[instr.rs1] + instr.immI);
                pc += 4;
                return;
            }
            if ((regs[instr.rs1] + instr.immI) < 46848 || (regs[instr.rs1] + instr.immI) > 55040)
            {
                illegalOperation(rawInstruction);
            }
            struct Node *current = head;
            for (int i = 0; i < NUM_BANKS; i++)
            {
                if (current->next->startingAddress > (regs[instr.rs1] + instr.immI))
                {
                    unsigned int firstQ = current->heap[(regs[instr.rs1] + instr.immI) - current->startingAddress];
                    unsigned int secondQ = current->heap[((regs[instr.rs1] + instr.immI) - current->startingAddress) + 1];
                    unsigned int thirdQ = current->heap[((regs[instr.rs1] + instr.immI) - current->startingAddress) + 2];
                    unsigned int fourthQ = current->heap[((regs[instr.rs1] + instr.immI) - current->startingAddress) + 3];
                    regs[instr.rd] = firstQ | secondQ | thirdQ | fourthQ;
                }
                current = current->next;
            }
            pc += 4;
            return;
        }
        case 0b100: // lbu
        {
            if (debugga)
            {
                printf("lbu, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            if ((regs[instr.rs1] + instr.immI) == 2066 || (regs[instr.rs1] + instr.immI) == 2070)
            {
                regs[instr.rd] = virtualReadCheck(regs[instr.rs1] + instr.immI);
                pc += 4;
                return;
            }
            if ((regs[instr.rs1] + instr.immI) < 46848 || (regs[instr.rs1] + instr.immI) > 55040)
            {
                illegalOperation(rawInstruction);
            }
            struct Node *current = head;
            for (int i = 0; i < NUM_BANKS; i++)
            {
                if (current->next->startingAddress > (regs[instr.rs1] + instr.immI))
                {
                    regs[instr.rd] = current->heap[(regs[instr.rs1] + instr.immI) - current->startingAddress];
                }
                current = current->next;
            }
            pc += 4;
            return;
        }
        case 0b101: // lhu
        {
            if (debugga)
            {
                printf("lhu, pc = %d\n", pc);
            }
            if (instr.rd == 0)
            {
                pc += 4;
                return;
            }
            if ((regs[instr.rs1] + instr.immI) == 2066 || (regs[instr.rs1] + instr.immI) == 2070)
            {
                regs[instr.rd] = virtualReadCheck(regs[instr.rs1] + instr.immI);
                pc += 4;
                return;
            }
            if ((regs[instr.rs1] + instr.immI) < 46848 || (regs[instr.rs1] + instr.immI) > 55040)
            {
                illegalOperation(rawInstruction);
            }
            struct Node *current = head;
            for (int i = 0; i < NUM_BANKS; i++)
            {
                if (current->next->startingAddress > (regs[instr.rs1] + instr.immI))
                {
                    unsigned int firstHalf = current->heap[(regs[instr.rs1] + instr.immI) - current->startingAddress];
                    unsigned int secondHalf = current->heap[((regs[instr.rs1] + instr.immI) - current->startingAddress) + 1];
                    unsigned int shiftedFirstHalf = firstHalf << 8;
                    regs[instr.rd] = shiftedFirstHalf | secondHalf;
                }
                current = current->next;
            }
            pc += 4;
            return;
        }
        default:
            notImplemented(rawInstruction);
            exit(1);
        }
    case 0b1100111: // Type I (jalr)
        if (debugga)
        {
            printf("jalr, pc = %d\n", pc);
        }
        if ((regs[instr.rs1] + instr.immI) < 0 || (regs[instr.rs1] + instr.immI) > 1020)
        {
            illegalOperation(rawInstruction);
        }
        if (instr.rd != 0)
        {
            regs[instr.rd] = pc + 4;
        }
        pc = regs[instr.rs1] + instr.immI;
        return;
    case 0b0100011: // Type: S (sb, sh, sw)
        switch (instr.func3)
        {
        case 0b000: // sb
        {
            if (debugga)
            {
                printf("sb, pc = %d\n", pc);
            }
            struct Node *current = head;
            for (int i = 0; i < NUM_BANKS; i++)
            {
                if (current->next->startingAddress > (regs[instr.rs1] + instr.immS))
                {
                    if (virtualWriteCheck((regs[instr.rs1] + instr.immS), regs[instr.rs2]))
                    {
                        pc += 4;
                        return;
                    }
                    current->heap[(regs[instr.rs1] + instr.immS) - current->startingAddress] = regs[instr.rs2];
                }
                current = current->next;
            }
            pc += 4;
            return;
        }
        case 0b001: // sh
        {
            if (debugga)
            {
                printf("sh, pc = %d\n", pc);
            }
            struct Node *current = head;
            for (int i = 0; i < NUM_BANKS; i++)
            {
                if (current->next->startingAddress > (regs[instr.rs1] + instr.immS))
                {
                    if (virtualWriteCheck((regs[instr.rs1] + instr.immS), regs[instr.rs2]))
                    {
                        pc += 4;
                        return;
                    }
                    unsigned int firstHalf = regs[instr.rs2] & 0b11111111;
                    unsigned int secondHalf = (regs[instr.rs2] >> 8) & 0b11111111;
                    current->heap[(regs[instr.rs1] + instr.immS) - current->startingAddress] = firstHalf;
                    current->heap[(regs[instr.rs1] + instr.immS) - current->startingAddress + 1] = secondHalf;
                }
                current = current->next;
            }
            pc += 4;
            return;
        }
        case 0b010: // sw
        {
            if (debugga)
            {
                printf("sw, pc = %d\n", pc);
            }
            struct Node *current = head;
            for (int i = 0; i < NUM_BANKS; i++)
            {
                if (current->next->startingAddress > (regs[instr.rs1] + instr.immS))
                {
                    if (virtualWriteCheck((regs[instr.rs1] + instr.immS), regs[instr.rs2]))
                    {
                        pc += 4;
                        return;
                    }
                    unsigned int firstQ = regs[instr.rs2] & 0b11111111;
                    unsigned int secondQ = (regs[instr.rs2] >> 8) & 0b11111111;
                    unsigned int thirdQ = (regs[instr.rs2] >> 16) & 0b11111111;
                    unsigned int fourthQ = (regs[instr.rs2] >> 24) & 0b11111111;
                    current->heap[(regs[instr.rs1] + instr.immS) - current->startingAddress] = firstQ;
                    current->heap[(regs[instr.rs1] + instr.immS) - current->startingAddress + 1] = secondQ;
                    current->heap[(regs[instr.rs1] + instr.immS) - current->startingAddress + 2] = thirdQ;
                    current->heap[(regs[instr.rs1] + instr.immS) - current->startingAddress + 3] = fourthQ;
                }
                current = current->next;
            }
            pc += 4;
            return;
        }
        default:
            notImplemented(rawInstruction);
            exit(1);
        }
    case 0b1100011: // Type: SB (beq, bne, blt, bltu, bge, bgeu)
        switch (instr.func3)
        {
        case 0b000: // beq
            if (debugga)
            {
                printf("beq, pc = %d\n", pc);
            }
            if (regs[instr.rs1] == regs[instr.rs2])
            {
                if ((pc + (instr.immSB << 1)) < 0 || (pc + (instr.immSB)) > 1020)
                {
                    illegalOperation(rawInstruction);
                }
                pc = pc + (instr.immSB);
                return;
            }
            pc += 4;
            return;
        case 0b001: // bne
            if (debugga)
            {
                printf("bne, pc = %d\n", pc);
            }
            if (regs[instr.rs1] != regs[instr.rs2])
            {
                if ((pc + (instr.immSB)) < 0 || (pc + (instr.immSB)) > 1020)
                {
                    illegalOperation(rawInstruction);
                }
                pc = pc + (instr.immSB);
                return;
            }
            pc += 4;
            return;
        case 0b100: // blt
        {
            if (debugga)
            {
                printf("blt, pc = %d\n", pc);
            }
            int signedRs1 = (int)regs[instr.rs1];
            int signedRs2 = (int)regs[instr.rs2];
            if (signedRs1 < signedRs2)
            {
                if ((pc + (instr.immSB)) < 0 || (pc + (instr.immSB)) > 1020)
                {
                    illegalOperation(rawInstruction);
                }
                pc = pc + (instr.immSB);
                return;
            }
            pc += 4;
            return;
        }
        case 0b110: // bltu
            if (debugga)
            {
                printf("bltu, pc = %d\n", pc);
            }
            if (regs[instr.rs1] < regs[instr.rs2])
            {
                if ((pc + (instr.immSB)) < 0 || (pc + (instr.immSB)) > 1020)
                {
                    illegalOperation(rawInstruction);
                }
                pc = pc + (instr.immSB);
                return;
            }
            pc += 4;
            return;
        case 0b101: // bge
        {
            if (debugga)
            {
                printf("bge, pc = %d\n", pc);
            }
            int signedRs1 = (int)regs[instr.rs1];
            int signedRs2 = (int)regs[instr.rs2];
            if (signedRs1 >= signedRs2)
            {
                if ((pc + (instr.immSB)) < 0 || (pc + (instr.immSB)) > 1020)
                {
                    illegalOperation(rawInstruction);
                }
                pc = pc + (instr.immSB);
                return;
            }
            pc += 4;
            return;
        }
        case 0b111: // bgeu
            if (debugga)
            {
                printf("bgeu, pc = %d\n", pc);
            }
            if (regs[instr.rs1] >= regs[instr.rs2])
            {
                if ((pc + (instr.immSB)) < 0 || (pc + (instr.immSB)) > 1020)
                {
                    illegalOperation(rawInstruction);
                }
                pc = pc + (instr.immSB);
                return;
            }
            pc += 4;
            return;
        default:
            notImplemented(rawInstruction);
            exit(1);
        }
    case 0b0110111: // Type: U (lui)
    {
        if (debugga)
        {
            printf("lui, pc = %d\n", pc);
        }
        if (instr.rd == 0)
        {
            pc += 4;
            return;
        }
        regs[instr.rd] = instr.immU;
        pc += 4;
        return;
    }
    case 0b1101111: // Type: UJ (jal)
        if (debugga)
        {
            printf("jal, pc = %d\n", pc);
        }
        if ((pc + (instr.immUJ)) < 0 || (pc + (instr.immUJ)) > 1020)
        {
            illegalOperation(rawInstruction);
        }
        if (instr.rd != 0)
        {
            regs[instr.rd] = pc + 4;
        }
        pc = pc + (instr.immUJ);
        return;
    default:
        notImplemented(rawInstruction);
        exit(1);
    }
    // pc += 4;
}

int main(int argc, char *argv[])
{
    struct Node *current = NULL;

    for (int i = 0; i < NUM_BANKS; i++) // create the Heap Bank linked list
    {
        struct Node *newNode = malloc(sizeof(struct Node));
        if (newNode == NULL)
        {
            printf("Error: unable to allocate memory.\n");
            return 1;
        }

        for (int j = 0; j < HEAP_SIZE; j++) // initialise the bank arrays to 0
        {
            newNode->heap[j] = 0;
        }

        newNode->next = NULL; // set the next pointer for the next node, the heap address, and unallocated.
        newNode->startingAddress = (46848 + (i * 64));
        newNode->allocated = 0;

        if (head == NULL) // add the node to the linked list
        {
            head = newNode;
            current = newNode;
        }
        else
        {
            current->next = newNode;
            current = newNode;
        }
    }

    if (argc < 2)
    {
        printf("Usage: %s <input file>\n", argv[0]);
        exit(1);
    }

    FILE *input = fopen(argv[1], "rb");
    if (input == NULL)
    {
        printf("Unable to open input file: %s\n", argv[1]);
        exit(1);
    }

    MachineInstructions inputData;

    size_t misRead = fread(&inputData, sizeof(MachineInstructions), 1, input);
    if (misRead != 1)
    {
        perror("Error reading from file");
        fclose(input);
        exit(1);
    }

    fclose(input);
    while (pc < 1024) // iterate over the instructions in steps of 4 bytes
    {
        Immediate unsignedImm;
        // int mostSig;
        Instruction instr;
        // extract raw instruction
        unsigned int rawInstruction = (inputData.inst_mem[pc] | (inputData.inst_mem[pc + 1] << 8) | (inputData.inst_mem[pc + 2] << 16) | (inputData.inst_mem[pc + 3] << 24));

        // decode opcode
        unsigned int opcodeByte = inputData.inst_mem[pc];

        instr.opcode = (opcodeByte & 0b1111111);

        // decode rd
        unsigned int rdByte1 = inputData.inst_mem[pc];

        unsigned int rdByte2 = inputData.inst_mem[pc + 1];
        unsigned int isolatedRdByte2 = (rdByte2 & 0b1111) << 1;

        instr.rd = isolatedRdByte2 | (rdByte1 >> 7);

        // decode func3
        unsigned int func3Byte = inputData.inst_mem[pc + 1];

        instr.func3 = (func3Byte & 0b1110000) >> 4;

        // decode rs1
        unsigned int rs1Byte1 = inputData.inst_mem[pc + 1];

        unsigned int rs1Byte2 = inputData.inst_mem[pc + 2];
        unsigned int isolatedRs1Byte2 = (rs1Byte2 & 0b1111) << 1;

        instr.rs1 = isolatedRs1Byte2 | (rs1Byte1 >> 7);

        // decode rs2
        unsigned int rs2Byte1 = inputData.inst_mem[pc + 2];
        unsigned int isolatedRs2Byte1 = rs2Byte1 >> 4;

        unsigned int rs2Byte2 = inputData.inst_mem[pc + 3];
        unsigned int isolatedRs2Byte2 = (rs2Byte2 & 0b1) << 4;

        instr.rs2 = isolatedRs2Byte1 | isolatedRs2Byte2;

        // decode func7
        unsigned int func7Byte = inputData.inst_mem[pc + 3];

        instr.func7 = func7Byte >> 1;

        // decode imm Type I
        unsigned int immIByte1 = inputData.inst_mem[pc + 2];
        unsigned int isolatedImmIByte1 = immIByte1 >> 4;

        unsigned int immIByte2 = inputData.inst_mem[pc + 3];
        unsigned int shiftedImmIByte2 = immIByte2 << 4;

        unsignedImm.u = isolatedImmIByte1 | shiftedImmIByte2;
        if (unsignedImm.u & 0x800)
        {
            unsignedImm.u |= 0xFFFFF000;
        }
        instr.immI = unsignedImm.s;

        // decode imm Type S
        unsigned int immSByte1 = inputData.inst_mem[pc];

        unsigned int immSByte2 = inputData.inst_mem[pc + 1];
        unsigned int isolatedImmSByte2 = (immSByte2 & 0b1111) << 1;
        unsigned int immSLower = isolatedImmSByte2 | (immSByte1 >> 7);

        unsigned int immSByte3 = inputData.inst_mem[pc + 3];
        unsigned int shiftedImmSByte3 = immSByte3 >> 1;
        unsigned int immSUpper = shiftedImmSByte3 << 5;

        unsignedImm.u = immSLower | immSUpper;
        if (unsignedImm.u & 0x800)
        {
            unsignedImm.u |= 0xFFFFF000;
        }
        instr.immS = unsignedImm.s;

        // decode imm Type SB
        unsigned int immSBByte1 = inputData.inst_mem[pc];
        unsigned int isolatedImmSBByte1 = immSBByte1 >> 7;
        unsigned int shiftedImmSBByte1 = isolatedImmSBByte1 << 11;

        unsigned int immSBByte2 = inputData.inst_mem[pc + 1];
        unsigned int isolatedImmSBByte2 = immSBByte2 & 0b1111;
        unsigned int shiftedImmSBByte2 = isolatedImmSBByte2 << 1;

        unsigned int immSBByte3 = inputData.inst_mem[pc + 3];
        unsigned int isolatedImmSBByte3 = immSBByte3 & 0b1111110;
        unsigned int shiftedImmSBByte3 = isolatedImmSBByte3 << 4;

        unsigned int lastImmSBBit = immSBByte3 >> 7;
        unsigned int shiftedImmSBBit = lastImmSBBit << 12;

        unsignedImm.u = shiftedImmSBByte1 | shiftedImmSBByte2 | shiftedImmSBByte3 | shiftedImmSBBit;
        if (unsignedImm.u & 0x1000)
        {
            unsignedImm.u |= 0xFFFFE000;
        }
        instr.immSB = unsignedImm.s; // good

        // decode imm Type U
        unsigned int immUByte1 = inputData.inst_mem[pc + 1];
        unsigned int isolatedImmUByte1 = immUByte1 >> 4;
        unsigned int shiftedImmUByte1 = isolatedImmUByte1 << 12;

        unsigned int immUByte2 = inputData.inst_mem[pc + 2];
        unsigned int shiftedImmUByte2 = immUByte2 << 16;

        unsigned int immUByte3 = inputData.inst_mem[pc + 3];
        unsigned int shiftedImmUByte3 = immUByte3 << 24;

        // unsignedImm.u = shiftedImmUByte1 | shiftedImmUByte2 | shiftedImmUByte3;
        // u = *(int32_t*)&unsignedImm.u;
        // instr.immU = u;
        unsignedImm.u = shiftedImmUByte1 | shiftedImmUByte2 | shiftedImmUByte3;
        instr.immU = unsignedImm.s;

        // decode imm Type UJ
        unsigned int immUJByte1 = inputData.inst_mem[pc + 1];
        unsigned int isolatedImmUJ15_12 = immUJByte1 >> 4;
        unsigned int shiftedImmUJ15_12 = isolatedImmUJ15_12 << 11;

        unsigned int immUJByte2 = inputData.inst_mem[pc + 2];
        unsigned int isolatedImmUJ19_16 = immUJByte2 & 0b1111;
        unsigned int shiftedImmUJ19_16 = isolatedImmUJ19_16 << 15;
        unsigned int isolatedImmUJ11 = immUJByte2 & 0b00010000;
        unsigned int shiftedImmUJ11 = isolatedImmUJ11 << 7;
        unsigned int isolatedImmUJ3_1 = immUJByte2 >> 5;
        unsigned int shiftedImmUJ3_1 = isolatedImmUJ3_1 << 1;

        unsigned int immUJByte3 = inputData.inst_mem[pc + 3];
        unsigned int isolatedImmUJ20 = immUJByte3 >> 7;
        unsigned int shiftedImmUJ20 = isolatedImmUJ20 << 20;
        unsigned int isolatedImmUJ10_4 = immUJByte3 & 0b1111111;
        unsigned int shiftedImmUJ10_4 = isolatedImmUJ10_4 << 4;

        unsignedImm.u = shiftedImmUJ20 | shiftedImmUJ19_16 | shiftedImmUJ15_12 | shiftedImmUJ11 | shiftedImmUJ10_4 | shiftedImmUJ3_1;
        if (unsignedImm.u & 0x100000)
        {
            unsignedImm.u |= 0xFFE00000;
        }
        instr.immUJ = unsignedImm.s;

        execute(instr, rawInstruction); // send it to execute
    }

    return 0;
}
