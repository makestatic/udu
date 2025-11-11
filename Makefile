BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
TARGET = udu
C_SRC = c/udu.c
OPT_SAFE = ReleaseFast
OPT_DEBUG = Debug

.PHONY: all clean install c zig release debug

# Default: build C binary
all: c

c:
	@echo "Building C binary..."
	mkdir -p $(BIN_DIR)
	# GCC Preferable
	cc -g -O3 -march=native -flto -fopenmp -std=gnu99 -Wall -Wextra -o $(BIN_DIR)/$(TARGET) $(C_SRC)

zig:
	@echo "Building Zig binary..."
	cd zig && zig build -Doptimize=$(OPT_SAFE) --prefix ../$(BUILD_DIR) --summary all

# release:
# 	cd zig && zig build -Dbuild-all-targets --prefix ../$(BUILD_DIR)

# debug:
# 	cd zig && zig build -Doptimize=$(OPT_DEBUG) --prefix ../$(BUILD_DIR)

install: 
	@if [ "$(INSTALL_TARGET)" = "zig" ]; then \
		echo "Installing Zig binary..."; \
		sudo mv $(BUILD_DIR)/zig-out/bin/$(TARGET) /usr/local/bin/$(TARGET); \
		echo "Installed Zig binary to /usr/local/bin/$(TARGET)"; \
	else \
		echo "Installing C binary..."; \
		sudo mv $(BIN_DIR)/$(TARGET) /usr/local/bin/$(TARGET); \
		echo "Installed C binary to /usr/local/bin/$(TARGET)"; \
	fi

clean:
	@echo "Cleaning build directories..."
	rm -rf $(BUILD_DIR)
	mkdir -p $(BIN_DIR)
