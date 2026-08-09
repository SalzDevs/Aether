#include "qeue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initSensorQeue(sensorQeue* sq) {
  sq->allocated_size = 100;
  sq->current_size = 0;
  sq->data = (sensor*)malloc(sq->allocated_size*sizeof(sensor)); 
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

void printQeue(sensorQeue* sq) {
  if (sq->current_size==0) {
    printf("Qeue is empty!");
    return;
  }
  printf("----------Start of Qeue Data----------\n");
  printf("size:%zu capacity:%zu\n",sq->current_size,sq->allocated_size); 
  for (size_t i = 0; i < sq->current_size; i++) {
    printf("Sensor id:%zu\n",i);
    printSensorData(sq->data[i]);
    if (i + 1 < sq->current_size) printf("\n");
  }
  printf("----------End of Qeue Data----------\n");
  printf("\n");
}
