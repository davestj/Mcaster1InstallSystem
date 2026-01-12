## Makefile.am — Mcaster1InstallSystem top-level
## Process with automake: autogen.sh → ./configure → make

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS  = -I m4

# manifest/, backends/, importers/ are compiled as headers/sources
# included directly into the cmake studio + runtime builds.
# Their Makefile.am files provide placeholder make targets and EXTRA_DIST.
SUBDIRS = manifest backends importers

EXTRA_DIST = VERSION autogen.sh configure.ac \
    resources/header-banner.png \
    resources/sidepanel.png \
    resources/splash.png \
    resources/icons/mcaster1.icns

# ── Qt6 CPU count helper ───────────────────────────────────────────────────────
NCPU = $$(sysctl -n hw.logicalcpu 2>/dev/null || nproc 2>/dev/null || echo 4)

# ── Studio — authoring IDE (cmake-delegated) ──────────────────────────────────
# Same pattern as `make gui` in the mcaster1dnas project:
# autotools triggers cmake; cmake handles Qt6 MOC/UIC/RCC.
.PHONY: studio studio-release studio-clean

studio:
	@if [ ! -f "config.h" ]; then \
	    echo "ERROR: config.h not found. Run ./configure first."; exit 1; \
	fi
	@echo "==> Building Mcaster1InstallStudio (Debug)..."
	@QT_PREFIX="$$(brew --prefix qt 2>/dev/null || echo /usr)" && \
	 cmake -B studio/build -S studio \
	    -DCMAKE_PREFIX_PATH="$$QT_PREFIX" \
	    -DCMAKE_BUILD_TYPE=Debug \
	    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
	 cmake --build studio/build -j$(NCPU)
	@echo "==> Studio build complete."
	@if [ "$$(uname -s)" = "Darwin" ]; then \
	    APP="studio/build/Mcaster1InstallStudio.app"; \
	    echo "==> Codesigning..."; \
	    codesign --force --sign - "$$APP" && \
	    echo "    App: $$APP"; \
	    echo "    Run: open $$APP"; \
	else \
	    echo "    Binary: studio/build/Mcaster1InstallStudio"; \
	fi

studio-release:
	@if [ ! -f "config.h" ]; then \
	    echo "ERROR: config.h not found. Run ./configure first."; exit 1; \
	fi
	@echo "==> Building Mcaster1InstallStudio (Release)..."
	@QT_PREFIX="$$(brew --prefix qt 2>/dev/null || echo /usr)" && \
	 cmake -B studio/build -S studio \
	    -DCMAKE_PREFIX_PATH="$$QT_PREFIX" \
	    -DCMAKE_BUILD_TYPE=Release && \
	 cmake --build studio/build -j$(NCPU)
	@if [ "$$(uname -s)" = "Darwin" ]; then \
	    codesign --force --sign - "studio/build/Mcaster1InstallStudio.app"; \
	fi

studio-clean:
	@echo "==> Cleaning Studio build..."
	@rm -rf studio/build
	@echo "    Done."

# ── Runtime installer wizard (cmake-delegated) ────────────────────────────────
.PHONY: runtime runtime-release runtime-clean

runtime:
	@if [ ! -f "config.h" ]; then \
	    echo "ERROR: config.h not found. Run ./configure first."; exit 1; \
	fi
	@echo "==> Building Mcaster1Installer runtime (Debug)..."
	@QT_PREFIX="$$(brew --prefix qt 2>/dev/null || echo /usr)" && \
	 cmake -B runtime/build -S runtime \
	    -DCMAKE_PREFIX_PATH="$$QT_PREFIX" \
	    -DCMAKE_BUILD_TYPE=Debug && \
	 cmake --build runtime/build -j$(NCPU)
	@echo "==> Runtime build complete."
	@if [ "$$(uname -s)" = "Darwin" ]; then \
	    codesign --force --sign - "runtime/build/Mcaster1Installer.app"; \
	    echo "    App: runtime/build/Mcaster1Installer.app"; \
	fi

runtime-release:
	@QT_PREFIX="$$(brew --prefix qt 2>/dev/null || echo /usr)" && \
	 cmake -B runtime/build -S runtime \
	    -DCMAKE_PREFIX_PATH="$$QT_PREFIX" \
	    -DCMAKE_BUILD_TYPE=Release && \
	 cmake --build runtime/build -j$(NCPU)
	@if [ "$$(uname -s)" = "Darwin" ]; then \
	    codesign --force --sign - "runtime/build/Mcaster1Installer.app"; \
	fi

runtime-clean:
	@echo "==> Cleaning runtime build..."
	@rm -rf runtime/build
	@echo "    Done."

# ── Convenience: build both ───────────────────────────────────────────────────
.PHONY: all-apps
all-apps: studio runtime
	@echo "==> All apps built."

# ── Version info ──────────────────────────────────────────────────────────────
.PHONY: version
version:
	@echo "Mcaster1InstallSystem $$(cat VERSION 2>/dev/null || echo unknown)"

# ── Clean everything ──────────────────────────────────────────────────────────
.PHONY: deep-clean
deep-clean: studio-clean runtime-clean
	@find . -name "*.o" -delete
	@echo "Deep clean complete."
