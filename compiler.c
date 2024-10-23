/************************************************************/
/*  PL/0 compiler                                           */
/************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INPUT_MAX 1024
#define TOKENS_MAX 2048
#define WORDS_SYMBOLS 30
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_INSTRUCTIONS 500
#define LEV_MAX 4

//struct for symbols to be contained in symbol table
typedef struct
{
    int kind; // const = 1, var = 2, proc = 3
    char name[12]; // name up to 11 chars   (11 chars + 1 for null character)
    int val; // number (ASCII value)
    int level; // L lev
    int addr; // M address
    int mark; // to indicate unavailable or deleted
} symbol_t;

//struct for instructions to be stored in text arr & ELF
typedef struct
{
    int op;
    int L;
    int M;
} text_t;

//token processing functions
void addToTokenList(int token, char input[]);
int isReservedWordOrSymbol(char word[]);
int isSpecialSymbol(char sym);
int isMismatched(char input[]);
int isIdentifier(char identifier[]);
int processLexeme(char input[]);
int isNumber(char input[]);
int splitSymbol(char input[]);
void lexemeProcessWrapper(char input[]);
void commentHandling(FILE* file);
void printTokenList();
void printSourceCode(FILE *file);

//parser functions
void addToSymbolTable(int kind, char* name, int val, int level, int address);
int symbolTableCheck(char str[]);
int getNextToken();
char* getNextIdentifier();
void program();
void block();
void error(int id);
void emit(int op, int L, int M);
void constDeclaration();
int varDeclaration();
void procDeclaration();
void statement();
void condition();
void expression();
void term();
void factor();
void printSymbolTable();
void produceElfAndOut();

/************************************************************
*
*   SCANNER VARIABLES
*
************************************************************/

typedef enum
{
    oddsym = 1, identsym, numbersym, plussym, minussym,
    multsym, slashsym, fisym, eqsym, neqsym, lessym, leqsym,
    gtrsym, geqsym, lparentsym, rparentsym, commasym, semicolonsym,
    periodsym, becomessym, beginsym, endsym, ifsym, thensym,
    whilesym, dosym, callsym, constsym, varsym, procsym, writesym,
    readsym, elsesym
} token_type;

char* reservedWordsAndSymbols[] = {
    "+", "-", "*", "/", "fi", "=", "<>", "<", "<=", ">", ">=", "(", ")", ",", ";", ".", ":=", "begin", "end", "if",
    "then", "while", "do", "call", "const", "var", "procedure", "write", "read", "eeelse"
};

char specialSymbols[] = {'+', '-', '*', '/', '(', ')', '=', ',', '.', '<', '>', ';', ':'};

int tokenArray[TOKENS_MAX]; // store tokens
char identifierArray[TOKENS_MAX][12]; // store identifiers
int trackerIdentifier = 0; // track current identifier
int trackerToken = 0; // track current token
int trackerInput = 0; //track current input

/************************************************************
*
*   PARSER VARIABLES
*
************************************************************/

symbol_t symbol_table[MAX_SYMBOL_TABLE_SIZE]; // store symbols
text_t text[MAX_INSTRUCTIONS]; // store instructions
int tp = 0; // table index tracker
int token_p; // stores current token
int cx = 0; // tracker for next instruction
int lev = -1;

