#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sensor/sensor.h"
#include "Qeue/qeue.h"

int main() {
  sensorQeue sq;
  initSensorQeue(&sq);
  sensor s;
  initSensor(&s);
  addElemToQueue(&sq, s);
  addElemToQueue(&sq,s);
  addElemToQueue(&sq, s);
  printQeue(&sq);
  removeElemFromQeue(&sq);
  printQeue(&sq);
  return 0;
}
