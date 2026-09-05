#this File can run you have Make installed (eg:in terminal-->make --version)
#make shure you have installed MinGW for running this file(eg:in terminal-->gcc --version)
#all is instaled in your pc already. check it simply type -->make clean then--> make run
CC = gcc
CFLAGS = -Wall -Wextra -g -I./includes
TARGET = btrfs_simulator.exe
SRCS = $(wildcard src/*.c)

ifeq ($(OS),Windows_NT)
	CLEAN_CMD = if exist $(TARGET) del $(TARGET)

else
	CLEAN_CMD = rm -f $(TARGET)
endif

all: build

build:
	@echo "--------------------------------------------------------------"
	@echo "		Compiling BTRFS Simulator"
	@echo "--------------------------------------------------------------"
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
	@echo "			Build Successful!"
	@echo "--------------------------------------------------------------"

run: build
	@echo "		Starting BTRFS System"
	@echo "--------------------------------------------------------------"
	./$(TARGET)

clean:
	@echo "--------------------------------------------------------------"
	@echo " 		Cleaning old builds..."
	@$(CLEAN_CMD)
	@echo "--------------------------------------------------------------"
	@echo " 		Clean complete!"
	@echo "--------------------------------------------------------------"