#include "whitespace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool DumpCode(const char *file, const char *code, size_t length) {
  FILE *fp = fopen(file, "w");
  size_t written = fwrite(code, sizeof(char), length, fp);
  fflush(fp);
  fclose(fp);
  return written == length;
}

bool Test_Print_1_100_1() {
#define S WHITESPACE_S
#define T WHITESPACE_T
#define L WHITESPACE_L
#define WS(x) WHITESPACE_##x

#define VALUE_0 S, S, L                       // S 0 L
#define VALUE_1 S, T, L                       // S 1 L
#define VALUE_LF S, S, S, S, S, T, S, T, S, L // S 00001010 L
#define VALUE_100 S, T, T, S, S, T, S, S, L   // S 1100100 L
#define VALUE_NEG_1 T, T, L                   // T 1 L

#define LABEL_S S, L
#define LABEL_T T, L

  // clang-format off
  const char code[] = {
    /* |----------------| */
    /* | int i = 1;     | */ WS(STACK_PUSH),          VALUE_1,    // i = 1
    /* | S: do {        | */ WS(FLOW_LABEL_CREATE),   LABEL_S,    // Create label 'S'
    /* |   print(i);    | */ WS(STACK_DUP),                       // j = i
    /* |                | */ WS(IO_OUTPUT_NUMBER),                // print j
    /* |   print('\n'); | */ WS(STACK_PUSH),          VALUE_LF,   // j = \n
    /* |                | */ WS(IO_OUTPUT_ASCII),                 // print j
    /* |   i++;         | */ WS(STACK_PUSH),          VALUE_1,    // j = 1
    /* |                | */ WS(ARITH_ADD),                       // i = i + j
    /* | } (i < 100);   | */ WS(STACK_DUP),                       // j = i
    /* |                | */ WS(STACK_PUSH),          VALUE_100,  // k = 100
    /* |                | */ WS(ARITH_SUB),                       // j = j - k
    /* |                | */ WS(FLOW_LABEL_JUMP_NEG), LABEL_S,    // goto 'S' if j < 0
    /* |----------------| */

    /* |----------------| */
    /* | int i = 100;   | */ // i already is 100
    /* | T: do {        | */ WS(FLOW_LABEL_CREATE),   LABEL_T,  // Create label 'T'
    /* |   print(i);    | */ WS(STACK_DUP),                     // j = i
    /* |                | */ WS(IO_OUTPUT_NUMBER),              // print j
    /* |   print('\n'); | */ WS(STACK_PUSH),          VALUE_LF, // j = \n
    /* |                | */ WS(IO_OUTPUT_ASCII),               // print j
    /* |   i--;         | */ WS(STACK_PUSH),          VALUE_1,  // j = 1
    /* |                | */ WS(ARITH_SUB),                     // i = i - j
    /* | } (i >= 1);    | */ WS(STACK_DUP),                     // j = i
    /* |                | */ WS(STACK_PUSH),          VALUE_0,  // k = 0
    /* |                | */ WS(STACK_SWAP),                    // swap(j, k)
    /* |                | */ WS(ARITH_SUB),                     // j = k - j
    /* |                | */ WS(FLOW_LABEL_JUMP_NEG), LABEL_T,  // goto 'T' if j < 0
    /* |----------------| */

    // end of program
    WS(FLOW_EXIT),

    '\0'
  };
  // clang-format on

  DumpCode("1_100_1.ws", code, sizeof(code) - 1);

  return WhiteSpace_Intepret(code, sizeof(code) - 1);
}

int main() { return Test_Print_1_100_1() ? EXIT_SUCCESS : EXIT_FAILURE; }
