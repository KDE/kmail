#!/bin/bash

function delete_this_key() {
    echo "# DELETE [$GROUP]$KEY"
}

while read; do
    if [ "${REPLY#\[}" != "$REPLY" ] ; then # group name
        GROUP="${REPLY:1:${#REPLY}-2}"
        continue;
    fi
    # normal key=value pair:
    KEY="${REPLY%%=*}"
    VALUE="${REPLY#*=}"

    case "$GROUP/$KEY" in
        General/sendOnCheck)
            delete_thisٍ_key;
            if [ "$VALUE" == "true" ] ; then
                VALUE="SendOnManualCheck"
            else
                VALUE="DontSendOnCheck"
            fi
            echo "[Behaviour]"
            echo "SendOnCheck=$VALUE"
            ;;
    esac
done
