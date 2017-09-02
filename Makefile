CFLAGS=-g -Wall -Wnoerror
CC = gcc
RM = rm

C_SRC = buddy_allocator.c buddy_allocator_test.c

all: buddy_allocator

buddy_allocator: $(C_SRC) 
	$(CC) $(LDFLAGS) -o $@ $(C_SRC)

clean:
	$(RM) *.o buddy_allocator 
