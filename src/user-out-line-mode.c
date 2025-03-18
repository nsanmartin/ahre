#include "src/user-out-line-mode.h"
#include "src/session.h"

Err _line_show_session_(Session* s) {
    if (!s) return "error: unexpected null session, this should really not happen";
    if (session_is_empty(s)) return Ok;
    return session_uout(s)->flush_std(NULL);
}


Err _line_show_err_(Session* s, char* err, size_t len) {
    (void)s;
    if (err) {
        if ((len != fwrite(err, 1, len, stderr))
        || (1 != fwrite("\n", 1, 1, stderr))
        ) return "error: fprintf failure while attempting to show an error :/";
    }
    fflush(stderr);
    return Ok;
}

