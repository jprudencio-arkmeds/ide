BIN_DIR = bin

.PHONY: configure debug release

configure:
	cmake -S . -B build

debug:
	if not exist debug mkdir debug
	cmake -S . -B debug
	if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	cmake --build debug --config Debug

release:
	if not exist release mkdir release
	cmake -S . -B release
	if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	cmake --build release --config Release
