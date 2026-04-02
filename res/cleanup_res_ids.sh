#!/bin/bash

# Check all ids in STRINGTABLE
# But ignore IDS_NFO_OVPN_STATE_ since those are accessed by offset
echo "Checking for unused ids"
for def in $(grep "^#define IDS_" openvpn-gui-res.h \
                 | grep -v IDS_NFO_OVPN_STATE_ \
                 | cut -d' ' -f2 )
do
    git grep -q $def *.c plap/*.c || echo "Didn't find $def"
    # Note that ids should be checked manually before removal
    #sed -i -e "/$def/d" openvpn-gui-res.h res/openvpn-gui-res*.rc
done

for trans in res/openvpn-gui-res-*.rc
do
    echo "Checking for obsolete translations in $trans"
    for def in $(grep "^ *IDS_" $trans | sed -e 's/^ *//' | cut -d' ' -f1)
    do
        git grep -q $def openvpn-gui-res.h || echo "$trans: Obsolete translation for $def"
    done
done
