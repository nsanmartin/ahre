#include "src/user-out-vi-mode.h"
#include "src/cmd-ed.h"
#include "src/user-out.h"
#include "src/session.h"
#include "src/screen.h"

Err _vi_print_(TextBuf textbuf[static 1], Range range[static 1], UserOutput out[static 1]) {
    try(validate_range_for_buffer(textbuf, range));
    StrView line;
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        if (line.len) {
            try( uiw_strview(out->write_std,&line));
        } else try( uiw_lit__(out->write_std, "\n"));
    }
    return Ok;
}
