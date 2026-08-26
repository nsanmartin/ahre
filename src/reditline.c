#include "escape-codes.h"
#include "reditline.h"
#include "nongeneric.h"

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
    RlErrorHistoryIndexOutOfRange,
    RlErrorReadingInput,
    RlErrorUnexpectedEmptyLine,
    RlErrorInvalidInput
} RlError;

#define validate_rl_err(Value) _Generic((Value), RlError: Value)
#define rl_try(Expr) do{\
    RlError rl_err_=validate_rl_err((Expr));if (rl_err_) return rl_err_;}while(0) 

Err switch_tty_to_raw_mode(struct termios prev_termios[_1_]) {

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



static inline void rlbuf_reset(RLBuf b[_1_]) { b->len = b->pos = 0; }
static inline RlError rlbuf_ensure_extra_capacity_(RLBuf b[_1_], size_t size) {
    if (b->capacity < b->len + size) {
        const size_t new_capacity = b->capacity + RL_DEFAULT_LINE_CAPACITY;
        char* realloc_res         = std_realloc(b->items, new_capacity);
        if (!realloc_res) {
            std_free(b->items);
            *b = (RLBuf){0};
            return RlErrorRealloc;
        }
        b->capacity = new_capacity;
        b->items    = realloc_res;
    }
    return ReditlineOk;
}

static inline RlError rlbuf_append(RLBuf b[_1_], const char* data, size_t datalen) {
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

static inline ArlOf(const_cstr)* rl_history(ReditLine rl[_1_]) { return rl->history; }
static inline RLBuf* rl_buf(ReditLine rl[_1_]) { return &rl->buf; }
static inline size_t* rl_pos(ReditLine rl[_1_]) { return &rl->buf.pos; }
static inline size_t* rl_history_ix(ReditLine rl[_1_]) { return &rl->history_ix; }

static inline const_cstr* rl_history_entry(ReditLine rl[_1_]) {
    size_t actual_ix = len__(rl_history(rl)) - *rl_history_ix(rl);
    return arlfn(const_cstr, at)(rl_history(rl), actual_ix);
}

static inline RlError rl_init(ReditLine rl[_1_], ArlOf(const_cstr) h[_1_], const char* line) {
    *rl = (ReditLine) {.history = h };
    if (!(rl_buf(rl)->items = std_malloc(RL_DEFAULT_LINE_CAPACITY))) return RlErrorMalloc;
    if (line && *line && isprint(*line)) {
        rl_try (rlbuf_append(rl_buf(rl), line, strlen(line)));
        rl_buf(rl)->pos = rl_buf(rl)->len;
    }
    return ReditlineOk;
}

static inline RlError rl_buf_write(ReditLine rl[_1_]) {
    size_t len = len__(rl_buf(rl)) ;
    if (len && fwrite(items__(rl_buf(rl)), 1, len, stdout) != len) 
            return RlErrorFwrite;
    return ReditlineOk;
}

static inline RlError rl_erase_line(void) {
    if (fwrite(EscCodeEraseLine, 1, lit_len__(EscCodeEraseLine), stdout) != lit_len__(EscCodeEraseLine)
    ||  fwrite(EscCodeUnsaveCursor, 1, lit_len__(EscCodeUnsaveCursor), stdout) != lit_len__(EscCodeUnsaveCursor))
        return RlErrorFwrite;
    return ReditlineOk;
}


static inline void rl_cleanup(ReditLine rl[_1_]) {
    //TODO: not cleaning history because it's cached and cleaned on exit;
    std_free(rl_buf(rl)->items);
    *rl_buf(rl) = (RLBuf){0};
}


/*
 * Cursor movement direction could be:
 * A: up
 * B: down
 * C: forward
 * D: backward
 */
#define EscCodeDirectionUp          'A'
#define EscCodeDirectionDown        'B'
#define EscCodeDirectionForward     'C'
#define EscCodeDirectionBackward    'D'
static RlError
rl_move_cursor(size_t num, char direction) {
    if (direction != EscCodeDirectionUp && direction != EscCodeDirectionDown
    && direction != EscCodeDirectionForward && direction != EscCodeDirectionBackward)
        return RlErrorInvalidInput;
    if (num > 999) return RlErrorInvalidInput;
    char buf[] = { EscCodePrefix };
    int len    = snprintf(buf + 2, 3, "%lu", num);
    if (len < 0 || (len > 3))
        return RlErrorInvalidInput;
    buf[2 + len] = direction;
    fwrite(buf, 1, 2 + len + 1, stdout);
    return ReditlineOk;
}


static RlError
rl_redraw_line(ReditLine rl[1]) {
   rl_try(rl_erase_line());
   rl_try(rl_buf_write(rl));
   const size_t left_movement = rl_buf(rl)->len - rl_buf(rl)->pos;
   if (left_movement)
       rl_try(rl_move_cursor(left_movement, EscCodeDirectionBackward));
   return ReditlineOk;
}




static RlError rl_insert_char(ReditLine rl[_1_], char c) {
    rl_try(rlbuf_ensure_extra_capacity_(rl_buf(rl), 1));

    bool redraw = rl->buf.pos < rl->buf.len;
    if (redraw) {
        char* dest = rl->buf.items + rl->buf.pos + 1;
        char* src  = rl->buf.items + rl->buf.pos;
        size_t n   = rl->buf.len - rl->buf.pos;
        memmove(dest, src, n);
    }
    rl->buf.items[rl->buf.pos] = c;
    ++rl->buf.len;
    ++rl->buf.pos;
    rl->buf.items[rl->buf.len] = '\0';
    if (redraw) {
        rl_try(rl_redraw_line(rl));
        /* const size_t left_movement = rl->buf.len - (rl->buf.pos + 1); */
        /* rl_try(rl_move_cursor(left_movement, EscCodeDirectionBackward)); */
    } else putchar(c);
    return ReditlineOk;
}

/* unsafe method, caller should check `*rl_pos(rl) > 1` */
static char
rl_unsafe_get_pos_char(ReditLine rl[1]) { return rl_buf(rl)->items[*rl_pos(rl)-1]; }

#define REDITLINE_HISTORY_PREV 1
#define REDITLINE_HISTORY_NEXT -1

static RlError _rl_buf_from_history_(ReditLine rl[_1_], int direction) {
    *rl_history_ix(rl) = *rl_history_ix(rl) + direction;
    const_cstr* entry = rl_history_entry(rl); 
    if (!entry) return RlErrorHistoryIndexOutOfRange;
    rlbuf_reset(rl_buf(rl));
    rl_try(rlbuf_append(rl_buf(rl),(char*)*entry, strlen(*entry)));
    *rl_pos(rl) = len__(rl_buf(rl));
    return ReditlineOk;
}

static inline RlError rl_history_next(ReditLine rl[_1_]) {
    return len__(rl_history(rl)) && *rl_history_ix(rl) > 1 
        ? _rl_buf_from_history_(rl, REDITLINE_HISTORY_NEXT)
        : ReditlineOk
        ;
}

static inline RlError rl_history_prev(ReditLine rl[_1_]) {
    return len__(rl_history(rl)) > *rl_history_ix(rl)
        ? _rl_buf_from_history_(rl, REDITLINE_HISTORY_PREV)
        : ReditlineOk
        ;
}


static RlError
rl_write_prev_hist(ReditLine rl[1]) {
   rl_try(rl_history_prev(rl));
   return rl_redraw_line(rl);
}


static RlError
rl_write_next_hist(ReditLine rl[1]) {
   rl_try(rl_history_next(rl));
   return rl_redraw_line(rl);
}


static RlError
rl_erase_and_cleanup(ReditLine rl[1]) {
    rl_try( rl_erase_line());
    rl_cleanup(rl);
    return ReditlineOk;
}


static RlError
rl_delete_char_back(ReditLine rl[1]) {
    const size_t prevpos = rl_buf(rl)->pos;
    const size_t prevlen = rl_buf(rl)->len;
    if (*rl_pos(rl) <= 1) return ReditlineOk;

    --rl_buf(rl)->pos;
    --rl_buf(rl)->len;
    if (rl_buf(rl)->pos < rl_buf(rl)->len)
        memmove(
            rl_buf(rl)->items + rl_buf(rl)->pos,
            rl_buf(rl)->items + prevpos,
            prevlen - prevpos
        );
    rl_try(rl_redraw_line(rl));
    return ReditlineOk;
}


static RlError
rl_delete_char_forward(ReditLine rl[1]) {
    if (rl_buf(rl)->len == rl_buf(rl)->pos) return ReditlineOk;
    --rl_buf(rl)->len;
    const size_t chars_to_move = rl_buf(rl)->len - (rl_buf(rl)->pos - 1);
    memmove(
        rl_buf(rl)->items + rl_buf(rl)->pos,
        rl_buf(rl)->items + rl_buf(rl)->pos + 1,
        chars_to_move
    );
    rl_try(rl_redraw_line(rl));
    return ReditlineOk;
}


static RlError
rl_delete_word_back(ReditLine rl[1]) {
    const size_t prevpos = rl_buf(rl)->pos;
    const size_t prevlen = rl_buf(rl)->len;
    while (*rl_pos(rl) > 1 && isspace(rl_unsafe_get_pos_char(rl))) {
       --rl_buf(rl)->pos;
       --rl_buf(rl)->len;
    }
    while (*rl_pos(rl) > 1 && !isspace(rl_unsafe_get_pos_char(rl))) {
       --rl_buf(rl)->pos;
       --rl_buf(rl)->len;
    }

   if (rl_buf(rl)->pos < rl_buf(rl)->len)
        memmove(
            rl_buf(rl)->items + rl_buf(rl)->pos,
            rl_buf(rl)->items + prevpos,
            prevlen - prevpos
        );

    if (prevpos > rl_buf(rl)->pos) rl_redraw_line(rl);
    return ReditlineOk;
}


static RlError
rl_move_right1(ReditLine rl[1]) {
    if (*rl_pos(rl) < len__(rl_buf(rl))) {
       ++(*rl_pos(rl));
       rl_try(rl_move_cursor(1, EscCodeDirectionForward));
    }
    return ReditlineOk;
}


static RlError
rl_move_beg(ReditLine rl[1]) {
    if (rl_buf(rl)->pos > 1) {
       const size_t left_movement = rl_buf(rl)->pos - 1;
       rl_buf(rl)->pos = 1;
       rl_try(rl_move_cursor(left_movement, EscCodeDirectionBackward));
    }
    return ReditlineOk;
}


static RlError
rl_move_end(ReditLine rl[1]) {
    const size_t right_movement = len__(rl_buf(rl)) - rl_buf(rl)->pos;
    if (right_movement) {
       rl_buf(rl)->pos = len__(rl_buf(rl));
       rl_try(rl_move_cursor(right_movement, EscCodeDirectionForward));
    }
    return ReditlineOk;
}


static RlError
rl_move_left1(ReditLine rl[1]) {
    if (*rl_pos(rl) > 1) {
       --(*rl_pos(rl));
        fwrite(EscCodeBackward1, 1, lit_len__(EscCodeBackward1), stdout);
    }
    return ReditlineOk;
}


static RlError
rl_edit(ReditLine rl[_1_]) {
    while (1) {
        int c = fgetc(stdin);
        switch (c) {
            case KeyCtrl_A: rl_try(rl_move_beg(rl)); break;
            case KeyCtrl_E: rl_try(rl_move_end(rl)); break;
            case KeyCtrl_F: rl_try(rl_move_right1(rl)); break;
            case KeyCtrl_B: rl_try(rl_move_left1(rl)); break;
            case KeyCtrl_W: rl_try(rl_delete_word_back(rl)); break;
            case KeyCtrl_H:
            case KeyBackSpace: {
               if (rl_buf(rl)->len <= 1) return rl_erase_and_cleanup(rl);
               if (rl_buf(rl)->pos <= 1) break;
               rl_try(rl_delete_char_back(rl));
               break;
            }
            case KeyCtrl_D: {
               if (rl_buf(rl)->len == rl_buf(rl)->pos) break;
               rl_try(rl_delete_char_forward(rl));
               break;
            }
            case KeyCtrl_C: return rl_erase_and_cleanup(rl);
            case KeyCtrl_P: rl_try(rl_write_prev_hist(rl)); break;
            case KeyCtrl_N: rl_try(rl_write_next_hist(rl)); break;
            case KeyEnter: return rlbuf_append(rl_buf(rl), "\0", 1);
            case '\033':
                           c = fgetc(stdin);
                           if (c == '[') c = fgetc(stdin);
                           break;
            default: {
                if (!isprint(c)) break;
                rl_try(rl_insert_char(rl, c));
                break;
            }
        }
    }
}


int redit_history_add(ArlOf(const_cstr) history[_1_], const char* line) {
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

 
char*
reditline(const char* prompt, char* line, ArlOf(const_cstr) history[_1_]) {
    fwrite(EscCodeSaveCursor, 1, lit_len__(EscCodeSaveCursor), stdout);
    if (prompt && *prompt) fwrite(prompt, 1, strlen(prompt), stdout);
        
    ReditLine rl;
    if (rl_init(&rl, history, line) != ReditlineOk) return _reditline_error_;
    if (rl_buf_write(&rl) != ReditlineOk) {
        rl_cleanup(&rl);
        return _reditline_error_;
    }

    if (line) {
        switch (*line) {
            case KeyCtrl_P: rl_write_prev_hist(&rl); break;
            default: break;
        }
    }

    RlError err = rl_edit(&rl);
    if (!err && len__(rl_buf(&rl))) return items__(rl_buf(&rl)); /* move semantics */

    rl_cleanup(&rl);
    return err ? _reditline_error_ : NULL;
}

//TODO: consider whether exporting or using atexit
void reditline_history_cleanup(ArlOf(const_cstr) history[_1_]) {
    arlfn(const_cstr, clean)(history);
}
