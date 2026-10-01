_vahre_() {
    local ERRFILE=${1:-0}; [ $# -gt 0 ] && shift
    make && valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./build/ahre "$@" 2> err/"$ERRFILE"
}
vahre() { _vahre_ "$@"; }
vddg0()   { local IFS="+"; _vahre_ 0  -d q="$*" lite.duckduckgo.com/lite; }
gahre()    { gdb -ex 'run' --args ./build/ahre "$@"; }
gahrebug() { gdb -ex 'run' --args ./build/debug/ahre "$@"; }
