SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
COMMON_SRCS := filter_common.c bmp.c util.c
COMMON_OBJS := $(addprefix $(OBJ_DIR)/,$(COMMON_SRCS:.c=.o))
TARGETS := pipeline desenfocador realzador
DOCS_DIR = docs
REPORT_FILE = g7-reporte.pdf

CC := cc
CFLAGS := -Wall -Wextra -std=c99 -I$(INC_DIR) -pthread -Wno-pragma-pack
LDFLAGS := -pthread

# OS-specific settings
# -lrt is unavailable on darwin
ifeq ($(shell uname -s),Linux)
    LDFLAGS += -lrt
endif

all: clean $(TARGETS) | outputs

$(TARGETS): | outputs $(OBJ_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

pipeline: $(filter-out $(OBJ_DIR)/desenfocador.o $(OBJ_DIR)/realzador.o, $(OBJS))

desenfocador realzador: %: $(OBJ_DIR)/%.o $(COMMON_OBJS)

$(OBJ_DIR) outputs:
	mkdir -p $@

docs:
	@printf "Building docs...\\n"
	typst compile --root $(DOCS_DIR) $(DOCS_DIR)/main.typ $(REPORT_FILE)

docs-watch:
	@printf "Watching docs...\\n"
	typst watch --root $(DOCS_DIR) $(DOCS_DIR)/main.typ $(REPORT_FILE)

clean:
	rm -Rf $(TARGETS) $(OBJ_DIR) outputs

.PHONY: all clean docs docs-watch