#ifndef AHRE_QUICKJS_DISABLED
#include <stdio.h>
#include <quickjs.h>
#include <quickjs-libc.h>


#include "error.h"
#include "session.h"
#include "js-engine.h"
#include "htmldoc.h"
#include "cmd-out.h"
#include "generic.h"


#define err_jse(Msg) err_fmt("error jse: %s  %s\n", Msg, file_line__)

#define js_fn__(FnName) JSValue FnName(JSContext* ctx, JSValueConst this, int argc, JSValueConst *argv)
#define js_get__(FnName) JSValue FnName(JSContext *ctx, JSValueConst this)
#define js_set__(FnName) JSValue FnName(JSContext* ctx, JSValueConst this, JSValueConst val)


void
jse_clean(JsEngine js[_1_])
{
    if (!js->rt) return;
    JS_RunGC(js->rt);
    JS_FreeContext(js->ctx);
    JS_FreeRuntime(js->rt);
    str_clean(jse_consolebuf(js));
    *js = (JsEngine){0};
}


static js_fn__(jse_fn_not_implemented)
{
    (void)this; (void)argc; (void)argv;
    return JS_ThrowPlainError(ctx, "Ahre jse: fn not implemented");
}

static js_get__(jse_get_not_implemented)
{
    (void)this;
    return JS_ThrowPlainError(ctx, "Ahre jse: getter not implemented");
}

static js_set__(jse_set_not_implemented)
{
    (void)this; (void)val;
    return JS_ThrowPlainError(ctx, "Ahre jse: setter not implemented");
}



static Err
init_class(JSClassID class_id[_1_], JSClassDef cdef[_1_], JSContext* ctx)
{
    JSRuntime* rt = JS_GetRuntime(ctx);
    JS_NewClassID(rt, class_id);
    if (JS_NewClass(rt, *class_id, cdef))
        return err_jse("could not initialize class");
    return Ok;
}


static Err
init_instance(JSValue instance[_1_], JSClassID class_id, JSContext* ctx)
{
    *instance = JS_NewObjectClass(ctx, class_id);
    if (JS_IsException(*instance)) return err_jse("could not create the console");
    return Ok;
}


static Err
set_propery_str(JSContext* ctx, JSValueConst this_obj, const char* name, JSValue value)
{
    return -1 == JS_SetPropertyStr(ctx, this_obj, name, value) /* return -1 on exception */
        ? err_jse("could not set propery")
        : Ok
        ;
}


#define set_property_fn_list(Ctx, Obj, Arr) (\
    JS_SetPropertyFunctionList(Ctx, Obj, Arr, sizeof(Arr)/sizeof(*Arr)) == 0 \
    ? Ok : err_jse("could not set property fn list"))

//
//TODO1: https://developer.mozilla.org/en-US/docs/Web/API/Window/setTimeout
static js_fn__(setTimeout)
{
    (void)argc;
    (void)this;
    int64_t delay;
    JSValueConst func;

    func = argv[0];
    if (!JS_IsFunction(ctx, func))
        return JS_ThrowTypeError(ctx, "setTimeout expects a function as first parameter");
    if (JS_ToInt64(ctx, &delay, argv[1]))
        return JS_EXCEPTION;

    JSValue ret = JS_Call(ctx, func, JS_UNDEFINED, 0, NULL);
    JS_FreeValue(ctx, ret);

    return JS_NewInt32(ctx, 1);
}



/* ---- Console ---- */
static JSClassID console_class_id = 0;


static JSValue
console_msg(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv, StrView prefix) {
    JSValue rv  = JS_UNDEFINED;
    Str*    buf = JS_GetOpaque(this, console_class_id);
    if (!buf) return JS_ThrowTypeError(ctx, "js-engine: could not get console");
    if (prefix.len && str_append(buf, prefix)) return JS_ThrowOutOfMemory(ctx);

    for (int i = 0; i < argc; ++i) {
        size_t len;
        const char* arg = JS_ToCStringLen(ctx, &len, argv[i]);
        if (!arg) return JS_ThrowTypeError(ctx, "invalid arg");
        Err e = str_append(buf, sv(arg, len));
        JS_FreeCString(ctx, arg);
        if (e) return JS_ThrowOutOfMemory(ctx);
    }
    if (str_append(buf, svl("\n"))) return JS_ThrowOutOfMemory(ctx);
    return rv;
}


static js_fn__(console_log) { return console_msg(ctx, this, argc, argv, svl("js console log: ")); }
static js_fn__(console_warn) { return console_msg(ctx, this, argc, argv, svl("js console warn: ")); }
static js_fn__(console_error) { return console_msg(ctx, this, argc, argv, svl("js console error: ")); }

static const JSCFunctionListEntry console_proto_fn_list[] = {
    JS_CFUNC_DEF("log",   1, console_log),
    JS_CFUNC_DEF("warn",  1, console_warn),
    JS_CFUNC_DEF("error", 1, console_error)
};


static Err
console_init(JSValue console[_1_], JSContext* ctx, HtmlDoc d[_1_]) {
    try(init_class(&console_class_id, &(JSClassDef){ "Console", .finalizer=NULL }, ctx));
    try(init_instance(console, console_class_id, ctx));

    Str* buf = jse_consolebuf(htmldoc_js(d));
    if (JS_SetOpaque(*console, buf)) err_jse("could not set the console buffer");
    try(set_property_fn_list(ctx, *console, console_proto_fn_list));
    return Ok;
}

