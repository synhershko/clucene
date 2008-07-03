#!/bin/sh
#Check compliance with Coding standards...

TMP=.

#create environment...
#cmake ..
rm src/CLucene/_clucene-config.h 2>/dev/null

#create header file for testing of symbols in headers.
echo "#include \"CLucene/StdHeader.h"\" >$TMP/pub-headers.cpp

#check to see that no #ifdefs exist in headers that don't belong
function checkForIfdefs {
    I=0
    grep "#if" $1| while read line; do
        I=`expr $I + 1`
        if [ $I -gt 1 ]; then
            echo $1 has invalid ifdef: $line
            exit 1
        fi
    done
}

#iterate all headers
for H in `find ../src/CLucene| grep "\.h$"`; do
    BH=`basename $H`
    if [ "${BH:0:1}" != "_" ]; then
        DH=`dirname ${H:3}`
        mkdir -p $DH 2>/dev/null
    
        cp $H ${H:3}
        #checkForIfdefs $H
        echo "#include \"${H:7}\"" >>$TMP/pub-headers.cpp
    fi
done

#test if headers contain any symbols:
echo "int main(){return 0;}"  >>$TMP/pub-headers.cpp
#g++ -Isrc pub-headers.cpp

#find inline code:
make DoxygenDoc
echo "These documentation files report inline code. This is not good."
grep -c "\[inline" doc/html/*.html|grep -v ":0$"|grep -v "util"|grep -v "jstreams" | nl
