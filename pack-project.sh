#!/usr/bin/env bash
set -euo pipefail

PROJECT_ID="$1"
ZIP_NAME="$PROJECT_ID"

# Read version from CMakePresets.json by following the preset's inherits (scalar only)
FILE="CMakePresets.json"
if ! command -v jq >/dev/null 2>&1; then
    echo "jq is required but not found. Install jq and try again." >&2
    exit 5
fi

VERSION=$(jq -r --arg z "$ZIP_NAME" '.configurePresets[] | select(.name==$z) | .cacheVariables.PROJECT_VERSION // empty' "$FILE")
[ -n "$VERSION" ] || { echo "PROJECT_VERSION not found for preset $ZIP_NAME" >&2; exit 6; }

echo "Using PROJECT_VERSION=$VERSION (from $FILE -> $ZIP_NAME)"
DEST_DIR="./release"

SRC_DIR="Projects/$PROJECT_ID"
if [ ! -d "$SRC_DIR" ]; then
	echo "Source project directory not found: $SRC_DIR" >&2
	exit 3
fi

TMPDIR=$(mktemp -d "${TMPDIR:-/tmp}/${PROJECT_ID}.XXXXXX")
trap 'rm -rf "$TMPDIR"' EXIT

echo "Preparing pack in temporary dir: $TMPDIR"
for d in Scripts Meshes Textures Materials Interface F4SE MCM; do
 	if [ -d "$SRC_DIR/$d" ]; then
 		echo "  - Copying $d"
 		cp -a "$SRC_DIR/$d" "$TMPDIR/"
 	fi
done

found=false
for f in "$SRC_DIR"/*.esp "$SRC_DIR"/*.esm "$SRC_DIR"/*.esl; do
 	if [ -f "$f" ]; then
 		found=true
 		echo "  - Copying $(basename "$f")"
 		cp -a "$f" "$TMPDIR/"
 	fi
done

if [ "$found" = false ]; then
 	echo "  - No .esp/.esm/.esl files found in $SRC_DIR"
fi

mkdir -p "$DEST_DIR"

ZIPFILE="$DEST_DIR/${ZIP_NAME}_v${VERSION}.zip"
ZIPFILE_ABS="$(cd "$DEST_DIR" && pwd -P)/${ZIP_NAME}_v${VERSION}.zip"

if command -v 7z >/dev/null 2>&1; then
 	( cd "$TMPDIR" && 7z a -tzip -mx=9 "$ZIPFILE_ABS" . >/dev/null )
else
 	echo "7z not found; please install p7zip or the 7z CLI" >&2
 	exit 4
fi

echo "Pack complete: $ZIPFILE"
