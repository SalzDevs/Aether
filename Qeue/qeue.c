#include "qeue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initSensorQeue(sensorQeue* sq) {
  sq->allocated_size = 100;
  sq->current_size = 0;
  sq->data = (sensorData*)malloc(sq->allocated_size*sizeof(sensorData)); 
}

bool isSensorQeueEmpty(sensorQeue* sq) {
  return sq->current_size == 0;
}


void addElemToQueue(sensorQeue *sq, sensorData d) {
  if (sq->current_size == sq->allocated_size) {
    size_t new_size = sq->allocated_size * 2;
    sensorData *temp = realloc(sq->data, new_size * sizeof(sensorData));
    if (temp == NULL) {
      fprintf(stderr,"Realocation of qeue failed");
      return; 
    }
    sq->data = temp;
    sq->allocated_size = new_size; 
  }

  sq->data[sq->current_size] = d;
  sq->current_size++;
}

sensorData removeElemFromQeue(sensorQeue *sq) {
  if(sq->current_size==0) {
    fprintf(stderr,"Cant remove itens from an empty Qeue");
    return (sensorData){0};
  }
  sensorData d = sq->data[0];
  memmove(sq->data, sq->data + 1, (sq->current_size - 1) * sizeof(sensorData));
  sq->current_size--;
  return d;
}

void printQeue(sensorQeue* sq) {
  if (isSensorQeueEmpty(sq)) {
    printf("Qeue is empty!");
    return;
  }
  printf("----------Start of Qeue Data----------\n");
  printf("size:%zu capacity:%zu\n",sq->current_size,sq->allocated_size); 
  for (size_t i = 0; i < sq->current_size; i++) {
    printSensorData(sq->data[i]);
    if (i + 1 < sq->current_size) printf("\n");
  }
  printf("----------End of Qeue Data----------\n");
  printf("\n");
}
