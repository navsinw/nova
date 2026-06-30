#include "nova.h"
#include <string.h>

/* word-wrap and alignment helpers for on-screen text. byte-oriented;
   every write is clamped to outcap. */

int nova_text_measure(const char *text)
{
    int max = 0, cur = 0;
    for (const char *p = text; *p; p++) {
        if (*p == '\n') { if (cur > max) max = cur; cur = 0; }
        else cur++;
    }
    if (cur > max) max = cur;
    return max;
}

int nova_text_wrap(const char *text, int width, char *out, int outcap)
{
    if (width < 1) width = 1;
    int op = 0, col = 0;
    int last_space = -1;
    int line_start = 0;

    for (const char *p = text; *p; p++) {
        if (op >= outcap - 1) break;
        char ch = *p;
        if (ch == '\n') {
            out[op++] = '\n';
            col = 0; last_space = -1; line_start = op;
            continue;
        }
        out[op++] = ch;
        if (ch == ' ') last_space = op - 1;
        col++;
        if (col >= width) {
            if (last_space >= line_start) {
                out[last_space] = '\n';
                col = op - last_space - 1;
                line_start = last_space + 1;
                last_space = -1;
            } else {
                if (op < outcap - 1) {
                    out[op++] = '\n';
                    col = 0; line_start = op; last_space = -1;
                }
            }
        }
    }
    out[op] = 0;
    return op;
}

void nova_text_align(char *line, int width, int mode)
{
    int len = (int)strlen(line);
    if (len >= width || width <= 0) return;
    int pad = width - len;
    if (mode == 1) {
        /* right: shift content over, prepend spaces */
        if (len + pad < width + 1) {
            memmove(line + pad, line, (size_t)len + 1);
            for (int i = 0; i < pad; i++) line[i] = ' ';
        }
    } else if (mode == 2) {
        /* center */
        int left = pad / 2;
        memmove(line + left, line, (size_t)len + 1);
        for (int i = 0; i < left; i++) line[i] = ' ';
    }
}
