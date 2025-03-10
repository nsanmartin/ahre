#ifndef AHRE_SCREEN_H__
#define AHRE_SCREEN_H__

typedef struct {
    size_t line;
} Screen;

static inline size_t* screen_line(Screen s[static 1]) { return &s->line; }
#endif
