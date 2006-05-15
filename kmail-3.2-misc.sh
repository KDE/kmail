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
	Geometry/MimePaneHeight)
	    case "$VALUE" in
		[0-9]|[1-9][0-9]|[1-9][0-9][0-9]|[1-9]) ;;
		*) VALUE=100 ;;
	    esac
	    GeometryMimePaneHeight="$VALUE"
	    ;;
	Geometry/MessagePaneHeight)
	    delete_this_key;
	    case "$VALUE" in
		[0-9]|[1-9][0-9]|[1-9][0-9][0-9]|[1-9]) ;;
		*) VALUE=180 ;;
	    esac
	    GeometryMessagePaneHeight="$VALUE"
	    ;;
	Geometry/FolderPaneHeight)
	    #
	    # keys to delete
	    #
	    delete_this_key
	    ;;
	Geometry/windowLayout)
	    #
	    # break [Geometry]windowLayout={0,1,2,3,4} into
	    # [Geometry]FolderList={long,short} and
      	    # [Reader]MimeTreeLocation={top,bottom}
       	    #
	    delete_this_key
	    case "$VALUE" in
		[0-4]) ;;
		*) VALUE=1 ;;
	    esac
	    location=("top" "bottom" "bottom" "top" "top")
	    folder=("long" "long" "long" "short" "short")
	    echo "[Reader]"
	    echo "MimeTreeLocation=${location[$VALUE]}"
	    echo "[Geometry]"
	    echo "FolderList=${folder[$VALUE]}"
	    continue;
	    ;;
	Geometry/showMIME)
	    #
	    # Rename [Geometry]showMime={0,1,2} into
	    # [Reader]MimeTreeMode={never,smart,always}
	    #
	    delete_this_key
	    case "$VALUE" in
		[0-2]) ;;
		*) VALUE=1 ;;
	    esac
	    substitution=("never" "smart" "always")
	    echo "[Reader]"
	    echo "MimeTreeMode=${substitution[$VALUE]}"
	    continue;
	    ;;
    esac
done

if [ "$GeometryMimePaneHeight" ] && [ "$GeometryMessagePaneHeight" ]; then
    echo "[Geometry]"
    echo "ReaderPaneHeight=$(($GeometryMimePaneHeight+$GeometryMessagePaneHeight))"
fi
