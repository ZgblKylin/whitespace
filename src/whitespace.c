#include "whitespace.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
#ifndef WHITESPACE_DYNAMIC_STACK
  #define STATIC_STACK

  #ifdef WHITESPACE_STATIC_STACK_SIZE
    #define DATA_STACK_SIZE (WHITESPACE_STATIC_STACK_SIZE)
  #else // no WHITESPACE_STATIC_STACK_SIZE
    #define DATA_STACK_SIZE 1024  // default size
  #endif // no WHITESPACE_STATIC_STACK_SIZE

  #define RANGE(x, min, max) (          \
    (x > min)                           \
      ? ( ((x > max) ? max : x) - min ) \
      : 0                               \
  )

  /* (1024, inf) part -> size/16 -> 184 + (0, inf)
   * (256, 1024] part -> size/8 -> 88 + (0, 96]
   * (64, 256)] part -> size/4 -> 40 + (0, 48]
   * (16, 64] part -> size/2 -> 16 + (0, 24)
   * (0, 16] part -> size -> (0, 16] */
  #define CALL_STACK_SIZE (                       \
    RANGE(DATA_STACK_SIZE, 1024, INT_MAX) / 16 +  \
    RANGE(DATA_STACK_SIZE,  256,    1024) /  8 +  \
    RANGE(DATA_STACK_SIZE,   64,     256) /  4 +  \
    RANGE(DATA_STACK_SIZE,   16,      64) /  2 +  \
    RANGE(DATA_STACK_SIZE,    0,      16)         \
  )

  #define LABEL_SIZE CALL_STACK_SIZE
  #define HEAP_SIZE DATA_STACK_SIZE
#endif // not WHITESPACE_DYNAMIC_STACK
// clang-format on

#define S WHITESPACE_S
#define T WHITESPACE_T
#define L WHITESPACE_L

#define UNUSED(x) (void)x

#define CHECK(x)                                                               \
  if (!x) {                                                                    \
    return false;                                                              \
  }

typedef struct Stack {
  int *stack;
  int depth;
  int size;
} Stack;

void StackRelease(struct Stack *stack) {
#ifndef STATIC_STACK
  if (stack->stack) {
    free(stack->stack);
  }
#endif // not STATIC_STACK
}

int StackPos(struct Stack *stack) { return stack->depth - 1; }

bool StackGrow(struct Stack *stack) {
#ifdef STATIC_STACK
  if (stack->depth >= stack->size) {
    return false;
  }
#else  // not STATIC_STACK
  if (!stack->stack) {
    stack->size = 1024;
    stack->stack = (int *)malloc(sizeof(int) * stack->size);
  } else if (stack->depth >= stack->size) {
    stack->size *= 2;
    stack->stack = (int *)realloc(stack->stack, sizeof(int) * stack->size);
  }
#endif // not STATIC_STACK
  stack->depth += 1;
  return true;
}

bool StackShrink(struct Stack *stack) {
  if (stack->depth) {
    stack->depth -= 1;
    return true;
  } else {
    return false;
  }
}

bool StackPush(struct Stack *stack, int number) {
  CHECK(StackGrow(stack));
  stack->stack[StackPos(stack)] = number;
  return true;
}

bool StackPopAt(struct Stack *stack, int *number, int pos) {
  if ((pos < 0) || (pos > StackPos(stack))) {
    return false;
  }
  *number = stack->stack[pos];
  for (int i = pos; i < stack->depth; i++) {
    stack->stack[i] = stack->stack[i + 1];
  }
  CHECK(StackShrink(stack));
  return true;
}

bool StackPop(struct Stack *stack, int *number) {
  return StackPopAt(stack, number, StackPos(stack));
}

bool StackPeekAt(struct Stack *stack, int *number, int pos) {
  if ((pos < 0) || (pos > StackPos(stack))) {
    return false;
  } else {
    *number = stack->stack[pos];
    return true;
  }
}

bool StackPeek(struct Stack *stack, int *number) {
  return StackPeekAt(stack, number, StackPos(stack));
}

typedef struct Label {
  const char *label;
  size_t label_length;
  size_t pc;
  struct Label *prev;
  struct Label *next;
} Label;

typedef struct Heap {
  int addr;
  int value;
  struct Heap *prev;
  struct Heap *next;
} Heap;

