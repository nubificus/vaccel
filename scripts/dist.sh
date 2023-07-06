#!/bin/sh

cd "$MESON_SOURCE_ROOT" || exit 1
./scripts/generate-version.sh > "$MESON_DIST_ROOT"/.version