static Err
global_add_console(JSValue global, HtmlDoc d[_1_], JSContext* ctx) {
    JSValue console   = JS_UNDEFINED;
    try( console_init(&console, ctx, d));
    try( set_propery_str(ctx, global, "console", console));
    return Ok;
}
/** console **/

/* ---- Element ---- */
static JSClassID element_class_id = 0;


static JSValue
element_get_textContent(JSContext* ctx, JSValueConst this_val)
{
    //TODO0
    (void)ctx;
    (void)this_val;
    return JS_NewString(ctx, "Foo");
}


static JSValue
element_set_textContent(JSContext* ctx, JSValueConst this_val, JSValueConst val)
{
    DomElem elem = dom_elem_from_ptr(JS_GetOpaque(this_val, element_class_id));
    if (isnull(elem)) return JS_ThrowTypeError(ctx, "ah re");

    size_t len;
    const char* new_content = JS_ToCStringLen(ctx, &len, val);

    Err e = dom_elem_set_text_content(elem, sv(new_content, len));
    if (e) return JS_ThrowTypeError(ctx, e);
    return JS_UNDEFINED;
}

static JSValue
element_getAttribute(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
{
    if (argc != 1) return JS_ThrowTypeError(ctx, "Expected 1 argument");

    DomElem elem = dom_elem_from_ptr(JS_GetOpaque(self, element_class_id));
    if (isnull(elem)) return JS_ThrowTypeError(ctx, "ah re: invalid attribute");

    size_t attrlen;
    const char* attr = JS_ToCStringLen(ctx, &attrlen, argv[0]);
    if (!attr) return JS_ThrowTypeError(ctx, "invalid attribute");

    StrView value = dom_elem_attr_value(elem, sv(attr, attrlen));
    if (!value.len || !value.items) return JS_ThrowTypeError(ctx, "invalid attribute");

    return JS_NewStringLen(ctx, (char*)value.items, value.len);
}


static DomElem
js_value_to_dom_elem(JSContext* ctx, JSValue jsv)
{
    //TODO0
    (void)ctx;
    (void)jsv;
    return (DomElem){0};
}


static JSValue
element_setAttribute(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
{
    if (argc != 2) return JS_ThrowTypeError(ctx, "setAttribute: name and value needed");
    DomElem elem = js_value_to_dom_elem(ctx, self);
    if (isnull(elem)) return JS_ThrowTypeError(ctx, "invalid element");
    size_t name_len, val_len;
    const char *name = JS_ToCStringLen(ctx, &name_len, argv[0]);
    const char *val  = JS_ToCStringLen(ctx, &val_len, argv[1]);
    if (!name || !val) {
        JS_FreeCString(ctx, name);
        JS_FreeCString(ctx, val);
        return JS_EXCEPTION;
    }
    Err e = dom_elem_set_attr(elem, sv(name, name_len), sv(val, val_len));
    JS_FreeCString(ctx, name);
    JS_FreeCString(ctx, val);
    if (e) return JS_ThrowTypeError(ctx, e);
    return JS_UNDEFINED;
}

static JSValue
element_removeAttribute(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
{
    if (argc != 1) return JS_ThrowTypeError(ctx, "removeAttribute: name needed");
    DomElem elem = js_value_to_dom_elem(ctx, self);
    if (isnull(elem)) return JS_ThrowTypeError(ctx, "invalid element");
    size_t len;
    const char *name = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!name) return JS_EXCEPTION;
    dom_node_remove_attr(dom_node_from_elem(elem), sv(name, len));
    JS_FreeCString(ctx, name);
    return JS_UNDEFINED;
}


static JSValue
element_remove(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    DomElem elem = js_value_to_dom_elem(ctx, self);
    if (isnull(elem)) return JS_ThrowTypeError(ctx, "invalid element");
    lxb_dom_node_destroy(dom_node_from_elem(elem).ptr);
    return JS_UNDEFINED;
}




/* ClassList */
//TODO0: none of class list related code was tested
static JSClassID classList_class_id = 0;

typedef struct {
    DomElem elem;
    JSContext *ctx;
} ClassListData;

static void
classList_finalizer(JSRuntime *rt, JSValue val)
{
    ClassListData *data = JS_GetOpaque(val, classList_class_id);
    if (data) js_free_rt(rt, data);
}


static JSValue
element_classList_add(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 1) return JS_ThrowTypeError(ctx, "add: token needed");
    ClassListData *data = JS_GetOpaque(this_val, classList_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid classList");
    DomElem elem = data->elem;
    size_t len;
    const char *token = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!token) return JS_EXCEPTION;
    StrView current = dom_elem_attr_value(elem, svl("class"));
    // Naive append (Google only uses add/remove for simple tokens)
    Str buf = {0};
    if (current.len > 0) {
        str_append(&buf, current);
        str_append_z(&buf, svl(" "));
    }
    str_append(&buf, sv(token, len));
    dom_elem_set_attr(elem, svl("class"), sv(&buf));
    str_clean(&buf);
    JS_FreeCString(ctx, token);
    return JS_UNDEFINED;
}


static JSValue
element_classList_remove(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 1) return JS_ThrowTypeError(ctx, "remove: token needed");
    ClassListData *data = JS_GetOpaque(this_val, classList_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid classList");
    DomElem elem = data->elem;
    size_t len;
    const char *token = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!token) return JS_EXCEPTION;
    StrView current = dom_elem_attr_value(elem, svl("class"));
    // Remove token (simple replace)
    Str buf = {0};
    // If current equals token, clear; otherwise keep as is
    if (!str_eq_case(current, sv(token, len)))
        str_append(&buf, current);
    if (buf.len == 0)
        dom_node_remove_attr(dom_node_from_elem(elem), svl("class"));
    else
        dom_elem_set_attr(elem, svl("class"), sv(&buf));
    str_clean(&buf);
    JS_FreeCString(ctx, token);
    return JS_UNDEFINED;
}