typedef struct Context {
  const char *code;
  size_t code_length;
  size_t pc;
  struct Stack data_stack;
  struct Stack call_stack;
  struct Label *labels;
  struct Heap *heaps;
#ifdef STATIC_STACK
  int _data_stack[DATA_STACK_SIZE];
  int _call_stack[CALL_STACK_SIZE];
  struct Label _labels[LABEL_SIZE];
  struct Heap _heaps[HEAP_SIZE];
#endif // STATIC_STACK
} Context;

void ContextInitialize(struct Context *ctx, const char *code, size_t length) {
  memset(ctx, 0, sizeof(struct Context));
  ctx->code = code;
  ctx->code_length = length;
#ifdef STATIC_STACK
  ctx->data_stack.stack = ctx->_data_stack;
  ctx->data_stack.size = DATA_STACK_SIZE;
  ctx->call_stack.stack = ctx->_call_stack;
  ctx->call_stack.size = CALL_STACK_SIZE;
#endif // not STATIC_STACK
}

struct Label *LabelCreate(struct Context *ctx, const char *label,
                          size_t label_length) {
  struct Label *ret;

#ifdef STATIC_STACK
  for (size_t i = 0; i < LABEL_SIZE; ++i) {
    // Find empty label
    struct Label *temp = &ctx->_labels[i];
    if (!temp->label) {
      ret = temp;
      break;
    }
  }
  if (!ret) {
    return NULL;
  }
#else  // not STATIC_STACK
  ret = (struct Label *)malloc(sizeof(struct Label));
  memset(ret, 0, sizeof(struct Label));
#endif // not STATIC_STACK

  ret->label = label;
  ret->label_length = label_length;
  ret->pc = ctx->pc;
  ret->next = ctx->labels;
  if (ret->next) {
    ret->next->prev = ret;
  }
  ctx->labels = ret;
  return ret;
}

struct Label *LabelRelease(struct Context *ctx, struct Label *label) {
  if (label->prev) {
    label->prev->next = label->next;
  } else {
    ctx->labels = label->next;
  }
  if (label->next) {
    label->next->prev = label->prev;
  }
  struct Label *ret = label->next;

#ifdef STATIC_STACK
  memset(label, 0, sizeof(struct Label));
#else  // not STATIC_STACK
  free(label);
#endif // not STATIC_STACK

  return ret;
}

struct Label *LabelFind(struct Context *ctx, const char *label,
                        size_t label_length) {
  struct Label *ret = ctx->labels;
  while (ret) {
    if (ret->label_length != label_length) {
      ret = ret->next;
      continue;
    }
    if (strncmp(ret->label, label, label_length) == 0) {
      return ret;
    }
  }
  return NULL;
}

struct Heap *HeapCreate(struct Context *ctx, int addr) {
  struct Heap *heap;

#ifdef STATIC_STACK
  for (size_t i = 0; i < HEAP_SIZE; ++i) {
    // Find empty heap
    struct Heap *temp = &ctx->_heaps[i];
    if (!temp->prev && !temp->next) {
      heap = temp;
      break;
    }
  }
  if (!heap) {
    return NULL;
  }
#else  // not STATIC_STACK
  heap = (struct Heap *)malloc(sizeof(struct Heap));
  memset(heap, 0, sizeof(struct Heap));
#endif // not STATIC_STACK

  heap->addr = addr;
  heap->next = ctx->heaps;
  if (heap->next) {
    heap->next->prev = heap;
  }
  ctx->heaps = heap;
  return heap;
}

struct Heap *HeapFind(struct Context *ctx, int addr) {
  struct Heap *heap = ctx->heaps;
  while (heap) {
    if (heap->addr == addr) {
      return heap;
    }
    heap = heap->next;
  }
  return NULL;
}

struct Heap *HeapRelease(struct Context *ctx, struct Heap *heap) {
  if (heap->prev) {
    heap->prev->next = heap->next;
  } else {
    ctx->heaps = heap->next;
  }
  if (heap->next) {
    heap->next->prev = heap->prev;
  }
  struct Heap *ret = heap->next;

#ifdef STATIC_STACK
  memset(heap, 0, sizeof(struct Heap));
#else  // not STATIC_STACK
  free(heap);
#endif // STATIC_STACK

  return ret;
}

void ContextRelease(struct Context *ctx) {
  StackRelease(&ctx->data_stack);
  StackRelease(&ctx->call_stack);
  while (ctx->labels) {
    ctx->labels = LabelRelease(ctx, ctx->labels);
  }
  while (ctx->heaps) {
    ctx->heaps = HeapRelease(ctx, ctx->heaps);
  }
}

