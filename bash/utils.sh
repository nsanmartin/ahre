vahre_ () {
    if [ $# -eq 0 ]; then echo You must provide error filename; return 1; fi
    ERRFILE=$1
    shift
    make && valgrind --leak-check=full  --track-origins=yes --show-leak-kinds=all  ./build/ahre "$@" \
        2> err/${ERRFILE};
}
vahre0  () { vahre_ 0  "$@"; }
vahre01 () { vahre_ 01 "$@"; }

gahre () {
    gdb -ex 'run' --args ./build/ahre "$@";
}
