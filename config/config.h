#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdlib.h>

typedef enum {
  CONFIG_OK,
  CONFIG_ERROR
} configStatus;

configStatus readConfigFile(const char *fileName, char** buf_out);
configStatus fileToSensors(char* fileName);

#endif
