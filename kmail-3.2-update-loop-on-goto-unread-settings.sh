#!/bin/bash

while read; do
    KEY="${REPLY%%=*}"
    VALUE="${REPLY#*=}"
    if [ "$KEY" = "LoopOnGotoUnread" ]; then
	case "$VALUE" in
	    [Ff][Aa][Ll][Ss][Ee]|0)
	        echo "LoopOnGotoUnread=0"
		;;
	    *)  # default
	        echo "LoopOnGotoUnread=1"
		;;
	esac
    else
	echo "$REPLY"
    fi
done