int main(int argc, const char* argv[]) {
    const char* fname = argv[1];
    FILE* file = fopen(fname, "r");
    char input[INPUT_MAX];

    ////////////////////////
    //begin scanning process
    ////////////////////////
    while (fscanf(file, "%c", &input[trackerInput]) == 1) {
        //detects if we are currently scanning in a comment
        if (trackerInput > 0 && input[trackerInput - 1] == '/' && input[trackerInput] == '*') {
            commentHandling(file);
            trackerInput = 0;
        }
        //tokenize before whitespace or skip over it
        else if (isspace(input[trackerInput])) {
            if (trackerInput != 0)
                lexemeProcessWrapper(input);
            trackerInput = 0;
        }
        //if we have mismatched characters or need to split symbols we tokenize first half
        else if (trackerInput > 0 && (isMismatched(input) || splitSymbol(input))) {
            char temp = input[trackerInput];
            lexemeProcessWrapper(input);

            //now input in mismatched character and update tracker
            input[0] = temp;
            trackerInput = 1;
        }
        //increment tracker if we dont tokenize since we're adding to input
        else
            trackerInput++;
    }
    //something left in input after end of loop
    if (trackerInput > 0) {
        lexemeProcessWrapper(input);
    }

    ////////////////////////
    //begin parsing process
    ////////////////////////
    trackerToken = 0;
    trackerIdentifier = 0;
    program();

    ////////////////////////
    //print source and output
    ////////////////////////
    printSourceCode(file);
    produceElfAndOut();

    return 0;
}

/************************************************************
*
*   SCANNER FUNCTIONS
*
************************************************************/

//wrapper function to process inputs into token list
void lexemeProcessWrapper(char input[]) {
    int token = processLexeme(input);
    if (token != -1)
        addToTokenList(token, input);
    else
        printf("error");
}

//adds given token into token list
void addToTokenList(int token, char input[]) {
    //token is invalid
    if (token == -1)
        return;
    tokenArray[trackerToken++] = token;
    if (token == 2) {
        strcpy(identifierArray[trackerIdentifier++], input);
    }
    if (token == 3) {
        tokenArray[trackerToken++] = atoi(input);
    }
}

//check if given input is a reserved word
int isReservedWordOrSymbol(char word[]) {
    if (strcmp(word, "odd") == 0)
        return 1;
    for (int i = 0; i < WORDS_SYMBOLS; i++)
        if (strcmp(word, reservedWordsAndSymbols[i]) == 0)
            return i + 4;

    //-1 indicates that given string is NOT a reserved word
    return -1;
}

//check if given input is an identifier
int isIdentifier(char identifier[]) {
    int i = 0;
    int len = 0;
    //check if first character of given input is a letter
    if (!isalpha(identifier[0]))
        return -1;
    //check the other 10 characters in input, if we find something other than a letter or number, return -1 to indicate invalid
    while (identifier[i] != '\0') {
        if (!isalpha(identifier[i]) && !isdigit(identifier[i]))
            return -1;

        len++;
        i++;
    }
    //a valid identifier can only be up to 11 characters long
    if (len > 11)
        return 0;

    //if code reaches here, given input is a valid identifier
    return identsym;
}

//check if given input is a special symbol
int isSpecialSymbol(char sym) {
    for (int i = 0; i < 13; i++)
        if (sym == specialSymbols[i])
            return 1;
    return 0;
}

//check if given input is a valid number
int isNumber(char input[]) {
    int i = 0;
    int len = 0;

    //check to see if every char inside input is a digit & keep track of # of digits
    while (input[i] != '\0') {
        if (!isdigit(input[i]))
            return -1;
        i++;
        len++;
    }
    //if number of digits exceeds 5, it is too long
    if (len > 5)
        return 0;

    return numbersym;
}

int processLexeme(char input[]) {
    //make char array a valid string for strcmp
    input[trackerInput] = '\0';

    //assume that -1 indicates invalid token
    int token = isReservedWordOrSymbol(input);

    if (token != -1)
        return token;

    token = isIdentifier(input);

    //error occured
    if (token == 0){
        error(17);  //ERROR: invalid identifier or identifier too long

    }
    if (token != -1)
        return token;

    token = isNumber(input);

    //error occured
    if (token == 0)
        error(18);  //ERROR: invalid number or number too long

    if (token != -1)
        return token;

    //if code reaches here, input failed all checks and is not valid symbol
    error(19);  //ERROR: invalid symbol
    return -1;
}

int isMismatched(char input[]) {
    //all none mismatched cases
    if ((isalpha(input[trackerInput]) && isalpha(input[trackerInput - 1]))
        || (isdigit(input[trackerInput]) && isdigit(input[trackerInput - 1]))
        || (isalpha(input[trackerInput]) && isdigit(input[trackerInput - 1]) && isalpha(input[0]))
        || (isdigit(input[trackerInput]) && isalpha(input[trackerInput - 1]))
        || (isSpecialSymbol(input[trackerInput]) && isSpecialSymbol(input[trackerInput - 1])))
        return 0;
    return 1;
}

