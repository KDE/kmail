#! /usr/bin/env bash

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

    case "$GROUP" in
	Identity*)
	    echo "# got Identity Key \"$KEY\""
	    case "$KEY" in
		"Default PGP Key")
		    echo "[$GROUP]"
            echo "PGP Signing Key=$VALUE"
		    echo "[$GROUP]"
            echo "PGP Encryption Key=$VALUE"
		    delete_this_key
		    ;;
	    esac
	    ;;
    esac
done
