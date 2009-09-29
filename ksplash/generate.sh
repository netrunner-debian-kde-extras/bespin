#!/bin/sh

if [ $# != 2 ]; then
    echo "usage: $0 <width> <height>"
    exit;
fi

if [ ! -e background.png ]; then
    convert ../kdm/background.jpg background.png
fi

X=$(( ($1-740)/2 ))
Y=$(( ($2-420)/2 ))
DIR="${1}x${2}"

mkdir "$DIR" 2>/dev/null

echo "SCALE ON
BACKGROUND_IMAGE 0 0 ../background.png
SCALE OFF

ANIM 1 $X $Y 10 ../init.png 80 1
WAIT_STATE kded
STOP_ANIM 1

ANIM 2 $X $Y 10 ../pulse.png 80 1
WAIT_STATE kcminit
STOP_ANIM 2

ANIM 3 $X $Y 10 ../pulse.png 80 1
WAIT_STATE ksmserver
STOP_ANIM 3

ANIM 4 $X $Y 10 ../pulse.png 80 1
WAIT_STATE wm
STOP_ANIM 4

ANIM 5 $X $Y 10 ../pulse.png 80 1
WAIT_STATE desktop
STOP_ANIM 5

ANIM 6 $X $Y 10 ../pulse.png 80 1
WAIT_STATE ready
STOP_ANIM 6
" > "$DIR/description.txt"