//checks if symbols need to be split
int splitSymbol(char input[]) {
    //not dealing with two symbols so we exit
    if (!isSpecialSymbol(input[trackerInput]) || !isSpecialSymbol(input[trackerInput - 1]))
        return 0;

    //only dont split if were leading to two symbol token such := >= <= <>
    if ((input[trackerInput - 1] == '<' && input[trackerInput] == '>')
        || (input[trackerInput - 1] == ':' && input[trackerInput] == '=')
        || (input[trackerInput - 1] == '<' && input[trackerInput] == '=')
        || (input[trackerInput - 1] == '>' && input[trackerInput] == '='))
        return 0;
    return 1;
}

//handles improperly closed comments, enables program to continue tokenizing as normal
void commentHandling(FILE* file) {
    //tracks the input while in this function
    int tracker = 0;

    char commentInput[INPUT_MAX] = {0};

    while (fscanf(file, "%c", &commentInput[tracker]) == 1) {
        //if its closed return and pointer will point to right after comment closing
        if (tracker > 0 && commentInput[tracker - 1] == '*' && commentInput[tracker] == '/')
            //return 0 if comment PROPERLY closed
            return;
        tracker++;
    }
}

void printTokenList() {
    printf("\nToken List:\n");
    int token;
    int j = 0;

    for (int i = 0; i < trackerToken; i++) {
        token = tokenArray[i];
        printf("%d ", token);
        if (token == 2 && (i == 0 || tokenArray[i - 1] != 3))
            printf("%s ", identifierArray[j++]);
    }
    puts("");
}

/************************************************************
*
*   PARSER FUNCTIONS
*
************************************************************/

void emit(int op, int L, int M) {
    if (cx > MAX_INSTRUCTIONS)
        error(16); //ERROR: max number of instructions exceeded
    else {
        text[cx].op = op;
        text[cx].L = L;
        text[cx].M = M;
        cx++;
    }
}

void error(int id) {
    int idx;
    printf("Error: ");
    switch (id) {
        case 1:
            printf("program must end with period\n");
            break;
        case 2:
            printf("const, var, and read keywords must be followed by identifier\n");
            break;
        case 3:
            printf("symbol name has already been declared\n");
            break;
        case 4:
            printf("constants must be assigned with =\n");
            break;
        case 5:
            printf("constants must be assigned an integer value\n");
            break;
        case 6:
            printf("constant, procedure and variable declarations must be followed by a semicolon\n");
            break;
        case 7:
            //get index of most recent identifier
            idx = trackerIdentifier - 1;
            printf("undeclared identifier %s\n", identifierArray[idx]);
            break;
        case 8:
            printf("assignment to constant or procedure is not allowed\n");
            break;
        case 9:
            printf("assignment statements must use :=\n");
            break;
        case 10:
            printf("begin must be followed by end\n");
            break;
        case 11:
            printf("if must be followed by then\n");
            break;
        case 12:
            printf("while must be followed by do\n");
            break;
        case 13:
            printf("condition must contain comparison operator\n");
            break;
        case 14:
            printf("right parenthesis must follow left parenthesis\n");
            break;
        case 15:
            printf("arithmetic equations must contain operands, parentheses, numbers or symbols\n");
            break;
        case 16:
            printf("max number of instructions exceeded");
            break;
        case 17:
            printf("identifier too long");
            break;
        case 18:
            printf("number too long");
            break;
        case 19:
            printf("invalid symbol");
            break;
        case 20:
            printf("then must be followed by fi");
            break;
        case 21:
            printf("call must be followed by an identifier");
            break;
        case 22:
            printf("call of a constant or variable is meaningless");
            break;
        case 23:
            printf("maximum number of nested functions exceeded");
            break;
        case 24:
            printf("semicolon expected");
            break;
        case 25:
            printf("expression must not contain a procedure identifier");   //make input file for this
            break;
        case 26:
            printf("semicolon missing after procedure declaration");
            break;
        case 27:
            printf("incorrect symbol after procedure declaration");
            break;
    }
    exit(0);
}

