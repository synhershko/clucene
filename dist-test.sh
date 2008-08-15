#!/bin/bash
#Check compliance with Coding standards...

#where to keep the temp files...
TMP=disttest

function usage {
    echo "usage: ../dist-test.sh [all | "
    echo "    <env - creates environment>"
    echo "    <c_all - compile all headers together>"
    echo "    <compile - compile and test>"
    echo "    <inline - test for inline using doxygen documents>]"
    echo "    <c_header - test that each header compiles independently of each other>]"
    echo "    <license - test that each header has a valid license>]"
    echo "    <ifdefs - test that each header doesn't have invalid ifdefs>]"
    echo "    <exports - test that each header exports all its classes>]"
    exit 1;
}
t_all=0
t_env=0
t_c_all=0
t_inline=0
t_compile=0
t_c_h=0
t_license=0
t_ifdefs=0
t_exports=0
FAIL=0

if [ $# == 0 ]; then
    usage
else
    while [ "$1" != "" ]; do
        if [ "$1" == "all" ]; then
            t_all=1
        elif [ "$1" == "env" ]; then
            t_env=1
        elif [ "$1" == "c_all" ]; then
            t_c_all=1
        elif [ "$1" == "inline" ]; then
            t_inline=1
        elif [ "$1" == "compile" ]; then
            t_compile=1
        elif [ "$1" == "c_header" ]; then
            t_c_h=1
        elif [ "$1" == "license" ]; then
            t_license=1
        elif [ "$1" == "ifdefs" ]; then
            t_ifdefs=1
        elif [ "$1" == "exports" ]; then
            t_exports=1
        else
            usage
        fi
        shift
    done
fi

if [ $t_all == 1 ]; then
    t_env=1
    t_c_all=1
    t_c_h=1
    t_inline=1
    t_compile=1
    t_license=1
    t_ifdefs=1
    t_exports=1
fi


#check to see that no #ifdefs exist in headers that don't belong
function checkForIfdefs {
    I=0
    grep "#if" $1| while read line; do
        I=`expr $I + 1`
        if [ $I -gt 1 ]; then
            echo $1 has invalid ifdef: $line
            FAIL=1
        fi
    done
}


if [ $t_env == 1 ]; then
    mkdir $TMP 2>/dev/null
    
    #create header file for testing of symbols in headers.
    echo "#include \"CLucene/StdHeader.h"\" >$TMP/pub-headers.cpp
fi

#iterate all headers
if [ $t_env == 1 ]; then
    for H in `find ../src/shared/CLucene| grep "\.h$"` `find ../src/core/CLucene| grep "\.h$"`; do
        BH=`basename "$H"`
        DN=`dirname "$H"`
        if [ "${BH:0:1}" != "_" ]; then
            DH=`dirname "${H:3}"`
        
            #move headers somewhere to compile
            mkdir -p "$TMP/$DH" 2>/dev/null
            ln -s "`cd "$DN" && pwd`" "$TMP/${H:3}" 2>/dev/null
            
            #create pub-headers.cpp
            echo "#include \"${H:7}\"" >>$TMP/pub-headers.cpp
        fi
    done
fi

if [ $t_env == 1 ]; then
    echo "int main(){return 0;}"  >>$TMP/pub-headers.cpp
fi

#find inline code:
if [ $t_inline == 1 ]; then
    if [ $t_env == 1 ]; then
        cmake -DENABLE_CLDOCS:BOOLEAN=TRUE .
        make doc
        if [ $? != 0 ]; then 
            exit 1
        fi
    fi
fi

################################################
#now the environment is finished being setup...
################################################
echo "Starting tests..."

if [ $t_c_h == 1 ] || [ $t_ifdefs == 1 ] || [ $t_exports == 1 ]; then
    for H in `find $TMP/src | grep "\.h$"`; do
        BH=`basename "$H"`
        DH=`dirname "${H:3}"`
        
        if [ $t_ifdefs == 1 ]; then
            checkForIfdefs $H
        fi
    
        #check that all classes are exported
        if [ $t_exports == 1 ]; then
            XX=`awk '/^[ \t]*(class|struct)/ { print $line }' $H| grep -v ";$"| grep -v CLUCENE_EXPORT| grep -v CLUCENE_INLINE_EXPORT| grep -v CLUCENE_SHARED_EXPORT| grep -v CLUCENE_SHARED_INLINE_EXPORT`
            if [ "$XX" != "" ]; then
                echo "$H has unexported class: $XX"
                echo ""
                FAIL=1
            fi
        fi
        
        #test that each header compiles independently...
        if [ $t_c_h == 1 ]; then
            echo "#include \"CLucene/StdHeader.h"\" >$TMP/pub-header.cpp
            echo "#include \"$H"\" >>$TMP/pub-header.cpp
            echo "int main(){return 0;}"  >>$TMP/pub-header.cpp
            g++ -I. -I$TMP/src/shared -I$TMP/src/core $TMP/pub-header.cpp
            if [ $? != 0 ]; then 
                FAIL=1; 
            fi
        fi
    done
fi

#iterate all our code...
if [ $t_license == 1 ]; then
    for H in `find ../src`; do
        BH=`basename "$H"`
        BH_len=${#BH}
        
        if [ "${BH:BH_len-2}" == ".h" ] || [ "${BH:BH_len-2}" == ".c" ] || [ "${BH:BH_len-4}" == ".cpp" ]; then
            if [ "`awk '/\* Copyright \(C\) [0-9]*-[0-9]* .*$/ { print $line }' $H`" == "" ]; then
                if [ "`awk '/\* Copyright [0-9]*-[0-9]* .*$/ { print $line }' $H`" == "" ]; then
                    echo "$H has invalid license"
                    FAIL=1
                fi
            fi
        fi
    done

fi


#test if headers can compile together by themselves:
if [ $t_c_all == 1 ]; then
    g++ -I$TMP/src -I$TMP/src/shared -I$TMP/src/core $TMP/pub-headers.cpp
fi

if [ $t_inline == 1 ]; then
    INLINES=0
    grep -c "\[inline" doc/html/*.html|grep -v ":0$"|grep -v "util"|grep -v "jstreams" | while read line; do
    
        #ignore some actual inlines...
        if [ "doc/html/classlucene_1_1index_1_1Term.html:1" == $line ]; then
            continue;
        fi

        if [ $INLINES == 0 ]; then
            echo "These files report inline code:"
            INLINES=1
            FAIL=1
        fi
        echo $line
    done
fi

if [ $t_compile == 1 ]; then
    #compile serperately
    make cl_test
    if [ $? != 0 ]; then 
        FAIL=1; 
    fi
    
    #compile together
    make test-all
    if [ $? != 0 ]; then 
        FAIL=1; 
    fi
fi


if [ $FAIL == 1 ]; then
    echo "There were errors, please correct them and re-run"
    exit 1
fi
exit 0
