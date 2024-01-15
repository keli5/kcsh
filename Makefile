AARCH64-CC = aarch64-linux-gnu-g++
X86_64-CC  = x86_64-linux-gnu-g++

OUTPUTDIR = ./build

RELEASE_OUTPUTDIR = $(OUTPUTDIR)/release
DEBUG_OUTPUTDIR = $(OUTPUTDIR)/debug

CCFLAGS = -std=c++20 -Wall -Wextra

RELEASE_CCFLAGS = $(CCFLAGS) -Os
DEBUG_CCFLAGS = $(CCFLAGS) -g3 -DNDEBUG

RELEASE_TARGETS = x86_64 aarch64
DEBUG_TARGETS = x86_64-debug aarch64-debug

release: $(RELEASE_TARGETS)
debug: $(DEBUG_TARGETS)

all: $(RELEASE_TARGETS) $(DEBUG_TARGETS)

# Build if target compiler exists
$(filter-out all, $(RELEASE_TARGETS)): main.cpp
	@if ! which $($(shell echo $@ | tr '[:lower:]' '[:upper:]')-CC) > /dev/null 2>&1; then \
		echo "Error: Compiler for target $@ not found."; \
		exit 1; \
	fi
	mkdir -p $(RELEASE_OUTPUTDIR)
	$($(shell echo $@ | tr '[:lower:]' '[:upper:]')-CC) -o $(RELEASE_OUTPUTDIR)/kcsh-$@ $(RELEASE_CCFLAGS) main.cpp

# Pattern rule for building debug targets
$(filter-out all, %-debug): main.cpp
	@if ! which $($(shell echo $* | tr '[:lower:]' '[:upper:]')-CC) > /dev/null 2>&1; then \
		echo "Error: Compiler for target $@ not found."; \
		exit 1; \
	fi
	mkdir -p $(DEBUG_OUTPUTDIR)
	$($(shell echo $* | tr '[:lower:]' '[:upper:]')-CC) -o $(DEBUG_OUTPUTDIR)/kcsh-$@ $(DEBUG_CCFLAGS) main.cpp

# Clean up intermediate files and executables
clean:
	rm -rf $(OUTPUTDIR)/*

# Declare the "clean" target as a phony target to ensure it always runs
.PHONY: clean
