#include "escape-codes.h"
#include "reditline.h"

#define RL_DEFAULT_LINE_CAPACITY 256u

typedef struct {
    char* items;
    size_t capacity;
    size_t len;
    size_t pos;
} RLBuf;

typedef enum {
    ReditlineOk = 0,
    RlErrorRealloc,
    RlErrorMalloc,
    RlErrorFwrite,
    RlErrorStrdup,
    RlErrorArlAppend,
    RlErrorHistoryIndexOutOfRange
} RlError;

#define validate_rl_err(Value) _Generic((Value), RlError: Value)
#define rl_try(Expr) do{\
    RlError rl_err_=validate_rl_err((Expr));if (rl_err_) return rl_err_;}while(0) 

Err switch_tty_to_raw_mode(struct termios prev_termios[static 1]) {

    if (!isatty(STDIN_FILENO)) return "error: not a tty";
    if (tcgetattr(STDIN_FILENO, prev_termios) == -1) return "error: tcgetattr falure";

    struct termios new_termios = *prev_termios;
    new_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    new_termios.c_oflag &= ~(OPOST);
    new_termios.c_cflag |= (CS8);
    new_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO,TCSAFLUSH, &new_termios) < 0) return "error: tcsetattr failure";
    return Ok;
}


static inline void rlbuf_reset(RLBuf b[static 1]) { b->len = b->pos = 0; }
static inline RlError rlbuf_ensure_extra_capacity_(RLBuf b[static 1], size_t size) {
    if (b->capacity < b->len + size) {
        b->capacity += RL_DEFAULT_LINE_CAPACITY;
        b->items = std_realloc(b->items, b->capacity);
        if (!b->items) return RlErrorRealloc;
    }
    return ReditlineOk;
}

static inline RlError rlbuf_append(RLBuf b[static 1], const char* data, size_t datalen) {
    rl_try(rlbuf_ensure_extra_capacity_(b, datalen));
    memmove(b->items + b->len, data, datalen);
    b->len += datalen;
    return ReditlineOk;
}

typedef struct {
    ArlOf(const_cstr)* history;
    size_t             history_ix;
    RLBuf              buf;
} ReditLine;

static inline ArlOf(const_cstr)* rl_history(ReditLine rl[static 1]) { return rl->history; }
static inline RLBuf* rl_buf(ReditLine rl[static 1]) { return &rl->buf; }
static inline size_t* rl_pos(ReditLine rl[static 1]) { return &rl->buf.pos; }
static inline size_t* rl_history_ix(ReditLine rl[static 1]) { return &rl->history_ix; }

static inline const_cstr* rl_history_entry(ReditLine rl[static 1]) {
    size_t actual_ix = len__(rl_history(rl)) - *rl_history_ix(rl);
    return arlfn(const_cstr, at)(rl_history(rl), actual_ix);
}

static inline RlError rl_init(ReditLine rl[static 1], ArlOf(const_cstr) h[static 1], const char* line) {
    *rl = (ReditLine) {.history = h };
    if (!(rl_buf(rl)->items = std_malloc(RL_DEFAULT_LINE_CAPACITY))) return RlErrorMalloc;
    if (line && *line) {
        rl_try (rlbuf_append(rl_buf(rl), line, strlen(line)));
        rl_buf(rl)->pos = rl_buf(rl)->len;
    }
    return ReditlineOk;
}

static inline RlError rl_buf_write(ReditLine rl[static 1]) {
    size_t len = len__(rl_buf(rl)) ;
    if (len && fwrite(items__(rl_buf(rl)), 1, len, stdout) != len) 
            return RlErrorFwrite;
    return ReditlineOk;
}

static inline RlError rl_erase_line(void) {
    if (fwrite(EscCodeEraseLine, 1, lit_len__(EscCodeEraseLine), stdout)
            != lit_len__(EscCodeEraseLine)
    || fwrite(EscCodeUnsaveCursor, 1, lit_len__(EscCodeUnsaveCursor), stdout)
            != lit_len__(EscCodeUnsaveCursor))
        return RlErrorFwrite;
    return ReditlineOk;
}


static inline void rl_cleanup(ReditLine rl[static 1]) {
    //TODO: not cleaning history because it's cached and cleaned on exit;
    std_free(rl_buf(rl)->items);
    *rl_buf(rl) = (RLBuf){0};
}


