
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "cutils.h"
#include "mquickjs.h"

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

DECLARE_RESOURCE(script_js);

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

    // tick 计数器频率来源是系统频率的4分频
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

static void js_log_func(void *opaque, const void *buf, size_t buf_len)
{
    fwrite(buf, 1, buf_len, stdout);
}


static const uint8_t *load_file(int *plen)
{
    if (plen)
        *plen = (int)script_js.size;

    return script_js.data;
}

#if 0
static const uint8_t *load_file(int *plen)
{
    static const uint8_t script[] =
        "function hello() {\n"
        "   console.log('Hello from MicroQuickJS')\n"
        "}\n"
        "hello()\n";
    uint8_t len = sizeof(script) - 1;

    if (plen)
        *plen = (int)len;

    return script;
}
#endif

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
