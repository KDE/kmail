#!/bin/bash

function delete_this_key() {
    echo "# DELETE [$GROUP]$KEY"
}

while read; do
    # Currently unused
    if [ "${REPLY#\[}" != "$REPLY" ] ; then # group name
	GROUP="${REPLY:1:${#REPLY}-2}"
	continue;
    fi
    # normal key=value pair:
    KEY="${REPLY%%=*}"
    VALUE="${REPLY#*=}"

    case "$GROUP/$KEY" in
	/pgpType)
            # 0 is auto -> delete key, i.e. use the default
            # 1-4 map to the kpgp ones
            # 5 is off -> off; since there's no such backend it won't set one
            if [ "$VALUE" -ne 0 ]; then
	        location=("" "Kpgp/gpg1" "Kpgp/pgp v2" "Kpgp/pgp v5" "Kpgp/pgp v6" "")
	        echo "OpenPGP=${location[$VALUE]}"
	    fi
	    continue;
	    ;;
    esac
done
