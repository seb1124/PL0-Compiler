//Von Neumannn Stack Machine

#include <stdio.h>

#define ARRAY_SIZE 500

//CPU struct
typedef struct {
    int bp;
    int sp;
    int pc;
    int ir[3];
}CPU;

//functions
void printCpu(CPU cpu);
void printStack(CPU cpu);
void printStackRec(CPU cpu, int index, int nextDL);
int base( int BP, int L);
void printUtil(CPU cpu);

//stack
int pas[ARRAY_SIZE] = {0};

int main(int argc, const char * argv[]) {
    CPU cpu = {499, 500, 10};
    int run = 1;

    //read in from file into text part of stack
    const char* fname = argv[1];
    FILE *file = fopen( fname, "r" );
    int i = 10;
    while (fscanf(file, "%d", &pas[i]) == 1) {
        i++;
    }

    //print initial values
    printf("%18s%-5s%-5s%-5s%-5s\n","", "PC", "BP", "SP", "Stack");
    printf("Initial values: %4d%6d%5d\n\n", cpu.pc, cpu.bp, cpu.sp);

     while(run == 1) {
         //fetch
         cpu.ir[0] = pas[cpu.pc];
         cpu.ir[1] = pas[cpu.pc + 1];
         cpu.ir[2] = pas[cpu.pc + 2];
         cpu.pc += 3;
        //execute
         switch(cpu.ir[0]) {
             //LIT
             case 1:
                 //Literal push
                 cpu.sp -= 1;
                 pas[cpu.sp] = cpu.ir[2];
                 break;
             //RTN or OPR
             case 2:
                 //switch M to execute correct operation
                 switch(cpu.ir[2]){
                     //RTN
                     case 0:
                         //Returns from a subroutine and restore the caller's AR
                         cpu.sp = cpu.bp + 1;
                         cpu.bp = pas[cpu.sp - 2];
                         cpu.pc = pas[cpu.sp - 3];
                         break;
                     //ADD
                     case 1:
                         pas[cpu.sp + 1] = pas[cpu.sp + 1] + pas[cpu.sp];
                         cpu.sp += 1;
                         break;
                     //SUB
                     case 2:
                         pas[cpu.sp + 1] = pas[cpu.sp + 1] - pas[cpu.sp];
                         cpu.sp += 1;
                         break;
                     //MUL
                     case 3:
                         pas[cpu.sp + 1] = pas[cpu.sp + 1] * pas[cpu.sp];
                         cpu.sp += 1;
                         break;
                     //DIV
                     case 4:
                         pas[cpu.sp + 1] = pas[cpu.sp + 1] / pas[cpu.sp];
                         cpu.sp += 1;
                         break;
                     //EQL
                     case 5:
                         pas[cpu.sp + 1] = pas[cpu.sp + 1] == pas[cpu.sp];
                         cpu.sp += 1;
                         break;
                     //NEQ
                     case 6:
                         pas[cpu.sp + 1] = pas[cpu.sp + 1] != pas[cpu.sp];
                         cpu.sp += 1;
                         break;
                     //LSS
                     case 7:
                         pas[cpu.sp + 1] = pas[cpu.sp + 1] < pas[cpu.sp];
                         cpu.sp += 1;
                         break;
                     //LEQ
                     case 8:
                         pas[cpu.sp + 1] = pas[cpu.sp + 1] <= pas[cpu.sp];
                         cpu.sp += 1;
                         break;
                     //GTR
                     case 9:
                         pas[cpu.sp + 1] = pas[cpu.sp + 1] > pas[cpu.sp];
                         cpu.sp += 1;
                         break;
                     //GEQ
                     case 10:
                         pas[cpu.sp + 1] = pas[cpu.sp + 1] >= pas[cpu.sp];
                         cpu.sp += 1;
                         break;
                 }
                 break;
             //LOD
             case 3:
                 //Load val to top of stack from stack location @ offset o
                 //from n lexicographical levels down
                 cpu.sp -= 1;
                 pas[cpu.sp] = pas[base(cpu.bp, cpu.ir[1]) - cpu.ir[2]];
             break;
             //STO
             case 4:
                 //Store value at top of stack in stack location at offset o
                 //from n lexicographical levels down
                 pas[base(cpu.bp, cpu.ir[1]) - cpu.ir[2]] = pas[cpu.sp];
                 cpu.sp += 1;
                 break;
             //CAL
             case 5:
                 //Call procedure at code index p, generating new AR and
                 //setting PC to p
                 pas[cpu.sp - 1] = base(cpu.bp, cpu.ir[1]);
                 pas[cpu.sp - 2] = cpu.bp;
                 pas[cpu.sp - 3] = cpu.pc;
                 cpu.bp = cpu.sp - 1;
                 cpu.pc = cpu.ir[2];
                 break;
             //INC
             case 6:
                 //Allocate m locals on the stack
                 cpu.sp = cpu.sp - cpu.ir[2];
                 break;
             //JMP
             case 7:
                 //Jump to address a
                 cpu.pc = cpu.ir[2];
                 break;
             //JPC
             case 8:
                 //Jump conditionally: if value in pas[cpu.sp] is 0, then
                 //jump to a and pop the stack
                 if(pas[cpu.sp] == 0) {
                     cpu.pc = cpu.ir[2];
                 }
                 cpu.sp++;
                 break;
             //SYS
             case 9:
                 switch(cpu.ir[2]){
                     case 1:
                         //Output value in pas[cpu.sp] to std output & pop
                         printf("Output result is: %d\n", pas[cpu.sp]);
                         cpu.sp++;
                         break;
                     case 2:
                         //Read an integer from stdin and store it on top of stack
                         cpu.sp--;
                         printf("Please enter an integer: ");
                         scanf("%d", &pas[cpu.sp]);
                         break;
                     case 3:
                         //Halt the program
                         run = 0;
                         break;
                 }
                 break;
         }

         //Print CPU & stack
         printCpu(cpu);
         printStackRec(cpu, cpu.sp, cpu.bp);
         puts("");
     }

    return 0;
}

