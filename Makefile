BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
TARGET = udu
OPT_SAFE = ReleaseFast
OPT_DEBUG = Debug

.PHONY: all clean run install testing debug

all: native
native: clean
	zig build -Doptimize=$(OPT_SAFE) --prefix $(BUILD_DIR) --summary all -Dstrip
release: clean
	zig build -Dbuild-all-targets --prefix $(BUILD_DIR)
clean:
	rm -rf $(BUILD_DIR)
run: $(BIN_DIR)/$(TARGET)
	@echo "Running $(TARGET)..."
	@$(BIN_DIR)/$(TARGET)
install:
	@echo "Installing $(TARGET)..."
	mv $(BIN_DIR)/$(TARGET) /usr/local/bin/$(TARGET)
	@echo "Installed $(TARGET) to /usr/local/bin/$(TARGET)"
debug: clean
	zig build -Doptimize=$(OPT_DEBUG) --prefix $(BUILD_DIR)
