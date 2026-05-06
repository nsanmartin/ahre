vahre0 () {
    make && valgrind --leak-check=full  --track-origins=yes --show-leak-kinds=all  ./build/ahre "$@" 2> err/0;
}

gahre () {
    gdb --args ./build/ahre "$@";
}