//prints cpu
void printCpu(CPU cpu) {
    printUtil(cpu);
    printf("%1s%-2d%-10d%-5d%-5d%-5d", "", cpu.ir[1], cpu.ir[2], cpu.pc, cpu.bp, cpu.sp);
}

//prints operation
void printUtil(CPU cpu) {
    printf("%2s", "");
    switch (cpu.ir[0]) {
        case 1:
            printf("%s", "LIT");
            break;
        case 2:
            switch(cpu.ir[2]){
                case 0:
                    printf("%s", "RTN");
                    break;
                case 1:
                    printf("%s", "ADD");
                    break;
                case 2:
                    printf("%s", "SUB");
                    break;
                case 3:
                    printf("%s", "MUL");
                    break;
                case 4:
                    printf("%s", "DIV");
                    break;
                case 5:
                    printf("%s", "EQL");
                    break;
                case 6:
                    printf("%s", "NEQ");
                    break;
                case 7:
                    printf("%s", "LSS");
                    break;
                case 8:
                    printf("%s", "LEQ");
                    break;
                case 9:
                    printf("%s", "GTR");
                    break;
                case 10:
                    printf("%s", "GEQ");
                    break;
            }
            break;
        case 3:
            printf("%s", "LOD");
            break;
        case 4:
            printf("%s", "STO");
            break;
        case 5:
            printf("%s", "CAL");
            break;
        case 6:
            printf("%s", "INC");
            break;
        case 7:
            printf("%s", "JMP");
            break;
        case 8:
            printf("%s", "JPC");
            break;
        case 9:
            printf("SYS");
            break;
    }
}

int base( int BP, int L) {
    int arb = BP; // arb = activation record base
    while ( L > 0) //find base L levels down
    {
        arb = pas[arb];
        L--;
    }
    return arb;
}

//prints stack
void printStack(CPU cpu) {
    int currentBP = cpu.bp;


     // Print elements from the top of the stack down to the stack pointer
     for (int i = ARRAY_SIZE - 1; i >= cpu.sp; i--) {

         //Iterate through Dynamic Links and print "|" for activation record if they correspond to current index
          while(currentBP < 499){
              if(currentBP == i) {
                  printf("| ");
              }
              currentBP = pas[currentBP - 1];
          }
         currentBP = cpu.bp;
         printf("%d ", pas[i]);
     }
 }

//recursive version of printStack function
void printStackRec(CPU cpu, int index, int nextDL)
{
    if(index > 499)
        return;
    if(index >= nextDL)
    {
        printStackRec(cpu, index + 1, pas[nextDL - 1]);
        if(index == nextDL && index != ARRAY_SIZE - 1)
            printf("| ");
    }
    else
        printStackRec(cpu, index + 1, nextDL);

    printf("%d ", pas[index]);
}
