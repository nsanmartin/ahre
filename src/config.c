#include "config.h"


Err resolve_bookmarks_file(const char* path, Str out[_1_]) {
    bool file_exists;
    try(resolve_path(path, &file_exists, out));

    if (file_exists) return Ok;
    Err err = Ok;
#define EMPTY_BOOKMARK \
    "<html><head><title>Bookmarks</title></head>\n<body>\n<h1>Bookmarks</h1>\n</body>\n</html>"
    FILE* fp;
    try_or_jump(err, Fail, file_open(items__(out), "w", &fp));
    try_or_jump(err, Fail, file_write_or_close(EMPTY_BOOKMARK, lit_len__(EMPTY_BOOKMARK), fp));
    try_or_jump(err, Fail, file_close(fp));
    return Ok;
Fail:
    str_clean(out);
    return err;
}
