CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -pthread
LDFLAGS = -pthread

# linux & darwin settings
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lrt
endif

SRC_DIR = src
TESTCASES_DIR = testcases
OBJS = $(SRC_DIR)/bmp.o $(SRC_DIR)/common_filter.o $(SRC_DIR)/util.o $(SRC_DIR)/publicador.o $(SRC_DIR)/desenfocador.o $(SRC_DIR)/realzador.o

all: clean combinador

debug: CFLAGS += -DDEBUG_REALZADOR -DDEBUG_DESENFOCADOR -DDEBUG_COMMON_FILTER
debug: all

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

combinador: $(SRC_DIR)/combinador.o $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

publicador: $(SRC_DIR)/publicador.o $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

desenfocador: $(SRC_DIR)/desenfocador.o $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

realzador: $(SRC_DIR)/realzador.o $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

docs:
	@printf "Building docs...\\n"
	typst compile --root docs docs/main.typ 'reporte-Threads.pdf'

fmt:
	@printf "Formatting src tree...\\n"
	@nix fmt

clean:
	rm -rf $(SRC_DIR)/*.o $(TESTCASES_DIR)/*.o $(SRC_DIR)/*.d combinador publicador desenfocador realzador outputs

.PHONY: all clean debug combinador_test docs fmt