#include "config.h"
#include <stddef.h>
#include <stdio.h>

configStatus readConfigFile(const char* fileName, char** buf_out) {
  FILE* fptr = fopen(fileName, "r");
  if (fptr == NULL) return CONFIG_ERROR;

  fseek(fptr, 0L, SEEK_END);
  long numbytes = ftell(fptr);
  rewind(fptr);
  if (numbytes < 0) { fclose(fptr); return CONFIG_ERROR; }

  char* buffer = (char*)calloc((size_t)numbytes + 1, 1);
  if (buffer == NULL) {
    fclose(fptr); return CONFIG_ERROR; 
  }

  size_t read = fread(buffer, 1, (size_t)numbytes, fptr);
  fclose(fptr);
  if (read != (size_t)numbytes) {
    free(buffer); return CONFIG_ERROR; 
  }

  *buf_out = buffer;
  return CONFIG_OK;
}