static JSValue
element_classList_contains(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 1) return JS_ThrowTypeError(ctx, "contains: token needed");
    ClassListData *data = JS_GetOpaque(this_val, classList_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid classList");
    /* DomElem elem = data->elem; */
    size_t len;
    const char *token = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!token) return JS_EXCEPTION;
    /* StrView current = dom_elem_attr_value(elem, svl("class")); */
    bool has = false;//str_contains(current, sv(token, len));
                     //TODO0!
    JS_FreeCString(ctx, token);
    return JS_NewBool(ctx, has);
}


static JSValue classList_create(JSContext *ctx, DomElem elem)
{
    if (classList_class_id == 0) {
        JS_NewClassID(JS_GetRuntime(ctx), &classList_class_id);
        JSClassDef def = { .class_name = "DOMTokenList", .finalizer = classList_finalizer };
        JS_NewClass(JS_GetRuntime(ctx), classList_class_id, &def);
    }
    JSValue obj = JS_NewObjectClass(ctx, classList_class_id);
    ClassListData *data = js_malloc(ctx, sizeof(ClassListData));
    if (!data) return JS_EXCEPTION;
    data->elem = elem;
    data->ctx = ctx;
    JS_SetOpaque(obj, data);

    JS_SetPropertyStr(ctx, obj, "add", JS_NewCFunction(ctx, element_classList_add, "add", 1));
    JS_SetPropertyStr(ctx, obj, "remove", JS_NewCFunction(ctx, element_classList_remove, "remove", 1));
    JS_SetPropertyStr(ctx, obj, "contains", JS_NewCFunction(ctx, element_classList_contains, "contains", 1));
    return obj;
}


static JSValue
element_get_classList(JSContext *ctx, JSValueConst this_val) {
    DomElem elem = js_value_to_dom_elem(ctx, this_val);
    if (isnull(elem)) return JS_ThrowTypeError(ctx, "invalid element");
    return classList_create(ctx, elem);
}


static JSValue
element_js_value_from_dom_elem(JSContext *ctx, DomElem elem)
{
    JSValue js_element = JS_NewObjectClass(ctx, element_class_id);

    Str* buf = &(Str){0};
    DomAttr attr = dom_elem_first_attr(elem);

    while (!isnull(attr)) {
        if (!dom_attr_has_owner(attr))
            return JS_ThrowTypeError(ctx, "internal error atribute is malformed");

        StrView name  = dom_attr_name_view(attr);
        StrView value = dom_attr_value_view(attr);
        
        str_reset(buf);

        if (str_append_z(buf, &name)) {
            str_clean(buf);
            return JS_ThrowTypeError(ctx, "str append failure");
        }

        if (value.len)
            JS_SetPropertyStr(
                ctx, js_element, items__(buf), JS_NewStringLen(ctx, (char*)value.items, value.len)
            );

        attr = dom_attr_next(attr);
    }
    str_clean(buf);
    if (JS_SetOpaque(js_element, elem.ptr))
        return JS_ThrowTypeError(ctx, "ahrerr: could not set opaque val");

   DomElem e = dom_elem_from_ptr(JS_GetOpaque(js_element, element_class_id));
   if (isnull(e)) return JS_ThrowTypeError(ctx, "ahrerr: invalid attribute");

    return js_element;
}


static const JSCFunctionListEntry element_fn_list[] = {
    JS_CFUNC_DEF("getAttribute", 1, element_getAttribute),
    JS_CFUNC_DEF("remove", 0, element_remove),
    JS_CFUNC_DEF("removeAttribute", 1, element_removeAttribute),
    JS_CFUNC_DEF("setAttribute", 2, element_setAttribute),
    JS_CGETSET_DEF("classList", element_get_classList, NULL),
    JS_CGETSET_DEF("textContent", element_get_textContent, element_set_textContent),
};


static Err
element_init(JSContext* ctx) {
    try(init_class(&element_class_id, &(JSClassDef){ "Element", .finalizer=NULL }, ctx));
    JSValue element_proto = JS_NewObject(ctx);
    try(set_property_fn_list(ctx, element_proto, element_fn_list));
    JS_SetClassProto(ctx, element_class_id, element_proto);
    return Ok;
}


/** element **/


/* Document */
static JSClassID document_class_id = 0;

