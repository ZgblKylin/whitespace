#include "whitespace.h"
#include <stdio.h>
#include <stdlib.h>

bool DumpCode(const char *code, const char *file) {
  FILE *fp = fopen(file, "w");
  size_t length = strlen(code);
  size_t written = fwrite(code, sizeof(char), length, fp);
  fflush(fp);
  fclose(fp);
  return written == length;
}

bool Test_Print_1_100_1() {
#define S WHITESPACE_S
#define T WHITESPACE_T
#define L WHITESPACE_L

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
    /* | int i = 1;     | */ WHITESPACE_STACK_PUSH, VALUE_1,           // i = 1
    /* | S: do {        | */ WHITESPACE_FLOW_LABEL_CREATE, LABEL_S,    // Create label 'S'
    /* |   print(i);    | */ WHITESPACE_STACK_DUP,                     // j = i
    /* |                | */ WHITESPACE_IO_OUTPUT_NUMBER,              // print j
    /* |   print('\n'); | */ WHITESPACE_STACK_PUSH, VALUE_LF,          // j = \n
    /* |                | */ WHITESPACE_IO_OUTPUT_ASCII,               // print j
    /* |   i++;         | */ WHITESPACE_STACK_PUSH, VALUE_1,           // j = 1
    /* |                | */ WHITESPACE_ARTH_ADD,                      // i = i + j
    /* | } (i < 100);   | */ WHITESPACE_STACK_DUP,                     // j = i
    /* |                | */ WHITESPACE_STACK_PUSH, VALUE_100,         // k = 100
    /* |                | */ WHITESPACE_ARTH_SUB,                      // j = j - k
    /* |                | */ WHITESPACE_FLOW_LABEL_JUMP_NEG, LABEL_S,  // goto 'S' if j < 0
    /* |----------------| */

    /* |----------------| */
    /* | int i = 100;   | */ // i already is 100
    /* | T: do {        | */ WHITESPACE_FLOW_LABEL_CREATE, LABEL_T,    // Create label 'T'
    /* |   print(i);    | */ WHITESPACE_STACK_DUP,                     // j = i
    /* |                | */ WHITESPACE_IO_OUTPUT_NUMBER,              // print j
    /* |   print('\n'); | */ WHITESPACE_STACK_PUSH, VALUE_LF,          // j = \n
    /* |                | */ WHITESPACE_IO_OUTPUT_ASCII,               // print j
    /* |   i--;         | */ WHITESPACE_STACK_PUSH, VALUE_1,           // j = 1
    /* |                | */ WHITESPACE_ARTH_SUB,                      // i = i - j
    /* | } (i >= 1);    | */ WHITESPACE_STACK_DUP,                     // j = i
    /* |                | */ WHITESPACE_STACK_PUSH, VALUE_0,           // k = 0
    /* |                | */ WHITESPACE_STACK_SWAP,                    // swap(j, k)
    /* |                | */ WHITESPACE_ARTH_SUB,                      // j = k - j
    /* |                | */ WHITESPACE_FLOW_LABEL_JUMP_NEG, LABEL_T,  // goto 'T' if j < 0
    /* |----------------| */

    // end of program
    WHITESPACE_FLOW_EXIT,

    '\0'
  };
  // clang-format on

  DumpCode(code, "1_100_1.ws");

  return WhiteSpace_Intepret(code);
}

int main() { return Test_Print_1_100_1() ? EXIT_SUCCESS : EXIT_FAILURE; }
