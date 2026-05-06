vahre () {
    if [ $# -eq 0 ]; then echo You must provide error filename; return 1; fi
    ERRFILE=$1
    shift
    make && valgrind --leak-check=full  --track-origins=yes --show-leak-kinds=all  ./build/ahre "$@" \
        2> err/${ERRFILE};
}
vahre0 () { vahre 0 "$@"; }
vahre1 () { vahre 1 "$@"; }

gahre () {
    gdb -ex 'run' --args ./build/ahre "$@";
}