char CodeAdvance(struct Context *ctx) {
  char ch;
  while (ctx->pc < ctx->code_length) {
    ch = ctx->code[ctx->pc++];
    switch (ch) {
    case S:
    case T:
    case L:
      return ch;
    default:
      continue;
    }
  }
  return '\0';
}

void CodePutBack(struct Context *ctx) { ctx->pc -= 1; }

typedef enum IMP {
  IMP_NONE,
  IMP_IO,
  IMP_STACK,
  IMP_ARITHMETIC,
  IMP_FLOW,
  IMP_HEAP
} IMP;
bool ParseIMP(struct Context *ctx, enum IMP *imp) {
  switch (CodeAdvance(ctx)) {
  case S:
    *imp = IMP_STACK;
    return true;

  case T:
    switch (CodeAdvance(ctx)) {
    case S:
      *imp = IMP_ARITHMETIC;
      return true;
    case T:
      *imp = IMP_HEAP;
      return true;
    case L:
      *imp = IMP_IO;
      return true;

    default:
      return false;
    }
    break;

  case L:
    *imp = IMP_FLOW;
    return true;

  case '\0':
    *imp = IMP_NONE;
    return true;

  default:
    return false;
  }
  return false;
}

bool ReadNumber(struct Context *ctx, int *number) {
  bool positive;
  switch (CodeAdvance(ctx)) {
  case S:
    positive = true;
    break;
  case T:
    positive = false;
    break;
  default:
    return false;
  }

  int num = 0;
  while (true) {
    switch (CodeAdvance(ctx)) {
    case S:
      num = num << 1;
      break;
    case T:
      num = num << 1;
      num = num | 0x1;
      break;
    case L:
      *number = positive ? num : -num;
      return true;
      break;
    default:
      return false;
    }
  }

  return false;
}

bool CommandIOReadAscii(struct Context *ctx) {
  int addr;
  CHECK(StackPop(&ctx->data_stack, &addr));
  char ch = CodeAdvance(ctx);
  struct Heap *heap = HeapFind(ctx, addr);
  CHECK(heap);
  heap->value = ch;
  return true;
}

bool CommandIOReadNumber(struct Context *ctx) {
  int addr;
  CHECK(StackPop(&ctx->data_stack, &addr));
  int number;
  CHECK(ReadNumber(ctx, &number));
  struct Heap *heap = HeapFind(ctx, addr);
  CHECK(heap);
  heap->value = number;
  return true;
}

bool CommandIOOutputAscii(struct Context *ctx) {
  int number;
  CHECK(StackPop(&ctx->data_stack, &number));
  char ch = (char)number;
  printf("%c", ch);
  return true;
}

bool CommandIOOutputNumber(struct Context *ctx) {
  int number;
  CHECK(StackPop(&ctx->data_stack, &number));
  printf("%d", number);
  return true;
}

bool CommandStackPush(struct Context *ctx) {
  int number;
  CHECK(ReadNumber(ctx, &number));
  CHECK(StackPush(&ctx->data_stack, number));
  return true;
}

bool CommandStackDuplicate(struct Context *ctx) {
  int number;
  CHECK(StackPeek(&ctx->data_stack, &number));
  CHECK(StackPush(&ctx->data_stack, number));
  return true;
}

bool CommandStackSwap(struct Context *ctx) {
  int a;
  CHECK(StackPop(&ctx->data_stack, &a));
  int b;
  CHECK(StackPop(&ctx->data_stack, &b));
  CHECK(StackPush(&ctx->data_stack, a));
  CHECK(StackPush(&ctx->data_stack, b));
  return true;
}

bool CommandStackDiscard(struct Context *ctx) {
  CHECK(StackShrink(&ctx->data_stack));
  return true;
}

bool CommandStackCopyNth(struct Context *ctx) {
  int offset;
  CHECK(ReadNumber(ctx, &offset));
  int number;
  CHECK(StackPeekAt(&ctx->data_stack, &number,
                    StackPos(&ctx->data_stack) - offset));
  CHECK(StackPush(&ctx->data_stack, number));
  return true;
}

bool CommandStackSlideN(struct Context *ctx) {
  int count;
  CHECK(ReadNumber(ctx, &count));
  for (int i = 0; i < count; ++i) {
    int number;
    CHECK(
        StackPopAt(&ctx->data_stack, &number, StackPos(&ctx->data_stack) - 1));
  }
  return true;
}

