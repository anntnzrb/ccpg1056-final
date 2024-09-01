CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -pthread
LDFLAGS = -pthread

SRC_DIR = src
TESTCASES_DIR = testcases
COMMON_OBJS = $(SRC_DIR)/bmp.o $(SRC_DIR)/common_filter.o

EXECUTABLES = publicador desenfocador realzador combinador

all: $(EXECUTABLES)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

publicador: $(SRC_DIR)/publicador.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

desenfocador: $(SRC_DIR)/desenfocador.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

realzador: $(SRC_DIR)/realzador.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

combinador: $(SRC_DIR)/combinador.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(EXECUTABLES)
	@mkdir -p outputs
	./combinador testcases/test0.bmp outputs/test0_combinador_out.bmp
	./combinador testcases/wizard.bmp outputs/wizard_combinador_out.bmp
	./combinador testcases/airplane.bmp outputs/airplane_combinador_out.bmp
	./combinador testcases/car.bmp outputs/car_combinador_out.bmp

docs:
	@printf "Building docs...\\n"
	typst compile --root docs docs/main.typ 'reporte-Threads.pdf'

fmt:
	@printf "Formatting src tree...\\n"
	@nix fmt

clean:
	rm -f $(SRC_DIR)/*.o $(TESTCASES_DIR)/*.o $(SRC_DIR)/*.d $(EXECUTABLES)
	rm -rf outputs

.PHONY: all clean test docs fmt