int getNextToken() {
    return tokenArray[trackerToken++];
}

char* getNextIdentifier() {
    return identifierArray[trackerIdentifier++];
}

void addToSymbolTable(int kind, char* name, int val, int level, int address) {
    symbol_t temp;
    temp.kind = kind;
    strcpy(temp.name, name);
    temp.val = val;
    temp.level = level;
    temp.addr = address;
    temp.mark = 0;

    symbol_table[tp++] = temp;
}

int symbolTableCheck(char str[]) {
    for (int i = tp - 1; i >= 0; i--) {
        if (strcmp(symbol_table[i].name, str) == 0)
            return i;
    }
    return -1;
}

void program() {
    token_p = getNextToken();
    block();
    if (token_p != periodsym)
        error(1); //ERROR: program must end with period
    emit(9, 0, 3); //halt program
}

void block() {
    lev++;
    int prev_tp = tp;

    if(lev > LEV_MAX) {
        error(23);       //ERROR: maximum number of nested functions exceeded
    }

    int jmpAddr = cx;
    emit(7, 0, 0);

    constDeclaration();
    int numvars = varDeclaration();
    procDeclaration();

    text[jmpAddr].M = cx * 3 + 10;

    emit(6, 0, numvars + 3);

    statement();

    if(lev != 0)
        emit(2, 0, 0);

    tp = prev_tp;
    lev--;
}

void constDeclaration() {
    if (token_p == constsym) {
        do {
            token_p = getNextToken();

            if (token_p != identsym)
                error(2); //ERROR: const, var, read keywords must be followed by identifier

            char* name = getNextIdentifier(); //get name of next identifier

            if (symbolTableCheck(name) != -1)
                error(3); //ERROR: symbol name already declared

            token_p = getNextToken();

            if (token_p != eqsym)
                error(4); //ERROR: constants must be assigned with =

            token_p = getNextToken();

            if (token_p != numbersym)
                error(5); //ERROR: constant must be assigned an integer value

            //get number value of const
            //getNextToken() is used to get the num value, as num values are stored in the same array as the tokens for simplicity
            int val = getNextToken();

            addToSymbolTable(1, name, val, 0, 0);

            token_p = getNextToken();
        }
        while (token_p == commasym);

        if (token_p != semicolonsym)
            error(6);   //ERROR: constant and variable declarations must be followed by a semicolon

        token_p = getNextToken();
    }
}

int varDeclaration() {
    //store amount of vars declared
    int numvars = 0;

    if (token_p == varsym) {
        do {
            numvars++;

            token_p = getNextToken();

            //must declare a valid identifier
            if (token_p != identsym)
                error(2);   //ERROR: const, var, and read keywords must be followed by identifier

            char* name = getNextIdentifier();

            //identifier cannot be declared twice in same scope
            if (symbolTableCheck(name) != -1 && symbol_table[symbolTableCheck(name)].level == lev)
                error(3);   //ERROR: symbol name has already been declared

            addToSymbolTable(2, name, 0, lev, numvars + 2);

            token_p = getNextToken();
        }
        while (token_p == commasym);

        //must end with semicolon
        if (token_p != semicolonsym)
            error(6);   //ERROR: constant and variable declarations must be followed by a semicolon

        token_p = getNextToken();
    }
    return numvars;
}

void procDeclaration(){
    while(token_p == procsym) {
        token_p = getNextToken();

        if (token_p != identsym) {
            error(27);          //ERROR: incorrect symbol after procedure declaration
        }
        char *name = getNextIdentifier();

        if (symbolTableCheck(name) != -1)
            error(3);   //ERROR: symbol name has already been declared

        addToSymbolTable(3, name, 0, lev, cx * 3 + 10);

        token_p = getNextToken();

        if(token_p != semicolonsym) {
            error(26);          //ERROR: semicolon missing after procedure declaration
        }

        token_p = getNextToken();

        block();

        if(token_p != semicolonsym) {
            error(24);          //ERROR: semicolon or comma missing
        }

        token_p = getNextToken();
    }
}

