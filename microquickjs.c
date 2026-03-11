
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "mquickjs.h"
#include "gd32vf103.h"

// eliminate "FALSE" redeclaration error
#define FALSE 0
#define TRUE  1

#include "readline_tty.h"

#define JS_CLASS_LED (JS_CLASS_USER + 0)
#define JS_CLASS_COUNT (JS_CLASS_USER + 1)

extern uint64_t get_timer_value(void);
extern uint32_t SystemCoreClock;

typedef struct {
    const char *data;
    size_t size;
} resource_t;

#define DECLARE_RESOURCE(name) \
    extern char _binary_##name##_start[]; \
    extern char _binary_##name##_end[]; \
    extern char _binary_##name##_size[]; \
    static const resource_t name = { \
        .data = _binary_##name##_start, \
        .size = (size_t)_binary_##name##_size \
    }

typedef enum {
    red = 0,
    green,
    blue,
} color_t;

typedef struct {
    color_t color;
    void (*cb)(color_t color, int state);
} led_t;

char *led_color_tab[] = {
    "red",
    "green",
    "blue",
};

static void set_led(color_t color, int state);

static led_t leds[] = {
    {red, set_led},
    {green, set_led},
    {blue, set_led},
};

static void set_led(color_t color, int state)
{
    uint32_t gpio, pin;

    switch (color) {
        case red:
            gpio = GPIOC;
            pin = GPIO_PIN_13;
            break;
        case green:
            gpio = GPIOA;
            pin = GPIO_PIN_1;
            break;
        case blue:
            gpio = GPIOA;
            pin = GPIO_PIN_2;
            break;
        default:
            return;
    }

    if (state) {
        gpio_bit_reset(gpio, pin);
    } else {
        gpio_bit_set(gpio, pin);
    }

    return;
}

static JSValue js_led_constructor(JSContext *ctx, JSValue *this_val, int argc,
                                  JSValue *argv)
{
    JSValue obj;

    if (!(argc & FRAME_CF_CTOR))
        return JS_ThrowTypeError(ctx, "must be called with new");
    // clear constructor flag bit
    argc &= ~FRAME_CF_CTOR;

    // accepts only one parameter.
    if (argc != 1)
        return JS_ThrowTypeError(ctx, "requires exactly 1 argument, got %d", argc);

    // the parameter must be of type string
    if (!JS_IsString(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "argument must be a string");

    // the parameter must be one of 'R', 'G' or 'B'
    size_t len;
    JSCStringBuf buf;
    const char *str = JS_ToCString(ctx, argv[0], &buf);
    const char color = str[0];
    if (strlen(str) != 1 || (color != 'R' && color != 'G' && color != 'B'))
        return JS_ThrowTypeError(ctx, "argument must be 'R', 'G', or 'B'");

    obj = JS_NewObjectClassUser(ctx, JS_CLASS_LED);
    switch (color) {
        case 'R':
            JS_SetOpaque(ctx, obj, &leds[red]);
            break;
        case 'G':
            JS_SetOpaque(ctx, obj, &leds[green]);
            break;
        case 'B':
            JS_SetOpaque(ctx, obj, &leds[blue]);
            break;
        default:
            break;
    }

    return obj;
}

static void js_led_finalizer(JSContext *ctx, void *opaque)
{
    return;
}

static JSValue js_led_get_color(JSContext *ctx, JSValue *this_val, int argc,
                                JSValue *argv)
{
    led_t *led;

    int class_id = JS_GetClassID(ctx, *this_val);
    if (class_id != JS_CLASS_LED)
        return JS_ThrowTypeError(ctx, "expectint LED class");

    led = JS_GetOpaque(ctx, *this_val);

    return JS_NewString(ctx, led_color_tab[led->color]);
}

static JSValue js_led_on(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    led_t *led = JS_GetOpaque(ctx, *this_val);
    led->cb(led->color, 1);

    return JS_UNDEFINED;
}

static JSValue js_led_off(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    led_t *led = JS_GetOpaque(ctx, *this_val);
    led->cb(led->color, 0);

    return JS_UNDEFINED;
}

static JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    int i;
    JSValue v;

    for(i = 0; i < argc; i++) {
        if (i != 0)
            putchar(' ');
        v = argv[i];
        if (JS_IsString(ctx, v)) {
            JSCStringBuf buf;
            const char *str;
            size_t len;
            str = JS_ToCStringLen(ctx, &len, v, &buf);
            fwrite(str, 1, len, stdout);
        } else {
            JS_PrintValueF(ctx, argv[i], JS_DUMP_LONG);
        }
    }
    putchar('\n');
    return JS_UNDEFINED;
}

