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


test-bench-scan: $(BIN)
	bash tests/test_bench_scan.sh

.PHONY: test-bench-scan


test-native-sha256:
	bash tests/test_native_sha256.sh

.PHONY: test-native-sha256


check-release-core:
	! grep -R "magick" -n src include tests --exclude='*.o'
	! grep -R "sha256sum" -n src include --exclude='*.o'
	! grep -R "python" -n src include --exclude='*.o'
	! grep -R "ImageMagick" -n src include tests --exclude='*.o'

.PHONY: check-release-core


check-core-noshell:
	! grep -n "popen" src/hash.c src/image.c src/index.c src/memory.c src/cmd_index.c src/cmd_search.c src/cmd_stats.c src/cmd_bench_scan.c src/verify.c src/why.c src/core/*.c include/*.h include/loogal/*.h
	! grep -n -F "system(" src/hash.c src/image.c src/index.c src/memory.c src/cmd_index.c src/cmd_search.c src/cmd_stats.c src/cmd_bench_scan.c src/verify.c src/why.c src/core/*.c include/*.h include/loogal/*.h

.PHONY: check-core-noshell

test-platform-core:
	bash tests/test_platform_core.sh

.PHONY: test-platform-core

# ---------------------------------------------------------------------
# Dependency and purity audits
# ---------------------------------------------------------------------

ENGINE_FILES = \
	src/core/delta.c \
	src/core/sha256.c \
	src/core/shadow.c \
	src/error.c \
	src/hash.c \
	src/image.c \
	src/index.c \
	src/jsonout.c \
	src/log.c \
	src/manifest.c \
	src/memory.c \
	src/pathsafe.c \
	src/rebuild.c \
	src/receipt.c \
	src/timer.c \
	src/verify.c \
	src/why.c \
	include/*.h \
	include/loogal/*.h

audit-deps:
	@echo "[audit] shell/process calls"
	@grep -R -nE '\b(popen|system|fork|exec|posix_spawn)\b|"\./loogal([[:space:]]|")' src include Makefile --exclude='*.o' || true
	@echo
	@echo "[audit] desktop/gui/platform adapters"
	@grep -R -nE 'gtk|pkg-config|xdg-open|dolphin|wl-copy|xclip|xsel' src include Makefile --exclude='*.o' || true
	@echo
	@echo "[audit] forbidden external image/runtime tools"
	@grep -R -nE 'ImageMagick|magick|convert|identify|sha256sum|python' src include Makefile --exclude='*.o' || true
	@echo
	@echo "[audit] dangerous C calls"
	@grep -R -nE '\b(strcpy|strcat|sprintf|gets|scanf|sscanf)\b' src include --exclude='*.o' || true

audit-engine-no-shell:
	@echo "[audit] engine must not shell out"
	@! grep -nE '\b(popen|system|fork|exec|posix_spawn)\b|"\./loogal([[:space:]]|")' $(ENGINE_FILES)

audit-engine-no-desktop:
	@echo "[audit] engine must not know desktop/gui tools"
	@! grep -nE 'gtk|pkg-config|xdg-open|dolphin|wl-copy|xclip|xsel' $(ENGINE_FILES)

audit-engine-no-external-image-tools:
	@echo "[audit] engine must not depend on external image/runtime tools"
	@! grep -nE 'ImageMagick|magick|convert|identify|sha256sum|python' $(ENGINE_FILES)

audit-engine-c-danger:
	@echo "[audit] engine must avoid dangerous C calls"
	@! grep -nE '\b(strcpy|strcat|sprintf|gets|scanf|sscanf)\b' $(ENGINE_FILES)

audit-engine-pure: audit-engine-no-shell audit-engine-no-desktop audit-engine-no-external-image-tools audit-engine-c-danger
	@echo "[ok] engine purity wall holds"

audit-built-artifacts:
	@echo "[audit] generated artifacts accidentally tracked"
	@git ls-files | grep -E '(^loogal$$|^loogal-window$$|^loogal-gallery-window$$|\.o$$|^releases/.*\.exe$$)' || true

.PHONY: audit-deps audit-engine-no-shell audit-engine-no-desktop audit-engine-no-external-image-tools audit-engine-c-danger audit-engine-pure audit-built-artifacts

