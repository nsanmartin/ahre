DIR=$HOME/cb/nsm/ahre

_vahre_() {
    local ERRFILE=${1:-0}; [ $# -gt 0 ] && shift
    echo stderr sent to "$DIR"/err/"$ERRFILE"
    echo "$@" >&2
    make -C $DIR && \
        valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all \
        $DIR/build/ahre "$@" 2> $DIR/err/"$ERRFILE"
}
vahre() { _vahre_ "$@"; }
vddg0()   { local IFS="+"; _vahre_ 0  -d q="$*" lite.duckduckgo.com/lite; }
gahre()    { gdb -ex 'run' -ex 'set print pretty on' --args $DIR/build/ahre "$@"; }
gahrebug() { gdb -ex 'run' --args $DIR/build/debug/ahre "$@"; }
