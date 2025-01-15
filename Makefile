CC = gcc
FINAL = wish

all: $(FINAL)

$(FINAL): wish.c
	$(CC) -o $(FINAL) wish.c
