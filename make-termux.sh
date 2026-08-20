#export LD_LIBRARY_PATH=$HOME/usr/lib
 #CFLAGS=-I$HOME/gh/lexbor/source LIB_PATH=$HOME/gh/lexbor make
 
export LDFLAGS="-landroid-wordexp -liconv"
make AHRE_QUICKJS_DISABLED=1

