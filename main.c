#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//#include "scheduler/scheduler.h"
#include "sensor/sensor.h"

typedef struct {
  size_t allocated_size;
  size_t current_size;
  sensor* data;
} sensorQeue;


void initSensorQeue(sensorQeue* sq) {
  sq->allocated_size = 100;
  sq->current_size = 0;
  sq->data = (sensor*)malloc(sizeof(sensor)); 
}


void addElemToQueue(sensorQeue *sq, sensor s) {
  if ((sq->current_size + 1) == sq->allocated_size) {
    size_t new_size = sq->allocated_size * 2;
    sensor *temp = realloc(sq->data, new_size * sizeof(sensor));
    if (temp == NULL) {
      fprintf(stderr,"Realocation of qeue failed");
      return; 
    }
    sq->data = temp;
    sq->allocated_size = new_size; 
  }

  sq->data[sq->current_size] = s;
  sq->current_size++;
}

sensor removeElemFromQeue(sensorQeue *sq) {
  if(sq->current_size==0) {
    fprintf(stderr,"Cant remove itens from an empty Qeue");
    return (sensor){0};
  }
  sensor s = sq->data[0];
  memmove(sq->data, sq->data + 1, (sq->current_size - 1) * sizeof(sensor));
  sq->current_size--;
  return s;
}

int main() {
  sensorQeue sq;
  initSensorQeue(&sq);
  printf("sensorQeue data after init: {%zu} {%zu} {%p}\n",sq.allocated_size, sq.current_size, sq.data);
  sensor s;
  initSensor(&s);
  addElemToQueue(&sq, s);
  addElemToQueue(&sq,s);
  printSensorData(sq.data[0]);
  addElemToQueue(&sq, s);
  printSensorData(sq.data[1]);
  printSensorData(sq.data[2]);
  printf("Size of sq is:%zu\n",sq.current_size);
  printf("removed first element from qeue\n");
  removeElemFromQeue(&sq);
  printf("New size of sq is:%zu\n",sq.current_size);
  /*
  initSensor(&s);
  printSensorData(s);
  printf("\nAfter Updating Data on sensor\n");
  sensorDataUpdate(&s);
  printSensorData(s);
  
  Task t;
  initTask(&t, 30);
  printTask(t); 
  
  */ 
  return 0;
}
