#!/bin/bash

DISPLAY=":1"
if [ "$1" != "" ]; then
DISPLAY="$1"
fi

make &&
cd data && sudo make install && cd .. &&
(
if [ "$BUS" == "session" ]; then
DISPLAY=$DISPLAY src/mokopanel
else
DISPLAY=$DISPLAY DBUS_SYSTEM_BUS_ADDRESS="tcp:host=neo,port=8000" src/mokopanel
fi
)