static RlError rl_insert_char(ReditLine rl[static 1], char c) {
    rl_try(rlbuf_ensure_extra_capacity_(rl_buf(rl), 1));
    if (rl->buf.pos < rl->buf.len) {
        char* dest = rl->buf.items + rl->buf.pos + 1;
        char* src  = rl->buf.items + rl->buf.pos;
        size_t n   = rl->buf.len - ++rl->buf.pos;
        memmove(dest, src, n);
    }
    rl->buf.items[rl->buf.pos] = c;
    ++rl->buf.len;
    ++rl->buf.pos;
    return ReditlineOk;
}

#define REDITLINE_HISTORY_PREV 1
#define REDITLINE_HISTORY_NEXT -1

static RlError _rl_buf_from_history_(ReditLine rl[static 1], int direction) {
    *rl_history_ix(rl) = *rl_history_ix(rl) + direction;
    const_cstr* entry = rl_history_entry(rl); 
    if (!entry) return RlErrorHistoryIndexOutOfRange;
    rlbuf_reset(rl_buf(rl));
    rl_try(rlbuf_append(rl_buf(rl),(char*)*entry, strlen(*entry)));
    *rl_pos(rl) = len__(rl_buf(rl)) - 1;
    return ReditlineOk;
}

static inline RlError rl_history_next(ReditLine rl[static 1]) {
    return len__(rl_history(rl)) && *rl_history_ix(rl) > 1 
        ? _rl_buf_from_history_(rl, REDITLINE_HISTORY_NEXT)
        : ReditlineOk
        ;
}

static inline RlError rl_history_prev(ReditLine rl[static 1]) {
    return len__(rl_history(rl)) > *rl_history_ix(rl)
        ? _rl_buf_from_history_(rl, REDITLINE_HISTORY_PREV)
        : ReditlineOk
        ;
}

static RlError rl_edit(ReditLine rl[static 1]) {
    while (1) {
        int c = fgetc(stdin);
        switch (c) {
            case KeyBackSpace: {
               if (*rl_pos(rl) > 1) {
                   --(*rl_pos(rl)); 
                   --rl_buf(rl)->len;
                   rl_try( rl_erase_line());
                   rl_try(rl_buf_write(rl));
                   continue;
               } else {
                   rl_try( rl_erase_line());
                   rl_cleanup(rl);
                   return ReditlineOk;
               }
            }
            case KeyCtrl_C: 
            case KeyCtrl_D:
                   rl_cleanup(rl);
                   rl_try( rl_erase_line());
                   return ReditlineOk;
            case KeyCtrl_P: 
                   rl_try(rl_history_prev(rl));
                   rl_try(rl_erase_line());
                   rl_try(rl_buf_write(rl));
                   continue;
            case KeyCtrl_N: 
                   rl_try(rl_history_next(rl));
                   rl_try( rl_erase_line());
                   rl_try(rl_buf_write(rl));
                   continue;
            case KeyEnter: return rlbuf_append(rl_buf(rl), "\0", 1);
            default: {
                if (!isprint(c)) continue;
                rl_try(rl_insert_char(rl, c));
                putchar(c);
                continue;
            }
        }
    }
}


int redit_history_add(ArlOf(const_cstr) history[static 1], const char* line) {
    line = std_strdup(line);
    if (!line) return RlErrorStrdup;
    if (!arlfn(const_cstr,append)(history, &line)) {
        std_free((char*)line);
        return RlErrorArlAppend;
    }
    return ReditlineOk;
}

static char* _reditline_error_ = "error: reditline failure";
bool reditline_error(char* res) { return res == _reditline_error_; }

 
char* reditline(const char* prompt, char* line, ArlOf(const_cstr) history[static 1]) {
    fwrite(EscCodeSaveCursor, 1, lit_len__(EscCodeSaveCursor), stdout);
    if (prompt && *prompt) fwrite(prompt, 1, strlen(prompt), stdout);
        
    ReditLine rl;
    if (rl_init(&rl, history, line) != ReditlineOk) return _reditline_error_;
    if (rl_buf_write(&rl) != ReditlineOk) {
        rl_cleanup(&rl);
        return _reditline_error_;
    }

    RlError err = rl_edit(&rl);
    if (!err && len__(rl_buf(&rl))) return items__(rl_buf(&rl)); /* move semantics */

    rl_cleanup(&rl);
    return err ? _reditline_error_ : NULL;
}

//TODO: consider whether exporting or using atexit
void reditline_history_cleanup(ArlOf(const_cstr) history[static 1]) {
    arlfn(const_cstr, clean)(history);
}