bool CommandArithmeticAddition(struct Context *ctx) {
  int b;
  CHECK(StackPop(&ctx->data_stack, &b));
  int a;
  CHECK(StackPop(&ctx->data_stack, &a));
  CHECK(StackPush(&ctx->data_stack, a + b));
  return true;
}

bool CommandArithmeticSubtraction(struct Context *ctx) {
  int b;
  CHECK(StackPop(&ctx->data_stack, &b));
  int a;
  CHECK(StackPop(&ctx->data_stack, &a));
  CHECK(StackPush(&ctx->data_stack, a - b));
  return true;
}

bool CommandArithmeticMultiplication(struct Context *ctx) {
  int b;
  CHECK(StackPop(&ctx->data_stack, &b));
  int a;
  CHECK(StackPop(&ctx->data_stack, &a));
  CHECK(StackPush(&ctx->data_stack, a * b));
  return true;
}

bool CommandArithmeticDivision(struct Context *ctx) {
  int b;
  CHECK(StackPop(&ctx->data_stack, &b));
  int a;
  CHECK(StackPop(&ctx->data_stack, &a));
  CHECK(StackPush(&ctx->data_stack, a / b));
  return true;
}

bool CommandArithmeticModulo(struct Context *ctx) {
  int b;
  CHECK(StackPop(&ctx->data_stack, &b));
  int a;
  CHECK(StackPop(&ctx->data_stack, &a));
  CHECK(StackPush(&ctx->data_stack, a % b));
  return true;
}

bool ReadLabel(struct Context *ctx, const char **label, size_t *label_length) {
  *label = ctx->code;
  *label_length = 0;
  while (true) {
    switch (CodeAdvance(ctx)) {
    case S:
    case T:
      *label_length += 1;
      break;
    case L:
      return true;
    default:
      return false;
    }
  }
  return false;
}

bool CommandFlowLabelCreate(struct Context *ctx) {
  const char *label;
  size_t label_length;
  CHECK(ReadLabel(ctx, &label, &label_length));
  return LabelCreate(ctx, label, label_length);
}

bool CommandFlowSubroutineCall(struct Context *ctx) {
  const char *label;
  size_t label_length;
  CHECK(ReadLabel(ctx, &label, &label_length));
  struct Label *ret = LabelFind(ctx, label, label_length);
  CHECK(ret);
  CHECK(StackPush(&ctx->call_stack, (int)ctx->pc));
  ctx->pc = ret->pc;
  return true;
}

bool CommandFlowLabelJumpImpl(struct Context *ctx, const char *label,
                              size_t label_length) {
  struct Label *ret = LabelFind(ctx, label, label_length);
  CHECK(ret);
  // No CHECK(StackPush(&ctx->call_stack, ctx->pc));
  ctx->pc = ret->pc;
  return true;
}

bool CommandFlowLabelJump(struct Context *ctx) {
  const char *label;
  size_t label_length;
  size_t pc = ctx->pc;
  CHECK(ReadLabel(ctx, &label, &label_length));
  return CommandFlowLabelJumpImpl(ctx, label, label_length);
}

bool CommandFlowLabelJumpIf0(struct Context *ctx) {
  int number;
  CHECK(StackPop(&ctx->data_stack, &number));
  const char *label;
  size_t label_length;
  size_t pc = ctx->pc;
  CHECK(ReadLabel(ctx, &label, &label_length));
  if (number == 0) {
    return CommandFlowLabelJumpImpl(ctx, label, label_length);
  } else {
    return true;
  }
}

bool CommandFlowLabelJumpIfNeg(struct Context *ctx) {
  int number;
  CHECK(StackPop(&ctx->data_stack, &number));
  const char *label;
  size_t label_length;
  size_t pc = ctx->pc;
  CHECK(ReadLabel(ctx, &label, &label_length));
  if (number < 0) {
    return CommandFlowLabelJumpImpl(ctx, label, label_length);
  } else {
    return true;
  }
}

bool CommandFlowSubroutineReturn(struct Context *ctx) {
  int pc;
  CHECK(StackPop(&ctx->call_stack, &pc));
  ctx->pc = pc;
  return true;
}

bool CommandFlowEndProgram(struct Context *ctx) {
  UNUSED(ctx);
  // exit(EXIT_SUCCESS);
  return true;
}

bool CommandHeapStore(struct Context *ctx) {
  int value;
  CHECK(StackPop(&ctx->data_stack, &value));
  int addr;
  CHECK(StackPop(&ctx->data_stack, &addr));
  struct Heap *heap = HeapFind(ctx, addr);
  CHECK(heap);
  heap->value = value;
  return true;
}

