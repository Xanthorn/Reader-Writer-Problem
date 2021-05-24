# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g

# the build target executable:
DIR = bin
TARGET = main

all: $(TARGET)

$(TARGET): $(TARGET).o 
	$(CC) $(CFLAGS) -o ./$(DIR)/$(TARGET) ./$(DIR)/$(TARGET).o

$(TARGET).o : $(TARGET).c 
	$(CC) $(CFLAGS) -o ./$(DIR)/$(TARGET).o -c $(TARGET).c

clean:
	$(RM) $(TARGET)