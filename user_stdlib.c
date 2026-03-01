
#include "mquickjs_build.h"

// Prototype properties/methods (instance members)
static const JSPropDef js_led_proto[] = {
    JS_CGETSET_DEF("color", js_led_get_color, NULL),
    JS_CFUNC_DEF("on", 0, js_led_on),
    JS_CFUNC_DEF("off", 0, js_led_off),
    JS_PROP_END,
};

// Class properties/methods (static members)
static const JSPropDef js_led[] = {
    JS_PROP_END,
};

static const JSClassDef js_led_class =
    JS_CLASS_DEF("LED", 1, js_led_constructor, JS_CLASS_LED, js_led, js_led_proto, NULL, js_led_finalizer);

#include "mqjs_stdlib.c"
