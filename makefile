CC = gcc
# CFLAGS = -std=c99 -Wall -Wextra -O2
CFLAGS = -std=c99 -O2
SRC = $(wildcard src/*.c)
OUT = nbody

ifeq ($(OS),Windows_NT)
	LDFLAGS = -lraylib -lopeng132 -lgdi32 -lwinmm -lpthread
	RUN = $(OUT).exe
else
	LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	RUN = ./$(OUT)
endif

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)

run: $(OUT)
	$(RUN)

clean:
	rm -rf $(OUT) $(OUT).exe