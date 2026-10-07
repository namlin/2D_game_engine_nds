#!/bin/bash

if [ -d "/opt/blocksds/core" ]; then
    BLOCKSDS="${BLOCKSDS:-/opt/blocksds/core/}"
else
    BLOCKSDS="${BLOCKSDS:-/opt/wonderful/thirdparty/blocksds/core/}"
fi
GRIT=$BLOCKSDS/tools/grit/grit

$GRIT ./sprites/*.png -ftB -fh! -gTFF00FF -gb -gB8 -m!

mv *.pal *.img ../nitrofiles/sprite

$GRIT ./backgrounds/*.png -ftB -fh! -gTFF00FF -gt -gB8 -mR8 -mLs

mv *.pal *.img *.map ../nitrofiles/bg
