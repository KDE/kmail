#!/bin/bash

while read; do
    KEY="${REPLY%%=*}"
    VALUE="${REPLY#*=}"
    if [ "$KEY" = "LoopOnGotoUnread" ]; then
	case "$VALUE" in
	    [Ff][Aa][Ll][Ss][Ee]|0)
	        echo "LoopOnGotoUnread=0"
		;;
	    2)  # we were run already, be nice and don't change anything
		echo "LoopOnGotoUnread=2"
		;;
	    *)  # default
	        echo "LoopOnGotoUnread=1"
		;;
	esac
    else
	echo "$REPLY"
    fi
done