static int64_t get_time_ms(void)
{
    uint64_t tick = get_timer_value();

    // the tick counter frequency is derived from the system clock divided by 4
    return (uint64_t)(tick / (SystemCoreClock / 4000));
}


static JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    JS_GC(ctx);
    return JS_UNDEFINED;
}

static JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_ThrowInternalError(ctx, "Not implemented");
}

static JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_ThrowInternalError(ctx, "Not implemented");
}

static JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_ThrowInternalError(ctx, "Not implemented");
}

#include "js_stdlib.h"

DECLARE_RESOURCE(script_js);

static const uint8_t *load_file(int *plen)
{
    if (plen)
        *plen = (int)script_js.size;

    return script_js.data;
}

/* repl */
#define STYLE_DEFAULT    COLOR_BRIGHT_GREEN
#define STYLE_COMMENT    COLOR_WHITE
#define STYLE_STRING     COLOR_BRIGHT_CYAN
#define STYLE_REGEX      COLOR_CYAN
#define STYLE_NUMBER     COLOR_GREEN
#define STYLE_KEYWORD    COLOR_BRIGHT_WHITE
#define STYLE_FUNCTION   COLOR_BRIGHT_YELLOW
#define STYLE_TYPE       COLOR_BRIGHT_MAGENTA
#define STYLE_IDENTIFIER COLOR_BRIGHT_GREEN
#define STYLE_ERROR      COLOR_RED
#define STYLE_RESULT     COLOR_BRIGHT_WHITE
#define STYLE_ERROR_MSG  COLOR_BRIGHT_RED

static int js_log_err_flag;

static void js_log_func(void *opaque, const void *buf, size_t buf_len)
{
    fwrite(buf, 1, buf_len, js_log_err_flag ? stderr : stdout);
}

static void dump_error(JSContext *ctx)
{
    JSValue obj;
    obj = JS_GetException(ctx);
    fprintf(stderr, "%s", term_colors[STYLE_ERROR_MSG]);
    js_log_err_flag++;
    JS_PrintValueF(ctx, obj, JS_DUMP_LONG);
    js_log_err_flag--;
    fprintf(stderr, "%s\n", term_colors[COLOR_NONE]);
}

static int eval_buf(JSContext *ctx, const char *eval_str, const char *filename, BOOL is_repl, int parse_flags)
{
    JSValue val;
    int flags;

    flags = parse_flags;
    if (is_repl)
        flags |= JS_EVAL_RETVAL | JS_EVAL_REPL;
    val = JS_Parse(ctx, eval_str, strlen(eval_str), filename, flags);
    if (JS_IsException(val))
        goto exception;

    val = JS_Run(ctx, val);
    if (JS_IsException(val)) {
    exception:
        dump_error(ctx);
        return 1;
    } else {
        if (is_repl) {
            printf("%s", term_colors[STYLE_RESULT]);
            JS_PrintValueF(ctx, val, JS_DUMP_LONG);
            printf("%s\r\n", term_colors[COLOR_NONE]);
        }
        return 0;
    }
}

static ReadlineState readline_state;
static uint8_t readline_cmd_buf[256];
static uint8_t readline_kill_buf[256];
static char readline_history[512];

void readline_find_completion(const char *cmdline)
{
    return;
}

static BOOL is_word(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        c == '_' || c == '$';
}

static const char js_keywords[] =
    "break|case|catch|continue|debugger|default|delete|do|"
    "else|finally|for|function|if|in|instanceof|new|"
    "return|switch|this|throw|try|typeof|while|with|"
    "class|const|enum|import|export|extends|super|"
    "implements|interface|let|package|private|protected|"
    "public|static|yield|"
    "undefined|null|true|false|Infinity|NaN|"
    "eval|arguments|"
    "await|";

static const char js_types[] = "void|var|";

