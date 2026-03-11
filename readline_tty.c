

#include <stdio.h>
#include <stdarg.h>
#include "readline_tty.h"

static int ctrl_c_pressed;
extern char uart_read(void);

ssize_t read(int fd, void *buf, size_t count)
{
    uint8_t ch = uart_read();
    *((uint8_t *)buf) = ch;

    // ctrl + c
    if (ch == 0x03) {
        ctrl_c_pressed = 1;
    }

    // read 1 byte
    return 1;
}

void term_printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void term_flush(void)
{
    fflush(stdout);
}

int readline_tty_init(void)
{
    int n_cols = 80;

    // clear screen
    term_printf("\033[2J\033[H");
    term_printf("mquickjs on risc-v\r\n");
    term_printf("presse Ctrl+C to exit\r\n");
    term_flush();

    return n_cols;
}

const char *readline_tty(ReadlineState *s,
                         const char *prompt, BOOL multi_line)
{
    int len, ctrl_c_count = 0, c, ret;
    const char *ret_str;
    uint8_t buf[128];

    readline_start(s, prompt, FALSE);

    for (;;) {
        len = read(0, buf, sizeof(buf));
        if (len == 0)
            return NULL;

        for (int i = 0; i < len; i++) {
            c = buf[i];
            ret = readline_handle_byte(s, c);
            if (ret == READLINE_RET_EXIT) {
                return NULL;
            } else if (ret == READLINE_RET_ACCEPTED) {
                return (const char *)s->term_cmd_buf;
            }
        }

        if (ctrl_c_pressed) {
            ctrl_c_pressed = 0;
            if (ctrl_c_count == 0) {
                printf("\r\n(Press Ctrl-C again to quit)\r\n");
                ctrl_c_count++;
            } else {
                printf("\r\nExiting.\r\n");
                return NULL;
            }
        }
    }
}

BOOL readline_is_interrupted(void)
{
    BOOL ret;
    ret = (ctrl_c_pressed != 0);
    ctrl_c_pressed = 0;
    return ret;
}

