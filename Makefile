CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -pthread
LDFLAGS = -pthread

# linux & darwin settings
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lrt
endif

SRC_DIR = src
TESTCASES_DIR = testcases
OBJS = $(SRC_DIR)/bmp.o $(SRC_DIR)/realzador.o $(SRC_DIR)/publicador.o $(SRC_DIR)/desenfocador.o $(SRC_DIR)/common_filter.o

all: clean realzador_test desenfocador_test

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(SRC_DIR)/*.o $(TESTCASES_DIR)/*.o $(SRC_DIR)/*.d realzador_test desenfocador_test

define build_and_run_test
	@mkdir -p outputs
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@./$@ testcases/test.bmp outputs/test_$(1)_out.bmp
endef

realzador_test: $(TESTCASES_DIR)/realzador_test.o $(OBJS)
	$(call build_and_run_test,realzador)

desenfocador_test: $(TESTCASES_DIR)/desenfocador_test.o $(OBJS)
	$(call build_and_run_test,desenfocador)

docs:
	@printf "Building docs...\\n"
	typst compile --root docs docs/main.typ 'reporte-Threads.pdf'

fmt:
	@printf "Formatting src tree...\\n"
	@nix fmt

.PHONY: all clean realzador_test desenfocador_test docs fmt