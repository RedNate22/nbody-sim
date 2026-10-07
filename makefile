CC = gcc
# CFLAGS = -std=c99 -Wall -Wextra -O2
CFLAGS = -std=c99 -O2
COMMON_SRC = src/body.c src/cli.c src/benchmark.c
GUI_SRC = src/main.c $(COMMON_SRC)
HEADLESS_SRC = src/main_headless.c $(COMMON_SRC)
OUT = nbody

ifeq ($(OS),Windows_NT)
	LDFLAGS = -lraylib -lopeng132 -lgdi32 -lwinmm -lpthread
	RUN = $(OUT).exe
else
	LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	RUN = ./$(OUT)
endif

.PHONY: all run clean nbody_headless

all: $(OUT)

$(OUT): $(GUI_SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(GUI_SRC) $(LDFLAGS)

nbody_headless: $(HEADLESS_SRC)
	$(CC) $(CFLAGS) -DNBODY_HEADLESS -o nbody_headless $(HEADLESS_SRC) -lm

run: $(OUT)
	$(RUN)

clean:
	rm -rf $(OUT) $(OUT).exe nbody_headless