CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -std=c11 -Iinclude
LDFLAGS ?=
LDLIBS ?= -lm
SRC := $(wildcard src/*.c) $(wildcard src/core/*.c)
OBJ := $(SRC:.c=.o)
BIN := loogal

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f src/*.o src/core/*.o $(BIN)

safe-clean: clean
	@echo "Preserving data/, logs/, manifests/"

test: $(BIN)
	bash tests/smoke.sh

.PHONY: all clean safe-clean test

test-search-json: $(BIN)
	bash tests/test_search_json_build.sh


test-action: $(BIN)
	bash tests/test_action_module.sh

.PHONY: test-action


test-session: $(BIN)
	bash tests/test_session_module.sh

.PHONY: test-session


test-history: $(BIN)
	bash tests/test_history_module.sh

.PHONY: test-history


test-similar: $(BIN)
	bash tests/test_similar_module.sh

.PHONY: test-similar


test-window-api: $(BIN)
	bash tests/test_window_api_module.sh

.PHONY: test-window-api


test-thumbnail: $(BIN)
	bash tests/test_thumbnail_module.sh

.PHONY: loogal-window test-window

loogal-window: src/gui/loogal_window.c include/gui/loogal_window.h loogal
	$(CC) $(CFLAGS) -Iinclude -o loogal-window src/gui/loogal_window.c $$(pkg-config --cflags --libs gtk4)

test-window:
	bash tests/test_window_build.sh

loogal-gallery-window: src/gui/loogal_gallery_window.c
	$(CC) $(CFLAGS) -Iinclude -o loogal-gallery-window src/gui/loogal_gallery_window.c $(shell pkg-config --cflags --libs gtk4)

test-gallery-window:
	bash tests/test_gallery_window_build.sh


test-delta:
	bash tests/test_delta_module.sh

.PHONY: test-delta


test-native-image: $(BIN)
	bash tests/test_native_image_backend.sh

.PHONY: test-native-image


test-native-similarity: $(BIN)
	bash tests/test_native_similarity_backend.sh

.PHONY: test-native-similarity


test-shadow-core:
	bash tests/test_shadow_core.sh

.PHONY: test-shadow-core


test-native-ahash: $(BIN)
	bash tests/test_native_ahash_backend.sh

.PHONY: test-native-ahash


test-shadow-search: $(BIN)
	bash tests/test_shadow_search_engine.sh

.PHONY: test-shadow-search


check: $(BIN)
	bash tests/test_delta_module.sh
	bash tests/test_native_image_backend.sh
	bash tests/test_native_ahash_backend.sh
	bash tests/test_shadow_core.sh
	bash tests/test_shadow_search_engine.sh
	bash tests/test_native_similarity_backend.sh

.PHONY: check