static BOOL find_keyword(const char *buf, size_t buf_len, const char *dict)
{
    const char *r, *p = dict;
    while (*p != '\0') {
        r = strchr(p, '|');
        if (!r)
            break;
        if ((r - p) == buf_len && !memcmp(buf, p, buf_len))
            return TRUE;
        p = r + 1;
    }
    return FALSE;
}

/* return the color for the character at position 'pos' and the number
   of characters of the same color */
static int term_get_color(int *plen, const char *buf, int pos, int buf_len)
{
    int c, color, pos1, len;

    c = buf[pos];
    if (c == '"' || c == '\'') {
        pos1 = pos + 1;
        for(;;) {
            if (buf[pos1] == '\0' || buf[pos1] == c)
                break;
            if (buf[pos1] == '\\' && buf[pos1 + 1] != '\0')
                pos1 += 2;
            else
                pos1++;
        }
        if (buf[pos1] != '\0')
            pos1++;
        len = pos1 - pos;
        color = STYLE_STRING;
    } else if (c == '/' && buf[pos + 1] == '*') {
        pos1 = pos + 2;
        while (buf[pos1] != '\0' &&
               !(buf[pos1] == '*' && buf[pos1 + 1] == '/')) {
            pos1++;
        }
        if (buf[pos1] != '\0')
            pos1 += 2;
        len = pos1 - pos;
        color = STYLE_COMMENT;
    } else if ((c >= '0' && c <= '9') || c == '.') {
        pos1 = pos + 1;
        while (is_word(buf[pos1]))
            pos1++;
        len = pos1 - pos;
        color = STYLE_NUMBER;
    } else if (is_word(c)) {
        pos1 = pos + 1;
        while (is_word(buf[pos1]))
            pos1++;
        len = pos1 - pos;
        if (find_keyword(buf + pos, len, js_keywords)) {
            color = STYLE_KEYWORD;
        } else {
            while (buf[pos1] == ' ')
                pos1++;
            if (buf[pos1] == '(') {
                color = STYLE_FUNCTION;
            } else {
                if (find_keyword(buf + pos, len, js_types)) {
                    color = STYLE_TYPE;
                } else {
                    color = STYLE_IDENTIFIER;
                }
            }
        }
    } else {
        color = STYLE_DEFAULT;
        len = 1;
    }
    *plen = len;
    return color;
}

static int js_interrupt_handler(JSContext *ctx, void *opaque)
{
    return readline_is_interrupted();
}

static void repl_run(JSContext *ctx)
{
    ReadlineState *s = &readline_state;
    const char *cmd;

    s->term_width = readline_tty_init();
    s->term_cmd_buf = readline_cmd_buf;
    s->term_kill_buf = readline_kill_buf;
    s->term_cmd_buf_size = sizeof(readline_cmd_buf);
    s->term_history = readline_history;
    s->term_history_buf_size = sizeof(readline_history);
    s->get_color = term_get_color;

    JS_SetInterruptHandler(ctx, js_interrupt_handler);

    for (;;) {
        cmd = readline_tty(&readline_state, "mqjs-rv > ", FALSE);
        if (!cmd)
            break;
        eval_buf(ctx, cmd, "<cmdline>", TRUE, 0);
    }
}

int js_runtime(void)
{
    JSContext *ctx;
    JSValue val;
    const size_t rt_mem_size = 25600;
    uint8_t buff[rt_mem_size];

    ctx = JS_NewContext(buff, rt_mem_size, &js_stdlib);
    JS_SetLogFunc(ctx, js_log_func);

    int script_len;
    const uint8_t *script = load_file(&script_len);
    val = JS_Eval(ctx, (const char *)script, script_len, "script.js", 0);

    if (JS_IsException(val)) {
        JSValue obj = JS_GetException(ctx);
        JS_PrintValueF(ctx, obj, JS_DUMP_LONG);
        printf("\n");
        return 1;
    }

    JS_FreeContext(ctx);
    return 0;
}

void js_run_repl(void)
{
    JSContext *ctx;
    JSValue val;
    const size_t rt_mem_size = 25600;
    uint8_t buff[rt_mem_size];

    ctx = JS_NewContext(buff, rt_mem_size, &js_stdlib);
    JS_SetLogFunc(ctx, js_log_func);

    repl_run(ctx);

    JS_FreeContext(ctx);
}