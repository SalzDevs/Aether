#ifndef SENSOR_H
#define SENSOR_H

typedef struct {
  float airSpeed;
  float altitude;
  int engineTemperature;
  int fuelLevel;
  int baterryVoltage;
} sensor;

void initSensor(sensor* s);

void printSensorData(sensor s); 

void sensorDataUpdate(sensor* s);


#endif