static js_fn__(document_createElement)
{
    if (argc != 1) return JS_ThrowTypeError(ctx, "createElement: tagName needed");
    Dom dom = dom_from_ptr(JS_GetOpaque(this, document_class_id));
    if (isnull(dom)) return JS_ThrowTypeError(ctx, "invalid document");
    size_t len;
    const char *tag = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!tag) return JS_EXCEPTION;
    DomElem elem;
    Err e = dom_elem_init(&elem, dom, sv(tag, len));
    JS_FreeCString(ctx, tag);
    if (e) return JS_ThrowTypeError(ctx, e);
    return element_js_value_from_dom_elem(ctx, elem);
}


static JSValue
document_getElementById(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) {

    if (argc != 1) return JS_ThrowTypeError(ctx, "Expected 1 argument");

    Dom dom = htmldoc_dom(JS_GetOpaque(self, document_class_id));

    if (isnull(dom)) return JS_ThrowTypeError(ctx, "error: document not properly initialized");

    size_t idlen;
    const char *id = JS_ToCStringLen(ctx, &idlen, argv[0]);
    if (!id) return JS_ThrowTypeError(ctx, "Invalid ID");

    DomElem element = dom_get_elem_by_id(dom, sv(id, idlen));
    JS_FreeCString(ctx, id);
    if (isnull(element)) return JS_NULL;

    return element_js_value_from_dom_elem(ctx, element);
}

static js_get__(document_head)
{
    HtmlDoc* d = JS_GetOpaque(this, document_class_id);
    DomElem head;
    Err e = dom_get_head_elem(htmldoc_dom(d), &head);
    if (e) return JS_ThrowPlainError(ctx, e);
    if (isnull(head)) return JS_ThrowTypeError(ctx, "dom not properly initialized");
    JSValue head_js = element_js_value_from_dom_elem(ctx, head);
    return head_js;
}


static js_get__(document_body)
{
    HtmlDoc* d = JS_GetOpaque(this, document_class_id);
    DomElem body;
    Err e = dom_get_body_elem(htmldoc_dom(d), &body);
    if (e) return JS_ThrowPlainError(ctx, e);
    if (isnull(body)) return JS_ThrowTypeError(ctx, "dom not properly initialized");
    JSValue body_js = element_js_value_from_dom_elem(ctx, body);
    return body_js;
}


static js_get__(document_get_title)
{
    HtmlDoc* d = JS_GetOpaque(this, document_class_id);
    Str title = (Str){0};
    Err e = dom_get_title_text_line(htmldoc_dom(d), &title);
    if (e) goto Clean;
    JSValue rv = JS_NewString(ctx, title.items);
Clean:
    str_clean(&title);
    if (JS_IsException(rv)) return rv;
    if (e) return JS_ThrowPlainError(ctx, e);
    return rv;
}


static js_set__(document_set_title)
{
    HtmlDoc* d = JS_GetOpaque(this, document_class_id);
    DomElem title;
    Err e = dom_get_title_elem(htmldoc_dom(d), &title);
    if (e) return JS_ThrowPlainError(ctx, e);

    size_t len;
    const char* new_content = JS_ToCStringLen(ctx, &len, val);

    e = dom_elem_set_text_content(title, sv(new_content, len));
    if (e) return JS_ThrowTypeError(ctx, e);
    return JS_UNDEFINED;
}