bool CommandHeapLoad(struct Context *ctx) {
  int addr;
  CHECK(StackPop(&ctx->data_stack, &addr));
  struct Heap *heap = HeapFind(ctx, addr);
  CHECK(heap);
  CHECK(StackPush(&ctx->data_stack, heap->value));
  return true;
}

bool IntepretImpl(struct Context *ctx) {
  while (true) {
    IMP imp;
    if (!ParseIMP(ctx, &imp)) {
      return true;
    }

    switch (imp) {
    case IMP_IO: {
      switch (CodeAdvance(ctx)) {
      case T: {
        switch (CodeAdvance(ctx)) {
        case S:
          CHECK(CommandIOReadAscii(ctx));
          break;
        case T:
          CHECK(CommandIOReadNumber(ctx));
          break;
        default:
          return false;
        }
      } break;

      case S: {
        switch (CodeAdvance(ctx)) {
        case S:
          CHECK(CommandIOOutputAscii(ctx));
          break;
        case T:
          CHECK(CommandIOOutputNumber(ctx));
          break;
        default:
          return false;
        }
      } break;

      default:
        return false;
      }
    } break;

    case IMP_STACK: {
      switch (CodeAdvance(ctx)) {
      case S: {
        CHECK(CommandStackPush(ctx));
      } break;

      case L: {
        switch (CodeAdvance(ctx)) {
        case S:
          CHECK(CommandStackDuplicate(ctx));
          break;
        case T:
          CHECK(CommandStackSwap(ctx));
          break;
        case L:
          CHECK(CommandStackDiscard(ctx));
          break;
        default:
          return false;
        }
      } break;

      case T: {
        switch (CodeAdvance(ctx)) {
        case S:
          CHECK(CommandStackCopyNth(ctx));
          break;
        case L:
          CHECK(CommandStackSlideN(ctx));
          break;
        default:
          return false;
        }
      } break;

      default:
        return false;
      }
    } break;

    case IMP_ARITHMETIC: {
      switch (CodeAdvance(ctx)) {
      case S: {
        switch (CodeAdvance(ctx)) {
        case S:
          CHECK(CommandArithmeticAddition(ctx));
          break;
        case T:
          CHECK(CommandArithmeticSubtraction(ctx));
          break;
        case L:
          CHECK(CommandArithmeticMultiplication(ctx));
          break;
        default:
          return false;
        }
      } break;

      case T: {
        switch (CodeAdvance(ctx)) {
        case S:
          CHECK(CommandArithmeticDivision(ctx));
          break;
        case T:
          CHECK(CommandArithmeticModulo(ctx));
          break;
        default:
          return false;
        }
      } break;

      default:
        return false;
      }
    } break;

    case IMP_FLOW: {
      switch (CodeAdvance(ctx)) {
      case S: {
        switch (CodeAdvance(ctx)) {
        case S:
          CHECK(CommandFlowLabelCreate(ctx));
          break;
        case T:
          CHECK(CommandFlowSubroutineCall(ctx));
          break;
        case L:
          CHECK(CommandFlowLabelJump(ctx));
          break;
        default:
          return false;
        }
      } break;

      case T: {
        switch (CodeAdvance(ctx)) {
        case S:
          CHECK(CommandFlowLabelJumpIf0(ctx));
          break;
        case T:
          CHECK(CommandFlowLabelJumpIfNeg(ctx));
          break;
        case L:
          CHECK(CommandFlowSubroutineReturn(ctx));
          break;
        default:
          return false;
        }
      } break;

      case L: {
        switch (CodeAdvance(ctx)) {
        case L:
          CHECK(CommandFlowEndProgram(ctx));
          return true;

        default:
          return false;
        }
      } break;

      default:
        return false;
      }
    } break;

    case IMP_HEAP: {
      switch (CodeAdvance(ctx)) {
      case S:
        CHECK(CommandHeapStore(ctx));
        break;
      case T:
        CHECK(CommandHeapLoad(ctx));
        break;
      default:
        return false;
      }
    } break;

    default:
      return false;
    }
  }
  return true;
}

bool WhiteSpace_Intepret(const char *code, size_t length) {
  Context ctx;
  ContextInitialize(&ctx, code, length);
  bool ret = IntepretImpl(&ctx);
  ContextRelease(&ctx);
  return ret;
}
