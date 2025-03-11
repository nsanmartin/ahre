#include "src/user-out-line-mode.h"
#include "src/session.h"

Err ui_show_session_line_mode(Session* s) {
    if (!s) return "error: unexpected null session, this should really not happen";
    return session_uout(s)->flush_std();
}