void statement() {
    if (token_p == identsym) {
        //get name of the next identifier & check if it exists in sym table
        char* name = getNextIdentifier();
        int symIdx = symbolTableCheck(name);

        if (symIdx == -1)
            error(7); //ERROR: undeclared identifier

        //if token is not a var
        if (symbol_table[symIdx].kind != 2)
            error(8);   //ERROR: assignment to constant or procedure is not allowed

        token_p = getNextToken();

        if (token_p != becomessym)
            error(9);   //ERROR: assignment statements must use :=

        token_p = getNextToken();

        expression();
        emit(4, lev - symbol_table[symIdx].level, symbol_table[symIdx].addr); //emit STO
        return;
    }
    if(token_p == callsym){
        token_p = getNextToken();

        if(token_p != identsym)
            error(21); //ERROR: call must be followed by an identifier

        char* name = getNextIdentifier();
        int symIdx = symbolTableCheck(name);

        if(symIdx == -1)
            error(7); //ERROR: undeclared identifier
        else if(symbol_table[symIdx].kind == 3)  {
            emit(5, lev - symbol_table[symIdx].level, symbol_table[symIdx].addr); //emit CAL
        }
        else{
            error(22);    //ERROR: call of a constant or variable is meaningless
        }

        token_p = getNextToken();

        return;
    }
    if (token_p == beginsym) {
        do {
            token_p = getNextToken();
            statement();
        }while (token_p == semicolonsym);

        if (token_p != endsym)
            error(10); //ERROR: "begin" must be followed by "end"

        token_p = getNextToken();
        return;
    }
    if (token_p == ifsym) {

        token_p = getNextToken();

        condition();

        int jpcIdx = cx; //set jpcIdx to current code index

        emit(8, 0, 0); //emit JPC

        if (token_p != thensym)
            error(11); //ERROR: "if" must be followed by "then"

        token_p = getNextToken();

        statement();
        text[jpcIdx].M = cx * 3 + 10;

        if(token_p != fisym)
            error(20);  //ERROR: fi must follow then

        token_p = getNextToken();

        return;
    }
    if (token_p == whilesym) {
        token_p = getNextToken();

        int loopIdx = cx;

        condition();

        if (token_p != dosym)
            error(12);  //ERROR: "while" must be followed by "do"

        token_p = getNextToken();

        int jpcIdx = cx;
        emit(8, 0, 0);  //emit JPC

        statement();

        emit(7, 0, loopIdx * 3 + 10);    //emit JMP

        text[jpcIdx].M = cx * 3 + 10;
        return;
    }
    if (token_p == readsym) {
        token_p = getNextToken();

        if (token_p != identsym)
            error(2); //ERROR: const, var, and read keywords must be followed by identifier

        char* name = getNextIdentifier();

        int symIdx = symbolTableCheck(name);

        if (symIdx == -1)
            error(7); //ERROR: undeclared identifier

        if (symbol_table[symIdx].kind != 2)
            error(8); //ERROR: only variable values may be altered

        token_p = getNextToken();
        emit(9, 0, 2); //emit READ
        emit(4, lev - symbol_table[symIdx].level, symbol_table[symIdx].addr); //emit STO
        return;
    }
    if (token_p == writesym) {
        token_p = getNextToken();
        expression();
        emit(9, 0, 1); //emit WRITE
        return;
    }
}

