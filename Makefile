SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
COMMON_SRCS := filter_common.c bmp.c util.c
COMMON_OBJS := $(addprefix $(OBJ_DIR)/,$(COMMON_SRCS:.c=.o))
TARGETS := pipeline desenfocador realzador

CC := cc
CFLAGS := -Wall -Wextra -std=c99 -I$(INC_DIR) -pthread -Wno-pragma-pack
LDFLAGS := -pthread

# OS-specific settings
# -lrt is unavailable on darwin
ifeq ($(shell uname -s),Linux)
    LDFLAGS += -lrt
endif

all: info $(TARGETS) | outputs

info:
	@printf "CC: ${CC}\\n"
	@printf "CFLAGS: ${CFLAGS}\\n"
	@printf "LDFLAGS: ${LDFLAGS}\\n"
	@printf "SAMPLE_IMAGES: ${SAMPLE_IMAGES}\\n\\n"

$(TARGETS): | outputs $(OBJ_DIR)
	$(CC) $(CFLAGS) ${^} -o ${@} $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c ${<} -o ${@}

pipeline: $(filter-out $(OBJ_DIR)/desenfocador.o $(OBJ_DIR)/realzador.o, $(OBJS))

desenfocador realzador: %: $(OBJ_DIR)/%.o $(COMMON_OBJS)

$(OBJ_DIR) outputs:
	@mkdir -p ${@}

clean:
	rm -Rf $(TARGETS) $(OBJ_DIR) outputs
	find . -name "*.pdf" -type f -delete

# ============================================================================
# TESTING
# ============================================================================
SAMPLE_IMAGES := test wizard airplane car purduetrain circle city

samples: all
	@for img in $(SAMPLE_IMAGES); do \
		printf "./samples/$$img.bmp\nq\n" | ./pipeline 4 ./outputs/$${img}_sol.bmp; \
	done

# ============================================================================
# docs
# ============================================================================
DOCS_DIR = docs
REPORT_FILE = g7-reporte.pdf

docs:
	@printf "Building docs...\\n"
	typst compile --root $(DOCS_DIR) $(DOCS_DIR)/main.typ $(REPORT_FILE)

docs-watch:
	@printf "Watching docs...\\n"
	typst watch --root $(DOCS_DIR) $(DOCS_DIR)/main.typ $(REPORT_FILE)

# ============================================================================
# Docker
# ============================================================================
DOCKER_IMAGE_NAME := ccpg1056-final

docker-build:
	docker build -t $(DOCKER_IMAGE_NAME) .

docker-run: docker-build
	docker run -it --rm -v $(PWD):/app $(DOCKER_IMAGE_NAME)

docker-clean:
	docker rmi $(DOCKER_IMAGE_NAME)

.PHONY: all info clean samples docs docs-watch docker-build docker-run docker-clean