static const JSCFunctionListEntry document_fn_list[] = {
    JS_CGETSET_DEF("activeElement",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("activeViewTransition",        jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("adoptedStyleSheets",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("body",                        document_body,           jse_set_not_implemented),
    JS_CGETSET_DEF("characterSet",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("childElementCount",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("children",                    jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("compatMode",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("contentType",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("currentScript",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("customElementRegistry",       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("doctype",                     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("documentElement",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("documentURI",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("embeds",                      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("featurePolicy",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("firstElementChild",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("fonts",                       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("forms",                       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("fragmentDirective",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("fullscreenElement",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("head",                        document_head,           jse_set_not_implemented),
    JS_CGETSET_DEF("hidden",                      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("images",                      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("implementation",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("lastElementChild",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("links",                       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("pictureInPictureElement",     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("pictureInPictureEnabled",     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("plugins",                     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("pointerLockElement",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("prerendering",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scripts",                     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollingElement",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("styleSheets",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("timeline",                    jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("visibilityState",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("cookie",                      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("defaultView",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("designMode",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("dir",                         jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("fullscreenEnabled",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("lastModified",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("location",                    jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("readyState",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("referrer",                    jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("title",                       document_get_title,      document_set_title),
    JS_CGETSET_DEF("URL",                         jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("alinkColor",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("all",                         jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("anchors",                     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("applets",                     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("bgColor",                     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("domain",                      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("fgColor",                     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("fullscreen",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("lastStyleSheetSet",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("linkColor",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("preferredStyleSheetSet",      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("rootElement",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("selectedStyleSheetSet",       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("styleSheetSets",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("vlinkColor",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("xmlEncoding",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("xmlStandalone",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("xmlVersion",                  jse_get_not_implemented, jse_set_not_implemented),

    JS_CFUNC_DEF("adoptNode",                     0, jse_fn_not_implemented),
    JS_CFUNC_DEF("append",                        0, jse_fn_not_implemented),
    JS_CFUNC_DEF("ariaNotify",                    0, jse_fn_not_implemented),
    JS_CFUNC_DEF("browsingTopics",                0, jse_fn_not_implemented),
    JS_CFUNC_DEF("captureEvents",                 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("caretPositionFromPoint",        0, jse_fn_not_implemented),
    JS_CFUNC_DEF("caretRangeFromPoint",           0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createAttribute",               0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createAttributeNS",             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createCDATASection",            0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createComment",                 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createDocumentFragment",        0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createElement",                 1, document_createElement),
    JS_CFUNC_DEF("createElementNS",               0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createEvent",                   0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createNodeIterator",            0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createProcessingInstruction",   0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createRange",                   0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createTextNode",                1, jse_fn_not_implemented),
    JS_CFUNC_DEF("createTouch",                   0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createTouchList",               0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createTreeWalker",              0, jse_fn_not_implemented),
    JS_CFUNC_DEF("elementFromPoint",              0, jse_fn_not_implemented),
    JS_CFUNC_DEF("elementsFromPoint",             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("enableStyleSheetsForSet",       0, jse_fn_not_implemented),
    JS_CFUNC_DEF("exitFullscreen",                0, jse_fn_not_implemented),
    JS_CFUNC_DEF("exitPictureInPicture",          0, jse_fn_not_implemented),
    JS_CFUNC_DEF("exitPointerLock",               0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getAnimations",                 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getBoxQuads",                   0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getElementById",                1, document_getElementById),
    JS_CFUNC_DEF("getElementsByClassName",        0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getElementsByTagName",          0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getElementsByTagNameNS",        0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getSelection",                  0, jse_fn_not_implemented),
    JS_CFUNC_DEF("hasPrivateToken",               0, jse_fn_not_implemented),
    JS_CFUNC_DEF("hasRedemptionRecord",           0, jse_fn_not_implemented),
    JS_CFUNC_DEF("hasStorageAccess",              0, jse_fn_not_implemented),
    JS_CFUNC_DEF("hasUnpartitionedCookieAccess",  0, jse_fn_not_implemented),
    JS_CFUNC_DEF("importNode",                    0, jse_fn_not_implemented),
    JS_CFUNC_DEF("moveBefore",                    0, jse_fn_not_implemented),
    JS_CFUNC_DEF("mozSetImageElement",            0, jse_fn_not_implemented),
    JS_CFUNC_DEF("prepend",                       0, jse_fn_not_implemented),
    JS_CFUNC_DEF("querySelector",                 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("querySelectorAll",              0, jse_fn_not_implemented),
    JS_CFUNC_DEF("releaseCapture",                0, jse_fn_not_implemented),
    JS_CFUNC_DEF("releaseEvents",                 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("replaceChildren",               0, jse_fn_not_implemented),
    JS_CFUNC_DEF("requestStorageAccess",          0, jse_fn_not_implemented),
    JS_CFUNC_DEF("requestStorageAccessFor",       0, jse_fn_not_implemented),
    JS_CFUNC_DEF("startViewTransition",           0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createExpression",              0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createNSResolver",              0, jse_fn_not_implemented),
    JS_CFUNC_DEF("evaluate",                      0, jse_fn_not_implemented),
    JS_CFUNC_DEF("clear",                         0, jse_fn_not_implemented),
    JS_CFUNC_DEF("close",                         0, jse_fn_not_implemented),
    JS_CFUNC_DEF("execCommand",                   0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getElementsByName",             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("hasFocus",                      0, jse_fn_not_implemented),
    JS_CFUNC_DEF("open",                          0, jse_fn_not_implemented),
    JS_CFUNC_DEF("queryCommandEnabled",           0, jse_fn_not_implemented),
    JS_CFUNC_DEF("queryCommandIndeterm",          0, jse_fn_not_implemented),
    JS_CFUNC_DEF("queryCommandState",             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("queryCommandSupported",         0, jse_fn_not_implemented),
    JS_CFUNC_DEF("queryCommandValue",             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("write",                         0, jse_fn_not_implemented),
    JS_CFUNC_DEF("writeln",                       0, jse_fn_not_implemented),
};


static Err
document_init(JSValue document[_1_], HtmlDoc d[_1_], JSContext* ctx)
{
    try(init_class(&document_class_id, &(JSClassDef){ "document", .finalizer=NULL }, ctx));
    JSValue document_proto = JS_NewObject(ctx);
    try(set_property_fn_list(ctx, document_proto, document_fn_list));
    JS_SetClassProto(ctx, document_class_id, document_proto);

    try(init_instance(document, document_class_id, ctx));

    if (JS_SetOpaque(*document, d)) return err_jse("could not set document opaque");
    
    return Ok;
}


static Err
global_add_document(JSValue global, HtmlDoc d[_1_], JSContext* ctx) {
    JSValue document  = JS_UNDEFINED;
    try( document_init(&document, d, ctx));
    try( set_propery_str(ctx, global, "document", document));
    return Ok;
}

/** document */


/* ---- Location ---- */
static JSClassID location_class_id = 0;
/* static char *current_loc = NULL; */

static const JSCFunctionListEntry location_fn_list[] = {
    JS_CGETSET_DEF("href",     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("protocol", jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("host",     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("hostname", jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("port",     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("pathname", jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("search",   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("hash",     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("origin",   jse_get_not_implemented, jse_set_not_implemented),
    JS_CFUNC_DEF("assign",   1, jse_fn_not_implemented),
    JS_CFUNC_DEF("reload",   0, jse_fn_not_implemented),
    JS_CFUNC_DEF("replace",  1, jse_fn_not_implemented),
    JS_CFUNC_DEF("toString", 0, jse_fn_not_implemented)
};

static Err
location_init(JSValue location[_1_], JSContext* ctx, HtmlDoc d[_1_]) {

    try(init_class(&location_class_id, &(JSClassDef){ "location", .finalizer=NULL }, ctx));
    try(init_instance(location, location_class_id, ctx));

    if (JS_SetOpaque(*location, d)) err_jse("could not set the location buffer");
    try(set_property_fn_list(ctx, *location, location_fn_list));
    return Ok;
}

static Err
global_add_location(JSValue global, HtmlDoc d[_1_], JSContext* ctx) {
    JSValue location  = JS_UNDEFINED;
    try( location_init(&location, ctx, d));
    try( set_propery_str(ctx, global, "location", location));
    return Ok;
}


/* static void location_finalizer(JSRuntime *rt, JSValue val) */
/* { */
/*     char **ptr = JS_GetOpaque(val, location_class_id); */
/*     if (ptr && *ptr) js_free_rt(rt, *ptr); */
/*     js_free_rt(rt, ptr); */
/* } */

/* static JSValue location_get_href(JSContext *ctx, JSValueConst this_val) */
/* { */
/*     char **data = JS_GetOpaque(this_val, location_class_id); */
/*     return JS_NewString(ctx, data && *data ? *data : "https://www.google.com/"); */
/* } */

/* static JSValue location_get_search(JSContext *ctx, JSValueConst this_val) */
/* { */
/*     char **data = JS_GetOpaque(this_val, location_class_id); */
/*     const char *url = data && *data ? *data : ""; */
/*     const char *q = strchr(url, '?'); */
/*     return JS_NewString(ctx, q ? q : ""); */
/* } */

/* static JSValue location_replace(JSContext *ctx, JSValueConst this_val, */
/*                                 int argc, JSValueConst *argv) */
/* { */
/*     if (argc != 1) return JS_ThrowTypeError(ctx, "replace: url needed"); */
/*     size_t len; */
/*     const char *new_url = JS_ToCStringLen(ctx, &len, argv[0]); */
/*     if (!new_url) return JS_EXCEPTION; */
/*     char **data = JS_GetOpaque(this_val, location_class_id); */
/*     if (data && *data) js_free(ctx, *data); */
/*     if (data) *data = js_strdup(ctx, new_url); */
/*     JS_FreeCString(ctx, new_url); */
/*     return JS_UNDEFINED; */
/* } */

/* static JSValue create_location_object(JSContext *ctx) */
/* { */
/*     if (location_class_id == 0) { */
/*         JS_NewClassID(JS_GetRuntime(ctx), &location_class_id); */
/*         JSClassDef def = { .class_name = "Location", .finalizer = location_finalizer }; */
/*         JS_NewClass(JS_GetRuntime(ctx), location_class_id, &def); */
/*     } */
/*     JSValue obj = JS_NewObjectClass(ctx, location_class_id); */
/*     char **data = js_malloc(ctx, sizeof(char*)); */
/*     *data = js_strdup(ctx, "https://www.google.com/"); */
/*     JS_SetOpaque(obj, data); */
/*     JS_SetPropertyStr(ctx, obj, "href", JS_NewCFunction(ctx, location_get_href, "get_href", 0)); */
/*     JS_SetPropertyStr(ctx, obj, "search", JS_NewCFunction(ctx, location_get_search, "get_search", 0)); */
/*     JS_SetPropertyStr(ctx, obj, "replace", JS_NewCFunction(ctx, location_replace, "replace", 1)); */
/*     return obj; */
/* } */

// -------- Performance stub --------
/* static JSValue performance_now(JSContext *ctx, JSValueConst this_val, */
/*                                int argc, JSValueConst *argv) */
/* { */
/*     (void)this_val; (void)argc; (void)argv; */
/*     double ms = (double)clock() / CLOCKS_PER_SEC * 1000.0; */
/*     return JS_NewFloat64(ctx, ms); */
/* } */

/* static JSValue create_performance_object(JSContext *ctx) */
/* { */
/*     JSValue perf = JS_NewObject(ctx); */
/*     JS_SetPropertyStr(ctx, perf, "now", JS_NewCFunction(ctx, performance_now, "now", 0)); */
/*     JS_SetPropertyStr(ctx, perf, "timing", JS_NewObject(ctx)); */
/*     return perf; */
/* } */


/* ---- SessionStorage ---- */

/* static JSValue sessionStorage_getItem(JSContext *ctx, JSValueConst this_val, */
/*                                       int argc, JSValueConst *argv) */
/* { */
/*     (void)this_val; */
/*     (void)argv; */
/*     if (argc != 1) return JS_ThrowTypeError(ctx, "getItem: key needed"); */
/*     // Always return null */
/*     return JS_NULL; */
/* } */

/* static JSValue sessionStorage_setItem(JSContext *ctx, JSValueConst this_val, */
/*                                       int argc, JSValueConst *argv) */
/* { */
/*     (void)this_val; */
/*     (void)argv; */
/*     if (argc != 2) return JS_ThrowTypeError(ctx, "setItem: key and value needed"); */
/*     return JS_UNDEFINED; */
/* } */

/* static JSValue create_sessionStorage_object(JSContext *ctx) */
/* { */
/*     JSValue ss = JS_NewObject(ctx); */
/*     JS_SetPropertyStr(ctx, ss, "getItem", JS_NewCFunction(ctx, sessionStorage_getItem, "getItem", 1)); */
/*     JS_SetPropertyStr(ctx, ss, "setItem", JS_NewCFunction(ctx, sessionStorage_setItem, "setItem", 2)); */
/*     return ss; */
/* } */

/* ---- Navigator ---- */
static JSClassID navigator_class_id = 0;
static const JSCFunctionListEntry navigator_fn_list[] = {
    JS_CFUNC_DEF("canShare",                    0, jse_fn_not_implemented),
    JS_CFUNC_DEF("clearAppBadge",               0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getAutoplayPolicy",           0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getBattery",                  0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getGamepads",                 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getInstalledRelatedApps",     0, jse_fn_not_implemented),
    JS_CFUNC_DEF("registerProtocolHandler",     0, jse_fn_not_implemented),
    JS_CFUNC_DEF("requestMIDIAccess",           0, jse_fn_not_implemented),
    JS_CFUNC_DEF("requestMediaKeySystemAccess", 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("sendBeacon",                  0, jse_fn_not_implemented),
    JS_CFUNC_DEF("setAppBadge",                 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("share",                       0, jse_fn_not_implemented),
    JS_CFUNC_DEF("unregisterProtocolHandler",   0, jse_fn_not_implemented),
    JS_CFUNC_DEF("vibrate",                     0, jse_fn_not_implemented),
    JS_CGETSET_DEF("activeVRDisplays",      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("appCodeName",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("appName",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("appVersion",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("audioSession",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("bluetooth",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("buildID",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("clipboard",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("connection",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("contacts",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("cookieEnabled",         jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("credentials",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("deviceMemory",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("devicePosture",         jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("doNotTrack",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("geolocation",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("globalPrivacyControl",  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("gpu",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("hardwareConcurrency",   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("hid",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ink",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("keyboard",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("language",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("languages",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("locks",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("login",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("maxTouchPoints",        jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("mediaCapabilities",     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("mediaDevices",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("mediaSession",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("mimeTypes",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("onLine",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("oscpu",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("pdfViewerEnabled",      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("permissions",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("platform",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("plugins",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("preferences",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("presentation",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("product",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("productSub",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scheduling",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("serial",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("serviceWorker",         jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("storage",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("usb",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("userActivation",        jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("userAgent",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("userAgentData",         jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("vendor",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("vendorSub",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("virtualKeyboard",       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("wakeLock",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("webdriver",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("windowControlsOverlay", jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("xr",                    jse_get_not_implemented, jse_set_not_implemented),
};

static Err
global_add_navigator(JSValue global, HtmlDoc d[_1_], JSContext* ctx) {
    (void)d;

    JSValue navigator   = JS_UNDEFINED;
    try(init_class(&navigator_class_id, &(JSClassDef){"navigator",.finalizer=NULL}, ctx));
    JSValue navigator_proto = JS_NewObject(ctx);
    if (JS_IsException(navigator_proto)) return err_jse("new object failed");
    try(set_property_fn_list(ctx, navigator_proto, navigator_fn_list));
    JS_SetClassProto(ctx, navigator_class_id, navigator_proto);
    try( set_propery_str(ctx, global, "navigator", navigator));
    return Ok;
}
//
/* static JSValue navigator_sendBeacon(JSContext *ctx, JSValueConst this_val, */
/*                                     int argc, JSValueConst *argv) */
/* { */
/*     (void)this_val; (void)argc; (void)argv;(void)ctx; */
/*     return JS_TRUE; // pretend success */
/* } */

/* static JSValue create_navigator_object(JSContext *ctx) */
/* { */
/*     JSValue nav = JS_NewObject(ctx); */
/*     JS_SetPropertyStr(ctx, nav, "sendBeacon", JS_NewCFunction(ctx, navigator_sendBeacon, "sendBeacon", 2)); */
/*     return nav; */
/* } */

/* // -------- Event listener stubs -------- */
/* static JSValue window_addEventListener(JSContext *ctx, JSValueConst this_val, */
/*                                        int argc, JSValueConst *argv) */
/* { */
/*     (void)ctx; (void)this_val; (void)argc; (void)argv; */
/*     return JS_UNDEFINED; */
/* } */

/* static JSValue window_removeEventListener(JSContext *ctx, JSValueConst this_val, */
/*                                           int argc, JSValueConst *argv) */
/* { */
/*     (void)ctx; (void)this_val; (void)argc; (void)argv; */
/*     return JS_UNDEFINED; */
/* } */

/* ---- Window ---- */
static const JSCFunctionListEntry window_fn_list[] = {
    JS_CFUNC_DEF("addEventListener", 2, jse_fn_not_implemented),
};


static Err
global_add_window(JSValue global, JSContext* ctx) {
    JSValue window = JS_DupValue(ctx, global);
    try(set_propery_str(ctx, global, "window", window));
    try(set_property_fn_list(ctx, window, window_fn_list));
    return Ok;
}

static const JSCFunctionListEntry global_fn_list[] = {
    JS_CFUNC_DEF("setTimeout", 2, setTimeout),
};


/* ---- Storage ---- */
static JSClassID storage_class_id = 0;

static const JSCFunctionListEntry storage_fn_list[] = {
    JS_CFUNC_DEF("getItem",    1, jse_fn_not_implemented),
    JS_CFUNC_DEF("setItem",    2, jse_fn_not_implemented),
    JS_CFUNC_DEF("removeItem", 1, jse_fn_not_implemented),
    JS_CFUNC_DEF("clear",      0, jse_fn_not_implemented),
    JS_CFUNC_DEF("key",        1, jse_fn_not_implemented),
    JS_CGETSET_DEF("length", jse_get_not_implemented, jse_set_not_implemented),
};


static Err
global_add_storages(JSValue global, HtmlDoc d[_1_], JSContext* ctx)
{
    (void)d;
    try(init_class(&storage_class_id, &(JSClassDef){"storage",.finalizer=NULL}, ctx));
    JSValue storage_proto = JS_NewObject(ctx);
    if (JS_IsException(storage_proto)) return err_jse("new object failed");
    try(set_property_fn_list(ctx, storage_proto, storage_fn_list));
    JS_SetClassProto(ctx, storage_class_id, storage_proto);

    JSValue local_obj = JS_NewObjectClass(ctx, storage_class_id);
    /* JS_SetOpaque(local_obj, local_storage); */
    if (-1 == JS_SetPropertyStr(ctx, global, "localStorage", local_obj))
        return err_jse("set property failure");

    JSValue session_obj = JS_NewObjectClass(ctx, storage_class_id);
    /* JS_SetOpaque(session_obj, session_storage); */
    if (-1 == JS_SetPropertyStr(ctx, global, "sessionStorage", session_obj))
        return err_jse("set property failure");

    return Ok;
}

/** storage **/


Err
jse_init(HtmlDoc* htmldoc) {
    Err e = Ok;

    if (!htmldoc) return err_internal("no HtmlDoc");

    JsEngine* js = htmldoc_js(htmldoc);
    if (!js) return err_internal("no JsEngine in HtmlDoc");

    js->rt = JS_NewRuntime();
    if (!js->rt) err_internal("could not initialize quickjs runtime");

    js->ctx = JS_NewContext(js->rt);
    if (!js->ctx) { e=err_internal("could not initialize quickjs runtime"); goto Fail; }


    JS_SetContextOpaque(js->ctx, htmldoc);

    JSValue global    = JS_GetGlobalObject(js->ctx);

    try(element_init(js->ctx));


    tryjmp(e,Fail, global_add_document(global, htmldoc, js->ctx));
    tryjmp(e,Fail, global_add_console(global, htmldoc, js->ctx));
    tryjmp(e,Fail, global_add_window(global, js->ctx));
    tryjmp(e,Fail, global_add_location(global, htmldoc, js->ctx));
    tryjmp(e,Fail, global_add_storages(global, htmldoc, js->ctx));
    tryjmp(e,Fail, global_add_navigator(global, htmldoc, js->ctx));

    tryjmp(e,Fail, set_property_fn_list(js->ctx, global, global_fn_list));

    JS_FreeValue(js->ctx, global);
    return Ok;
Fail:
    JS_FreeValue(js->ctx, global);

    jse_clean(js);
    return e;
}


Err
jse_eval(JsEngine js[_1_], Session* s,  StrView script, CmdOut* out)
{
    (void)s; //TODO: do not pass it anymore
    if (!script.len) return err_jse("jse_eval cannot evaluate nullptr\n");

    JSContext *ctx = jse_context(js);
    if (!ctx) return err_jse("no js context (js engine is enabled with .js)");
    
    JSValue result = JS_Eval(ctx, script.items, script.len, NULL, JS_EVAL_TYPE_GLOBAL);
    
    Err err = Ok;

    Str* consolebuf = jse_consolebuf(js);
    if (len__(consolebuf)) {
        try(msg_ln__(out, consolebuf));
        str_reset(consolebuf);
    }

    if (JS_IsException(result)) {
        JSValue error = JS_GetException(ctx);
        const char *error_str = JS_ToCString(ctx, error);
        err = err_fmt("warn: evaluating js: %s\n", error_str ? error_str : "(unknown error)");
        JS_FreeCString(ctx, error_str);
        JS_FreeValue(ctx, error);
#ifdef AHRE_DEBUG
        msg_ln__(out, svl("jse: an exception ocurred evaluating the script:"));
        msg_ln__(out, script);
#endif
    } else {
        const char* str = JS_ToCString(ctx, result);
        size_t str_len  = strlen(str);
        try(msg__(out,  svl("  => ")));
        if (str && str_len) try(msg_ln__(out, str));
        else try(msg__(out,  svl("\\0")));
        JS_FreeCString(ctx, str);
    }
    
    JS_FreeValue(ctx, result);
    return err;
}



#else /* quickjs disabled: */
typedef int _quickjs_disabled_;
#endif
