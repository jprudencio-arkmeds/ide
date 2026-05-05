BUILD_DIR = build
BIN_DIR = bin

.PHONY: configure debug release

configure: $(BUILD_DIR)
	cmake -S . -B $(BUILD_DIR)

debug: configure | $(BIN_DIR)
	cmake --build $(BUILD_DIR) --config Debug

release: configure | $(BIN_DIR)
	cmake --build $(BUILD_DIR) --config Release

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)
	