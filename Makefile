# ==============================================================================
# Compiler & Flags
# ==============================================================================
CXX      := g++
CXXFLAGS := -Wall -Wextra -O2 -std=c++20
INCLUDES := -Isrc/include -Iinclude
LDFLAGS  :=
LDLIBS   := -lraylib -lm -lpthread -ldl
DEPFLAGS := -MMD -MP

# ==============================================================================
# Project structure
# ==============================================================================
BIN_DIR  := bin
SRC_DIR  := src
TEST_DIR := tests
SRCS != find $(SRC_DIR) -type f -name '*.cpp'
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)
TARGET   := $(BIN_DIR)/linalg_viz

.PHONY: default build run clean help
default: run

list:
	@echo $(SRCS)
	@echo $(OBJS)

# ------------------------------------------------------------------------------
# Utilities
# ------------------------------------------------------------------------------
define AWK_HELP_SCRIPT
/^## / { desc = substr($$0, 4); next; }
/^[a-zA-Z\-_0-9]+:/ {
	if (desc) {
		target = substr($$1, 1, length($$1)-1);
		printf "  \033[36m%-9s\033[0m  %s\n", target, desc;
	}
}
{ desc = "" }
endef
export AWK_HELP_SCRIPT
## display this help
help:
	@printf "\033[1mavailable targets:\033[0m\n"
	@awk "$${AWK_HELP_SCRIPT}" $(MAKEFILE_LIST)

## clean-up files inside BIN_DIR
clean:
ifeq ($(strip $(BIN_DIR)),)
	$(warning variable BIN_DIR is empty)
else
	rm -rf ./$(BIN_DIR)/*
endif


# ------------------------------------------------------------------------------
# Build Rules
# ------------------------------------------------------------------------------
## run default target
run: build
	@echo "executing: $(TARGET)"
	@$(TARGET)

## build target
build: $(TARGET)

# link binaries to make target
$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "linking: $@"
	@$(CXX) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# compile files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BIN_DIR)
	@echo "compiling: $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

# create directories if they don't exist
$(BIN_DIR):
	@mkdir -p $@

-include $(DEPS)
