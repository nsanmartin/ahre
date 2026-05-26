_vahre_ () {
    if [ $# -eq 0 ]; then echo You must provide error filename; return 1; fi
    ERRFILE=$1
    shift
    make && valgrind --leak-check=full  --track-origins=yes --show-leak-kinds=all  ./build/ahre "$@" \
        2> err/${ERRFILE};
}
vahre0  () { _vahre_ 0  "$@"; }
vahre01 () { _vahre_ 01 "$@"; }
vddg0   () { local IFS="+"; _vahre_ 0 -d q="$*" lite.duckduckgo.com/lite; }
gahre () {
    gdb -ex 'run' --args ./build/ahre "$@";
}


gahrebug () {
    gdb -ex 'run' --args ./build/debug/ahre "$@";
}
