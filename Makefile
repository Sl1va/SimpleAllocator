FILES = main.c
CC = gcc
FLAGS = -ggdb

all: build run clean

build:
	@$(CC) $(FILES) -o main $(FLAGS)
	@clear

run:
	@./main
	@echo
	@echo 'Program finished'
	@read -r _
	@clear

gdb:
	@gdb ./main
	@echo 'Program finished'
	@read -r _
	@clear

valgrind:
	@valgrind -s ./main
	@echo 'Program finished'
	@read -r _
	@clear

clean:
	@rm main