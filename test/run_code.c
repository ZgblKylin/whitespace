#include "whitespace.h"
#include <stdio.h>
#include <stdlib.h>

const char *LoadCode(const char *file) {
  FILE *fp = fopen(file, "r");
  if (!fp) {
    return NULL;
  }
  fseek(fp, 0, SEEK_END);
  size_t length = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char *code = (char *)malloc(length + 1);
  if (!code) {
    fclose(fp);
    return NULL;
  }
  fread(code, sizeof(char), length, fp);
  code[length] = '\0';
  fclose(fp);
  return code;
}

int main(int argc, char *argv[]) {
  const char *code = LoadCode((argc > 1) ? argv[1] : "1_100_1.ws");
  bool ret = WhiteSpace_Intepret(code);
  free(code);
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
