#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initSensorQueue(sensorQueue* sq) {
  sq->allocated_size = 100;
  sq->current_size = 0;
  sq->data = (sensorData*)malloc(sq->allocated_size*sizeof(sensorData));
}

void destroySensorQueue(sensorQueue* sq) {
  if (sq == NULL) {
    return;
  }

  free(sq->data);
  sq->data = NULL;
  sq->allocated_size = 0;
  sq->current_size = 0;
}

bool isSensorQueueEmpty(sensorQueue* sq) {
  return sq->current_size == 0;
}


void addElemToQueue(sensorQueue *sq, sensorData d) {
  if (sq->current_size == sq->allocated_size) {
    size_t new_size = sq->allocated_size * 2;
    sensorData *temp = realloc(sq->data, new_size * sizeof(sensorData));
    if (temp == NULL) {
      fprintf(stderr,"Reallocation of queue failed");
      return;
    }
    sq->data = temp;
    sq->allocated_size = new_size;
  }

  sq->data[sq->current_size] = d;
  sq->current_size++;
}

sensorData removeElemFromQueue(sensorQueue *sq) {
  if(sq->current_size==0) {
    fprintf(stderr,"Cannot remove items from an empty queue");
    return (sensorData){0};
  }
  sensorData d = sq->data[0];
  memmove(sq->data, sq->data + 1, (sq->current_size - 1) * sizeof(sensorData));
  sq->current_size--;
  return d;
}

void printQueue(sensorQueue* sq) {
  if (isSensorQueueEmpty(sq)) {
    printf("Queue is empty!");
    return;
  }
  printf("----------Start of Queue Data----------\n");
  printf("size:%zu capacity:%zu\n",sq->current_size,sq->allocated_size);
  for (size_t i = 0; i < sq->current_size; i++) {
    printSensorData(sq->data[i]);
    if (i + 1 < sq->current_size) printf("\n");
  }
  printf("----------End of Queue Data----------\n");
  printf("\n");
}
