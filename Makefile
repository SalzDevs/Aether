# Variables
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
OBJS = main.o sensor/sensor.o scheduler/scheduler.o Qeue/qeue.o 
TARGET = main

# Final Link Step (combines .o files into executable)
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Compile Steps (.c to .o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)