void condition() {
    if (token_p == oddsym) {
        token_p = getNextToken();
        expression();
        emit(2, 0, 11); //emit ODD
    }
    else {
        expression();
        if (token_p == eqsym) {
            token_p = getNextToken();
            expression();
            emit(2, 0, 5); //emit EQL
        }
        else if (token_p == neqsym) {
            token_p = getNextToken();
            expression();
            emit(2, 0, 6); //emit NEQ
        }
        else if (token_p == lessym) {
            token_p = getNextToken();
            expression();
            emit(2, 0, 7); //emit LSS
        }
        else if (token_p == leqsym) {
            token_p = getNextToken();
            expression();
            emit(2, 0, 8); //emit LEQ
        }
        else if (token_p == gtrsym) {
            token_p = getNextToken();
            expression();
            emit(2, 0, 9); //emit GTR
        }
        else if (token_p == geqsym) {
            token_p = getNextToken();
            expression();
            emit(2, 0, 10); //emit GEQ
        }
        else
            error(13); //ERROR: condition must contain comparison operator
    }
}

void expression() {
    term();

    //stay in loop while we do addition or subtraction
    while (token_p == plussym || token_p == minussym) {
        if (token_p == plussym) {
            token_p = getNextToken();

            term();
            emit(2, 0, 1); //ADD
        }
        else {
            token_p = getNextToken();

            term();
            emit(2, 0, 2); //SUB
        }
    }
}

void term() {
    factor();
    //stay in loop while we do division or multiplication
    while (token_p == multsym || token_p == slashsym) {
        if (token_p == multsym) {
            token_p = getNextToken();

            factor();
            emit(2, 0, 3);  //emit MUL
        }
        else if (token_p == slashsym) {
            token_p = getNextToken();

            factor();
            emit(2, 0, 4);  //emit DIV
        }
    }
}

void factor() {

    if (token_p == identsym) {
        char* name = getNextIdentifier();
        int symIdx = symbolTableCheck(name);

        //identifier hasnt been declared
        if (symIdx == -1)
            error(7);   //ERROR: undeclared identifier

        //either constant or variable, NO procedures
        if (symbol_table[symIdx].kind == 1)                     //ERROR IF IT IS PROCEDURE
            emit(1, 0, symbol_table[symIdx].val);   //emit LIT
        else if(symbol_table[symIdx].kind == 3)
            error(25);  //ERROR: expression must not contain a procedure identifier
        else
            emit(3, lev - symbol_table[symIdx].level, symbol_table[symIdx].addr);  //emit LOD

        token_p = getNextToken();
    }
    else if (token_p == numbersym) {
        token_p = getNextToken();
        emit(1, 0, token_p);    //emit LIT
        token_p = getNextToken();
    }
    else if (token_p == lparentsym) {
        token_p = getNextToken();
        expression();
        if (token_p != rparentsym)
            error(14);  //ERROR: right parenthesis must follow left parenthesis
        token_p = getNextToken();
    }
    else
        error(15);  //ERROR: arithmetic equations must contain operands, parentheses, numbers or symbols
}

void printSymbolTable() {
    printf("\nKind | Name        | Value | Level | Address | Mark\n"
        "---------------------------------------------------\n");
    for (int i = 0; i < tp; i++) {
        printf("%4d |%12s |%6d |%6d |%8d |%4d\n", symbol_table[i].kind, symbol_table[i].name,
               symbol_table[i].val, symbol_table[i].level, symbol_table[i].addr, symbol_table[i].mark);
    }
}

//print to stdout and create elf file
void produceElfAndOut() {
    FILE* file = fopen("elf.txt", "w");
    for (int i = 0; i < cx; i++) {
        fprintf(file, "%d %d %d\n", text[i].op, text[i].L, text[i].M);
    }
    for (int i = 0; i < cx; i++) {
        int op = text[i].op;
        switch (op) {
        case 1:
            printf("%s", "LIT");
            break;
        case 2:
            switch (text[i].M) {
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
            case 11:
                printf("%s", "ODD");
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
        printf(" %d %d\n", text[i].L, text[i].M);
    }
}

void printSourceCode(FILE *file){
    fseek(file, 0, SEEK_SET);

    char code;
    printf("Source Program:\n\n");

    while(fscanf(file, "%c", &code) == 1)
        printf("%c", code);

    printf("\n\n");

    printf("No errors, program is syntactically correct\n\n");
}

