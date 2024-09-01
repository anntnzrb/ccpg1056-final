CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -pthread
LDFLAGS = -pthread

# linux & darwin settings
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lrt
endif

SRC_DIR = src
TESTCASES_DIR = testcases
OBJS = $(SRC_DIR)/bmp.o $(SRC_DIR)/realzador.o $(SRC_DIR)/publicador.o $(SRC_DIR)/desenfocador.o $(SRC_DIR)/common_filter.o $(SRC_DIR)/combinador.o

all: clean combinador_test

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

combinador_test: $(SRC_DIR)/combinador.o $(OBJS)
	@mkdir -p outputs
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@./$@ testcases/test0.bmp outputs/test0_combinador_out.bmp
	@./$@ testcases/wizard.bmp outputs/wizard_combinador_out.bmp
	@./$@ testcases/airplane.bmp outputs/airplane_combinador_out.bmp
	@./$@ testcases/car.bmp outputs/car_combinador_out.bmp
docs:
	@printf "Building docs...\\n"
	typst compile --root docs docs/main.typ 'reporte-Threads.pdf'

fmt:
	@printf "Formatting src tree...\\n"
	@nix fmt

clean:
	rm -rf $(SRC_DIR)/*.o $(TESTCASES_DIR)/*.o $(SRC_DIR)/*.d combinador_test outputs

.PHONY: all clean combinador_test docs fmt