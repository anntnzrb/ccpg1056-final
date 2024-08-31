CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -Wshadow -Wunreachable-code -std=c11 -O2 -Iinclude
SRC_DIR = src
OBJS = $(SRC_DIR)/main.o $(SRC_DIR)/bmp.o $(SRC_DIR)/desenfocador.o

DOCS_DIR = docs

all: clean main

main: $(OBJS)
	@mkdir -p outputs
	@$(CC) $(CFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(SRC_DIR)/*.o $(SRC_DIR)/*.d main outputs/

test: main
	@./main testcases/test.bmp outputs/test_out.bmp
	@diff outputs/test_out.bmp testcases/test_sol.bmp && printf 'Test passed!\n'

testmem: main
	valgrind --tool=memcheck --leak-check=summary ./main testcases/test.bmp outputs/test_out.bmp

docs:
	@printf "Building docs...\\n"
	typst compile --root $(DOCS_DIR) $(DOCS_DIR)/main.typ 'reporte-Threads.pdf'

fmt:
	@printf "Formatting src tree...\\n"
	@nix fmt

.PHONY: all clean test testmem main docs fmt