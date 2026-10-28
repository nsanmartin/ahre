export LDFLAGS="-landroid-wordexp -liconv"
export CFLAGS=-fsanitize=address
make AHRE_QUICKJS_DISABLED=1
