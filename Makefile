# ==============================================================================
# Compiler & Flags
# ==============================================================================
# clang++ when installed (the compiler this project is checked with), otherwise g++
CXX      := $(shell command -v clang++ >/dev/null 2>&1 && echo clang++ || echo g++)
CXXFLAGS := -Wall -Wextra -Wshadow -O2 -std=c++20
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
# Everything but the GTK front end (src/gtk, built by `make gtk`)
SRCS != find $(SRC_DIR) -type f -name '*.cpp' -not -path '$(SRC_DIR)/gtk/*'
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)
TARGET   := $(BIN_DIR)/linalg_viz

# The GTK4 front end (docs/gtk-port.md): src/gtk plus the shared core (everything but the raylib UI: app/, edu/,
# the panels in ui/; ui/core still draws the 3D labels for now). Needs gtkmm-4.0, libadwaita-1 and epoxy
# (pkg-config; epoxy comes with GTK).
GTK_TARGET := $(BIN_DIR)/axismundi-gtk
GTK_SRCS != find $(SRC_DIR)/gtk -type f -name '*.cpp' 2>/dev/null
GTK_CORE := $(filter-out $(SRC_DIR)/main.cpp $(SRC_DIR)/app/% $(SRC_DIR)/edu/% $(SRC_DIR)/ui/%,$(SRCS)) \
            $(addprefix $(SRC_DIR)/ui/, SceneIO.cpp RayguiImpl.cpp core/Ui.cpp core/Theme.cpp core/Icons.cpp)
GTK_OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(GTK_CORE) $(GTK_SRCS))
GTK_PKGS := gtkmm-4.0 libadwaita-1 epoxy
# Lazy (=): pkg-config runs only when the GTK app is built; -isystem keeps the library headers' warnings out
GTK_CFLAGS = $(patsubst -I%,-isystem %,$(shell pkg-config --cflags $(GTK_PKGS)))
GTK_LIBS = $(shell pkg-config --libs $(GTK_PKGS))

# Math library + unit tests (no raylib needed)
MATH_SRCS != find $(SRC_DIR)/math $(SRC_DIR)/animation $(SRC_DIR)/scene $(SRC_DIR)/export $(SRC_DIR)/text -type f -name '*.cpp'; echo $(SRC_DIR)/ui/SceneIO.cpp
TEST_SRCS != find $(TEST_DIR) -type f -name '*.cpp' 2>/dev/null
TEST_OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(MATH_SRCS)) \
             $(patsubst $(TEST_DIR)/%.cpp, $(BIN_DIR)/tests/%.o, $(TEST_SRCS))
TEST_TARGET := $(BIN_DIR)/run_tests

.PHONY: default build run clean help test fonts icon install uninstall gtk run-gtk
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

## build the GTK4 front end (bin/axismundi-gtk; work in progress, see docs/gtk-port.md)
gtk: $(GTK_TARGET)

## run the GTK4 front end
run-gtk: gtk
	@$(GTK_TARGET)

$(GTK_TARGET): $(GTK_OBJS) | $(BIN_DIR)
	@echo "linking: $@"
	@$(CXX) $(GTK_OBJS) $(LDFLAGS) $(GTK_LIBS) $(LDLIBS) -o $@

# GTK sources need the GTK headers
$(BIN_DIR)/gtk/%.o: $(SRC_DIR)/gtk/%.cpp | $(BIN_DIR)
	@echo "compiling: $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) $(GTK_CFLAGS) -c $< -o $@

# compile files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BIN_DIR)
	@echo "compiling: $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

## regenerate src/ui/generated/fonts.inc from assets/fonts
fonts:
	@python3 tools/fonts.py embed

## redraw the app icon (data/icons, src/ui/generated/icon.inc) with tools/icon.py
icon:
	@python3 tools/icon.py

APP_ID := com.bisonanon.axismundi
# ~/.local for a normal user; /usr/local when run as root (sudo), not root's own home
PREFIX ?= $(if $(filter 0,$(shell id -u)),/usr/local,$(HOME)/.local)
ICONS  := $(DESTDIR)$(PREFIX)/share/icons/hicolor

# An icon cache only lists the icons present when it was made, so refresh one that exists (never create one
# in a user's folder); touching the folder makes GTK look again
define refresh_icons
	@touch $(ICONS)
	@if [ -f $(ICONS)/icon-theme.cache ] && command -v gtk-update-icon-cache >/dev/null; then \
	  gtk-update-icon-cache -q -f -t $(ICONS); fi
endef

## install the app, its desktop file and icons under PREFIX (~/.local, or /usr/local with sudo)
install: build
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/axismundi
	@# The desktop entry names the program by its full path: GNOME's PATH often lacks ~/.local/bin
	install -d $(DESTDIR)$(PREFIX)/share/applications
	sed -e 's|^Exec=.*|Exec=$(PREFIX)/bin/axismundi|' -e 's|^TryExec=.*||' data/$(APP_ID).desktop \
	  > $(DESTDIR)$(PREFIX)/share/applications/$(APP_ID).desktop
	cd data/icons && find hicolor -type f -exec install -Dm644 {} $(ICONS)/../{} \;
	@# The fonts are embedded in the program; the OFL asks for their license to travel with it
	install -Dm644 -t $(DESTDIR)$(PREFIX)/share/licenses/axismundi assets/fonts/OFL-Inter.txt assets/fonts/OFL-JetBrainsMono.txt
	@# Packages (DESTDIR set) leave the caches to the system: pacman hooks rebuild them on install
ifeq ($(DESTDIR),)
	$(refresh_icons)
	@command -v update-desktop-database >/dev/null && update-desktop-database -q $(DESTDIR)$(PREFIX)/share/applications || true
	@echo "installed under $(PREFIX): look for axismundi in the app grid (log out and in if it doesn't show)"
endif

## remove what install put under PREFIX
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/axismundi $(DESTDIR)$(PREFIX)/share/applications/$(APP_ID).desktop
	cd data/icons && find hicolor -type f -exec rm -f $(ICONS)/../{} \;
	rm -rf $(DESTDIR)$(PREFIX)/share/licenses/axismundi
	$(refresh_icons)

## build and run the math unit tests
test: $(TEST_TARGET)
	@$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) | $(BIN_DIR)
	@echo "linking: $@"
	@$(CXX) $(TEST_OBJS) -o $@

$(BIN_DIR)/tests/%.o: $(TEST_DIR)/%.cpp | $(BIN_DIR)
	@echo "compiling: $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

# create directories if they don't exist
$(BIN_DIR):
	@mkdir -p $@

-include $(DEPS) $(TEST_OBJS:.o=.d) $(GTK_OBJS:.o=.d)
