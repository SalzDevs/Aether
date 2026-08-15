#include "../config/config.h"
#include <stdio.h>

static void test_read_config_file(void) {
  char* buf_out = NULL;
  if (readConfigFile("config/sensors.yaml", &buf_out) == CONFIG_OK) {
    printf("%s", buf_out);
    free(buf_out);
  }
}

int main() {
  test_read_config_file();
  return 0; 
}
