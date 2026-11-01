findimpl () {
    FN_NAME=$1
    grep "^$1" src/*.c 
}

# make -C utests 2>&1 \
grep -o "undefined reference to[^']*" \
    | sort -u \
    | cut -b25- \
    | while IFS= read -r sym; do
        findimpl "$sym"
      done \
    | sed 's/:.*//' \
    | sort -u \
    | sed 's/src/$(AHRE_BUILD)/' \
    | sed 's/\.c$/.o/'

