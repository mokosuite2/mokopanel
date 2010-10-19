#!/bin/bash

DISPLAY=":1"
if [ "$1" != "" ]; then
DISPLAY="$1"
fi

make &&
cd data && sudo make install && cd .. &&
sudo DISPLAY=$DISPLAY ELM_FINGER=80 ELM_SCALE=2 ELM_THEME=gry DBUS_SYSTEM_BUS_ADDRESS="tcp:host=neo,port=8000" src/mokopanel
