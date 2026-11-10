#!/bin/bash
# Usage: make-dmg.sh <app-dir-containing-Aura Generator.app> <output.dmg> <background.png>
set -euo pipefail
src="$1"; out="$2"; bg="$3"
rm -f "$out"
if command -v create-dmg >/dev/null 2>&1; then
    if create-dmg --volname "Aura Generator" --background "$bg" \
        --window-pos 200 120 --window-size 600 400 --icon-size 100 \
        --icon "Aura Generator.app" 150 190 --hide-extension "Aura Generator.app" \
        --app-drop-link 450 190 "$out" "$src"; then
        exit 0
    fi
    echo "create-dmg failed; falling back to a plain DMG" >&2
    rm -f "$out"
fi
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
cp -R "$src/." "$tmp/"
ln -s /Applications "$tmp/Applications"
hdiutil create -volname "Aura Generator" -srcfolder "$tmp" -ov -format UDZO "$out"
