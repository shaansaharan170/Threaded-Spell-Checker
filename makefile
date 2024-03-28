CC=gcc
CFLAGS= -Wall -g -std=c11 -lpthread

# Target to build everything
all: A2checker

# Rule for building A2checker
A2checker: A2checker.c
	$(CC) $(CFLAGS) A2checker.c -o A2checker -lpthread

# Clean rule for removing compiled objects and executable
clean:
	rm -f A2checker *.o
