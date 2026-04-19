BUILD_DIR = ./build
BIN_DIR = ./bin

build: | $(BUILD_DIR)
	cmake -S . -B $(BUILD_DIR)

debug: build | $(BIN_DIR)
	cmake --build $(BUILD_DIR) --config Debug

release: build | $(BIN_DIR)
	cmake --build $(BUILD_DIR) --config Release

$(BUILD_DIR):
	$(call MKDIR,$(BUILD_DIR))

$(BIN_DIR):
	$(call MKDIR,$(BIN_DIR))