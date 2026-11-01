#!/bin/sh
# Adds the DAISY engine to a Felucca checkout.
#   ports/fm1/felucca/apply.sh /path/to/hugelton_Felucca
# Tested against Felucca 406b328 (engines submodule d026007, bsp 5942e68).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
FM1=$(dirname "$HERE")
F=${1:?usage: apply.sh /path/to/Felucca}
[ -f "$F/firmware/src/engines.c" ] || { echo "not a Felucca checkout: $F" >&2; exit 1; }

if grep -q ENGI_DAISY "$F/firmware/src/core.h"; then
    echo "patch already applied, refreshing the engine files only"
else
    git -C "$F" apply --check "$HERE/felucca-daisy.patch"
    git -C "$F" apply "$HERE/felucca-daisy.patch"
fi
cp "$HERE/eng_daisy.c" "$FM1/src/daisy_fx.c" "$FM1/src/daisy_fx.h" \
   "$FM1/src/daisy_fx_tables.h" "$FM1/src/daisy_lyrics.h" "$F/firmware/src/"
mkdir -p "$F/tests/daisy"
cp "$FM1/tests/felucca_daisy_test.c" "$FM1/tests/level_cmp.c" "$FM1/tests/cost.c" "$F/tests/daisy/"
echo "DAISY added to $F (engine 15). Build as usual: see Felucca's BUILDING.md."
