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


static JSClassID console_class_id     = 0;
static JSClassID node_class_id        = 0;
static JSClassID element_class_id     = 0;
static JSClassID document_class_id    = 0;
static JSClassID location_class_id    = 0;
static JSClassID navigator_class_id   = 0;
static JSClassID storage_class_id     = 0;
static JSClassID performance_class_id = 0;
static JSClassID URLSearchParams_class_id = 0;
/* static JSClassID dom_token_list_class_id = 0; */
/* static JSClassID classList_class_id = 0; */


typedef struct {
    const JSCFunctionListEntry* fn_list;
    size_t                      fn_count;
    JSClassID*                  class_id;
    JSClassID*                  base_class_id;
    const char*                 name;
    JSClassFinalizer*           finalizer;
    JSCFunction*                ctor;
} JsClass;

#define mk_jsclass(ClassName, BaseClassIdPtr) (JsClass){\
    .fn_list       = ClassName ## _fn_list,\
    .fn_count      = COUNT(ClassName ## _fn_list),\
    .class_id      = & ClassName ## _class_id,\
    .base_class_id = BaseClassIdPtr,\
    .name          = stringify(ClassName)\
}

#define mk_jsclass_ctor_fin(ClassName, BaseClassIdPtr, Ctor, Finalizer) (JsClass){\
    .fn_list       = ClassName ## _fn_list,\
    .fn_count      = COUNT(ClassName ## _fn_list),\
    .class_id      = & ClassName ## _class_id,\
    .base_class_id = BaseClassIdPtr,\
    .name          = stringify(ClassName),\
    .finalizer     = Finalizer,\
    .ctor          = Ctor\
}

#define COUNT(X) (sizeof(X)/sizeof(*X))

#define mk_proto_fns(Name) ((ProtoFns){.proto=Name ## _proto, .fn_list= Name ## _fn_list, .len=COUNT(Name ## _fn_list)})

static JSValueConst console_log_not_implemented_impl(JSContext* ctx, StrView fname);
#define console_log_not_implemented(Ctx) console_log_not_implemented_impl(Ctx, sv(__func__))

/*
 * I'm using many macros here to make this file shorted and write less
*/

#define validate_jsv(Value) _Generic((Value), JSValue: Value)
#define tryjs(Expr) do{\
    JSValueConst ahre_jsv_=validate_jsv((Expr));if (JS_IsException(ahre_jsv_)) return ahre_jsv_;}while(0) 

#define tryjmpjs(Lval, Label, Expr) do{\
    Lval=validate_jsv((Expr));if (JS_IsException(Lval)) goto Label;}while(0)

#define jsassert_has_params(N, Argc) do{\
    if (Argc < N) return JS_ThrowTypeError(ctx, "fn %s requires %n params", __func__, N); }

#define err_jse(Msg) err_fmt("error jse: %s %s\n", Msg, file_line__)
#define throw(Msg) return JS_ThrowPlainError(ctx, "ahre: %s: %s", Msg, file_line__)

#define js_fn__(FnName) JSValue FnName(JSContext* ctx, JSValueConst this, int argc, JSValueConst *argv)
#define js_fn_n__(FnName, ...) JSValue FnName(JSContext* ctx, JSValueConst this, int argc, JSValueConst *argv, __VA_ARGS__)
#define js_get__(FnName) JSValue FnName(JSContext *ctx, JSValueConst this)
#define js_get_n__(FnName, ...) JSValue FnName(JSContext *ctx, JSValueConst this, __VA_ARGS__)
#define js_set__(FnName) JSValue FnName(JSContext* ctx, JSValueConst this, JSValueConst val)
#define js_set_n__(FnName, ...) JSValue FnName(JSContext* ctx, JSValueConst this, JSValueConst val, __VA_ARGS__)

#define xstringify(N) #N
#define stringify(N) xstringify(N)

#define NOT_IMPL_MSG "not implemented. You can send a patch to:\n  https://codeberg.org/nsm/ahre"
#define not_impl_fn_name__(Name) jse_fn_not_implemented_ ## Name
#define mk_not_imple_fn_with_name(Name) \
    static js_fn__(not_impl_fn_name__(Name)) {(void)this; (void)argc; (void)argv;\
        console_log_not_implemented(ctx); return JS_UNDEFINED; }
    /* return JS_ThrowPlainError(ctx, "ahjs: fn '"# Name "' " NOT_IMPL_MSG); } */

#define mk_not_impl_fn(NS, Fn) mk_not_imple_fn_with_name(NS ## _ ## Fn)

#define mk_not_impl_fn_list_entry(NS, Attr) \
    JS_CFUNC_DEF(stringify(Attr), 0, not_impl_fn_name__(NS ## _ ## Attr))

#define not_impl_getter_name__(Name) jse_getter_not_implemented_ ## Name
#define mk_not_impl_getter(Name) \
    static js_get__(not_impl_getter_name__(Name)) {(void)this;\
        console_log_not_implemented(ctx); return JS_UNDEFINED; }
    /* return JS_ThrowPlainError(ctx, "ahjs: getter '"# Name "' " NOT_IMPL_MSG); } */

#define not_impl_setter_name__(Name) jse_setter_not_implemented_ ## Name
#define mk_not_impl_setter(Name) \
    static js_set__(not_impl_setter_name__(Name)) {(void)this; (void)val;\
        console_log_not_implemented(ctx); return JS_UNDEFINED; }
    /* return JS_ThrowPlainError(ctx, "ahjs: setter '"# Name "' " NOT_IMPL_MSG); } */

#define mk_not_impl_getset(NS, Attr) \
    mk_not_impl_getter(NS ## _ ## Attr) \
    mk_not_impl_setter(NS ## _ ## Attr)

#define mk_not_impl_getset_list_entry(NS, Attr) \
    JS_CGETSET_DEF(stringify(Attr), not_impl_getter_name__(NS ## _ ## Attr), not_impl_setter_name__(NS ## _ ## Attr))


#define set_property_str(Ctx, Obj, Name, Value) (\
     -1 == JS_SetPropertyStr(Ctx, Obj, Name, Value) ? err_fmt("ahjs error: could not set propery (in %s)", __func__) : Ok)

#define set_property_fn_list_len(Ctx, Obj, Arr, ArrLen) (\
    JS_SetPropertyFunctionList(Ctx, Obj, Arr, ArrLen) == 0 \
    ? Ok : err_fmt("ahjs error: could not set property fn list (in %s)",  __func__ ))

#define set_property_fn_list(Ctx, Obj, Arr) (\
    JS_SetPropertyFunctionList(Ctx, Obj, Arr, sizeof(Arr)/sizeof(*Arr)) == 0 \
    ? Ok : err_fmt("ahjs error: could not set property fn list (in %s)",  __func__ ))


static js_set__(js_set_noop) { (void)ctx; (void)this; (void)val; return JS_UNDEFINED; }


static Err
init_js_class(JsClass c[_1_], JSContext* ctx)
{
    JSRuntime* rt = JS_GetRuntime(ctx);
    JS_NewClassID(rt, c->class_id);
    if (JS_NewClass(rt, *c->class_id, &(JSClassDef){c->name, .finalizer=c->finalizer}))
        return err_jse("could not initialize class");
    if (c->fn_list && c->fn_count) {
        JSValue proto;
        if (c->base_class_id) proto = JS_NewObjectClass(ctx, *c->base_class_id);
        else                  proto = JS_NewObject(ctx);
        if (JS_IsException(proto)) fail("could not initialize prototype instance for js class");

        try(set_property_fn_list_len(ctx, proto, c->fn_list, c->fn_count));

        if (c->ctor) {
            JSValue ctor_fn = JS_NewCFunction2(ctx, c->ctor, c->name, 2, JS_CFUNC_constructor, 0);
            if (JS_IsException(ctor_fn)) {
                JS_FreeValue(ctx, proto);
                return err_jse("could not create constructor function");
            }
            JS_SetConstructor(ctx, ctor_fn, proto);
            JSValue global = JS_GetGlobalObject(ctx);
            try(set_property_str(ctx, global, c->name, ctor_fn));
            JS_FreeValue(ctx, global);
        }
        JS_SetClassProto(ctx, *c->class_id, proto);
    }
    return Ok;
}


static Err
init_instance(JSValue instance[_1_], JSClassID class_id, JSContext* ctx)
{
    *instance = JS_NewObjectClass(ctx, class_id);
    if (JS_IsException(*instance)) return err_jse("could not create the console");
    return Ok;
}


static JSValue
get_global(JSContext* ctx, const char* name)
{
    JSValue global    = JS_GetGlobalObject(ctx);
    JSValue obj = JS_GetPropertyStr(ctx, global, name);
    JS_FreeValue(ctx, global);
    return obj;
}


/* ---- Console ---- */

static JSValue
console_msg(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv, StrView prefix) {
    JSValue rv  = JS_UNDEFINED;
    Str*    buf = JS_GetOpaque(this, console_class_id);
    Err     e   = Ok;
    if (!buf) return JS_ThrowTypeError(ctx, "js-engine: could not get console");
    if (prefix.len) tryjmp(e,Ret, str_append(buf, prefix));

    for (int i = 0; i < argc; ++i) {
        size_t len;
        const char* arg = JS_ToCStringLen(ctx, &len, argv[i]);
        if (!arg) return JS_ThrowTypeError(ctx, "invalid arg");
        if (len) {
            e = str_append(buf, sv(arg, len));
            JS_FreeCString(ctx, arg);
            if (e) throw(e);
        }
    }
    tryjmp (e,Ret, str_append(buf, svl("\n")));
Ret:
    if (e) throw(e);
    return rv;
}


static js_fn__(console_log) { return console_msg(ctx, this, argc, argv, svl("js console log: ")); }
static js_fn__(console_warn) { return console_msg(ctx, this, argc, argv, svl("js console warn: ")); }
static js_fn__(console_error) { return console_msg(ctx, this, argc, argv, svl("js console error: ")); }

mk_not_impl_fn(console, assert)
mk_not_impl_fn(console, clear)
mk_not_impl_fn(console, count)
mk_not_impl_fn(console, countReset)
mk_not_impl_fn(console, debug)
mk_not_impl_fn(console, dir)
mk_not_impl_fn(console, dirxml)
mk_not_impl_fn(console, exception)
mk_not_impl_fn(console, group)
mk_not_impl_fn(console, groupCollapsed)
mk_not_impl_fn(console, groupEnd)
mk_not_impl_fn(console, info)
mk_not_impl_fn(console, profile)
mk_not_impl_fn(console, profileEnd)
mk_not_impl_fn(console, table)
mk_not_impl_fn(console, time)
mk_not_impl_fn(console, timeEnd)
mk_not_impl_fn(console, timeLog)
mk_not_impl_fn(console, timeStamp)
mk_not_impl_fn(console, trace)

static const JSCFunctionListEntry console_fn_list[] = {
    mk_not_impl_fn_list_entry(console, assert),
    mk_not_impl_fn_list_entry(console, clear),
    mk_not_impl_fn_list_entry(console, count),
    mk_not_impl_fn_list_entry(console, countReset),
    mk_not_impl_fn_list_entry(console, debug),
    mk_not_impl_fn_list_entry(console, dir),
    mk_not_impl_fn_list_entry(console, dirxml),
    mk_not_impl_fn_list_entry(console, exception),
    mk_not_impl_fn_list_entry(console, group),
    mk_not_impl_fn_list_entry(console, groupCollapsed),
    mk_not_impl_fn_list_entry(console, groupEnd),
    mk_not_impl_fn_list_entry(console, info),
    mk_not_impl_fn_list_entry(console, profile),
    mk_not_impl_fn_list_entry(console, profileEnd),
    mk_not_impl_fn_list_entry(console, table),
    mk_not_impl_fn_list_entry(console, time),
    mk_not_impl_fn_list_entry(console, timeEnd),
    mk_not_impl_fn_list_entry(console, timeLog),
    mk_not_impl_fn_list_entry(console, timeStamp),
    mk_not_impl_fn_list_entry(console, trace),
    JS_CFUNC_DEF("error", 1, console_error),
    JS_CFUNC_DEF("log",   1, console_log),
    JS_CFUNC_DEF("warn",  1, console_warn),
};


static Err
singleton_add__(
    JSClassID   class_id,
    const char* name,
    JSContext*  ctx,
    JSValue     global,
    void*       opaque
) {
    JSValue obj;
    try(init_instance(&obj, class_id, ctx));
    if (JS_SetOpaque(obj, opaque)) err_jse("could not set the singleton");
    try(set_property_str(ctx, global, name, obj));
    return Ok;
}

#define singleton_add(Name, Global, Ctx, Opaque) singleton_add__(Name ## _class_id, stringify(Name), Ctx, Global, Opaque)


static DomElem
js_value_to_dom_elem(JSValueConst jsv)
{
    DomNodePtr nodeptr;
    DomElemPtr elemptr;
    DomElem elem = (DomElem){0};
    if      ((nodeptr = JS_GetOpaque(jsv, node_class_id)))    elem = dom_elem_from_nodeptr(nodeptr);
    else if ((elemptr = JS_GetOpaque(jsv, element_class_id))) elem = dom_elem_from_ptr(elemptr);
    return elem;
}


static JSValueConst
console_log_not_implemented_impl(JSContext* ctx, StrView fname) {
    JSValueConst console = get_global(ctx, "console");
    Str*    buf = JS_GetOpaque(console, console_class_id);
    Err e = Ok;
    tryjmp(e,Clean, str_append(buf, svl("TODO: ")));
    tryjmp(e,Clean, str_append(buf, fname));
    tryjmp(e,Clean, str_append(buf, svl(" not implemented.\n You can send patch to https://codeberg.org/nsm/ahre\n")));
Clean:
    JS_FreeValue(ctx, console);
    if (e) throw(e);
    return JS_UNDEFINED;
}

static JSValueConst __attribute__((unused))
console_log_helper(JSContext* ctx, StrView msg) {
    JSValueConst console = get_global(ctx, "console");
    Str*    buf = JS_GetOpaque(console, console_class_id);
    Err e = Ok;
    tryjmp(e,Clean, str_append(buf, msg));
Clean:
    JS_FreeValue(ctx, console);
    if (e) throw(e);
    return JS_UNDEFINED;
}


/* ---- EventTarget ---- */
//TODO
static js_fn__(event_target_addEventListener)
{
    (void)this;(void)argc;(void)argv;
    tryjs(console_log_not_implemented(ctx));
    return JS_UNDEFINED;
}

/** event target */

/* ---- Node ---- */


static js_set_n__(_set_textContent_impl, DomElem elem) {
    (void)this;
    if (isnull(elem)) return JS_ThrowTypeError(ctx, "ahre: cannot set textContext of NULL");
    size_t len;
    const char* new_content = JS_ToCStringLen(ctx, &len, val);

    Err e = dom_elem_set_text_content(elem, sv(new_content, len));
    JS_FreeCString(ctx, new_content);
    if (e) return JS_ThrowTypeError(ctx, "%s", e);
    return JS_UNDEFINED;
}

static js_fn__(node_appendChild)
{
    (void)argc;(void)this;
    // TODO0: manipulate actual DOM
    tryjs(console_log_not_implemented(ctx));
    return JS_DupValue(ctx, argv[0]);
}

mk_not_impl_getter(node_textContent)
static js_set__(node_set_textContent)
{
    DomElem elem = js_value_to_dom_elem(this);
    return _set_textContent_impl(ctx, this, val, elem);
}

mk_not_impl_getset(node, baseURI)
mk_not_impl_getset(node, childNodes)
mk_not_impl_getset(node, firstChild)
mk_not_impl_getset(node, isConnected)
mk_not_impl_getset(node, lastChild)
mk_not_impl_getset(node, nextSibling)
mk_not_impl_getset(node, nodeName)
mk_not_impl_getset(node, nodeType)
mk_not_impl_getset(node, nodeValue)
mk_not_impl_getset(node, ownerDocument)
mk_not_impl_getset(node, parentNode)
mk_not_impl_getset(node, parentElement)
mk_not_impl_getset(node, previousSibling)

mk_not_impl_fn(node, cloneNode)
mk_not_impl_fn(node, compareDocumentPosition)
mk_not_impl_fn(node, contains)
mk_not_impl_fn(node, getRootNode)
mk_not_impl_fn(node, hasChildNodes)
mk_not_impl_fn(node, insertBefore)
mk_not_impl_fn(node, isDefaultNamespace)
mk_not_impl_fn(node, isEqualNode)
mk_not_impl_fn(node, isSameNode)
mk_not_impl_fn(node, lookupPrefix)
mk_not_impl_fn(node, lookupNamespaceURI)
mk_not_impl_fn(node, normalize)
mk_not_impl_fn(node, removeChild)
mk_not_impl_fn(node, replaceChild)

static js_get__(node_get_innerHtml) { (void)ctx; (void)this;
    tryjs(console_log_not_implemented(ctx));
    return JS_UNDEFINED;
}

static js_set__(node_set_innerHtml) { (void)ctx; (void)this;(void)val;
    tryjs(console_log_not_implemented(ctx));
    return JS_UNDEFINED;
}

static js_get__(node_get_className)
{
    DomElem elem = js_value_to_dom_elem(this);
    StrView value = dom_elem_attr_value(elem, svl("class"));
    if (!value.len || !value.items) return JS_NewString(ctx, "");
    return JS_NewStringLen(ctx, (char*)value.items, value.len);
}

static js_set__(node_set_className)
{
    DomElem elem = js_value_to_dom_elem(this);
    size_t len;
    const char *cstr = JS_ToCStringLen(ctx, &len, val);
    if (!cstr) return JS_EXCEPTION;
    Err e = dom_elem_set_attr(elem, svl("class"), sv(cstr, len));
    JS_FreeCString(ctx, cstr);
    if (e) return JS_ThrowTypeError(ctx, "%s", e);
    return JS_UNDEFINED;
}

static const JSCFunctionListEntry node_fn_list[] = {
    JS_CGETSET_DEF("textContent", not_impl_getter_name__(node_textContent), node_set_textContent),
    JS_CGETSET_DEF("innerHTML", node_get_innerHtml, node_set_innerHtml),
    JS_CGETSET_DEF("className", node_get_className, node_set_className),
    JS_CFUNC_DEF("appendChild",  1, node_appendChild),

    mk_not_impl_getset_list_entry(node, baseURI),
    mk_not_impl_getset_list_entry(node, childNodes),
    mk_not_impl_getset_list_entry(node, firstChild),
    mk_not_impl_getset_list_entry(node, isConnected),
    mk_not_impl_getset_list_entry(node, lastChild),
    mk_not_impl_getset_list_entry(node, nextSibling),
    mk_not_impl_getset_list_entry(node, nodeName),
    mk_not_impl_getset_list_entry(node, nodeType),
    mk_not_impl_getset_list_entry(node, nodeValue),
    mk_not_impl_getset_list_entry(node, ownerDocument),
    mk_not_impl_getset_list_entry(node, parentNode),
    mk_not_impl_getset_list_entry(node, parentElement),
    mk_not_impl_getset_list_entry(node, previousSibling),

    mk_not_impl_fn_list_entry(node, cloneNode),
    mk_not_impl_fn_list_entry(node, compareDocumentPosition),
    mk_not_impl_fn_list_entry(node, contains),
    mk_not_impl_fn_list_entry(node, getRootNode),
    mk_not_impl_fn_list_entry(node, hasChildNodes),
    mk_not_impl_fn_list_entry(node, insertBefore),
    mk_not_impl_fn_list_entry(node, isDefaultNamespace),
    mk_not_impl_fn_list_entry(node, isEqualNode),
    mk_not_impl_fn_list_entry(node, isSameNode),
    mk_not_impl_fn_list_entry(node, lookupPrefix),
    mk_not_impl_fn_list_entry(node, lookupNamespaceURI),
    mk_not_impl_fn_list_entry(node, normalize),
    mk_not_impl_fn_list_entry(node, removeChild),
    mk_not_impl_fn_list_entry(node, replaceChild),
};


/* ---- Element ---- */

static js_get__(element_get_id)
{
    DomElem elem = js_value_to_dom_elem(this);

    StrView value = dom_elem_attr_value(elem, svl("id"));
    if (!value.len || !value.items) return JS_NULL;

    return JS_NewStringLen(ctx, (char*)value.items, value.len);
}

static js_set__(element_set_id)
{
    DomElem elem = js_value_to_dom_elem(this);
    if (isnull(elem)) return JS_ThrowTypeError(ctx, "ahjs element is null");

    StrView value = dom_elem_attr_value(elem, svl("id"));
    if (!value.len || !value.items) return JS_NULL;

    return JS_NewStringLen(ctx, (char*)value.items, value.len);

    size_t len;
    const char *cstr = JS_ToCStringLen(ctx, &len, val);
    if (!cstr) {
        JS_FreeCString(ctx, cstr);
        return JS_EXCEPTION;
    }
    Err e = dom_elem_set_attr(elem, svl("id"), sv(cstr, len));
    JS_FreeCString(ctx, cstr);
    if (e) return JS_ThrowTypeError(ctx, "%s", e);
    return JS_UNDEFINED;
}

static JSValue
element_getAttribute(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv)
{
    if (argc != 1) return JS_ThrowTypeError(ctx, "Expected 1 argument");
    DomElem elem = js_value_to_dom_elem(this);

    size_t attrlen;
    const char* attr = JS_ToCStringLen(ctx, &attrlen, argv[0]);
    if (!attr) return JS_ThrowTypeError(ctx, "invalid attribute");

    StrView value = dom_elem_attr_value(elem, sv(attr, attrlen));
    if (!value.len || !value.items) return JS_NULL;

    return JS_NewStringLen(ctx, (char*)value.items, value.len);
}


static JSValue
element_setAttribute(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
{
    if (argc != 2) return JS_ThrowTypeError(ctx, "setAttribute: name and value needed");
    DomElem elem = js_value_to_dom_elem(self);
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
    if (e) return JS_ThrowTypeError(ctx, "%s", e);
    return JS_UNDEFINED;
}

static JSValue
element_removeAttribute(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
{
    if (argc != 1) return JS_ThrowTypeError(ctx, "removeAttribute: name needed");
    DomElem elem = js_value_to_dom_elem(self);
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
    DomElem elem = js_value_to_dom_elem(self);
    if (isnull(elem)) return JS_ThrowTypeError(ctx, "invalid element");
    lxb_dom_node_destroy(dom_node_from_elem(elem).ptr);
    return JS_UNDEFINED;
}


static JSValue
element_js_value_from_dom_elem(JSContext *ctx, DomElem elem)
{
    JSValue js_element = JS_NewObjectClass(ctx, element_class_id);
    if (JS_SetOpaque(js_element, elem.ptr))
        return JS_ThrowTypeError(ctx, "ahjs: could not set opaque val");

    return js_element;
}



mk_not_impl_getset(element, assignedSlot)
mk_not_impl_getset(element, attributes)
mk_not_impl_getset(element, childElementCount)
mk_not_impl_getset(element, children)
mk_not_impl_getset(element, classList)
mk_not_impl_getset(element, className)
mk_not_impl_getset(element, clientHeight)
mk_not_impl_getset(element, clientLeft)
mk_not_impl_getset(element, clientTop)
mk_not_impl_getset(element, clientWidth)
mk_not_impl_getset(element, currentCSSZoom)
mk_not_impl_getset(element, customElementRegistry)
mk_not_impl_getset(element, elementTiming)
mk_not_impl_getset(element, firstElementChild)
mk_not_impl_getset(element, lastElementChild)
mk_not_impl_getset(element, localName)
mk_not_impl_getset(element, namespaceURI)
mk_not_impl_getset(element, nextElementSibling)
mk_not_impl_getset(element, outerHTML)
mk_not_impl_getset(element, part)
mk_not_impl_getset(element, prefix)
mk_not_impl_getset(element, previousElementSibling)
mk_not_impl_getset(element, scrollHeight)
mk_not_impl_getset(element, scrollLeft)
mk_not_impl_getset(element, scrollLeftMax)
mk_not_impl_getset(element, scrollTop)
mk_not_impl_getset(element, scrollTopMax)
mk_not_impl_getset(element, scrollWidth)
mk_not_impl_getset(element, shadowRoot)
mk_not_impl_getset(element, slot)
mk_not_impl_getset(element, tagName)
mk_not_impl_getset(element, ariaAtomic)
mk_not_impl_getset(element, ariaAutoComplete)
mk_not_impl_getset(element, ariaBrailleLabel)
mk_not_impl_getset(element, ariaBrailleRoleDescription)
mk_not_impl_getset(element, ariaBusy)
mk_not_impl_getset(element, ariaChecked)
mk_not_impl_getset(element, ariaColCount)
mk_not_impl_getset(element, ariaColIndex)
mk_not_impl_getset(element, ariaColIndexText)
mk_not_impl_getset(element, ariaColSpan)
mk_not_impl_getset(element, ariaCurrent)
mk_not_impl_getset(element, ariaDescription)
mk_not_impl_getset(element, ariaDisabled)
mk_not_impl_getset(element, ariaExpanded)
mk_not_impl_getset(element, ariaHasPopup)
mk_not_impl_getset(element, ariaHidden)
mk_not_impl_getset(element, ariaInvalid)
mk_not_impl_getset(element, ariaKeyShortcuts)
mk_not_impl_getset(element, ariaLabel)
mk_not_impl_getset(element, ariaLevel)
mk_not_impl_getset(element, ariaLive)
mk_not_impl_getset(element, ariaModal)
mk_not_impl_getset(element, ariaMultiline)
mk_not_impl_getset(element, ariaMultiSelectable)
mk_not_impl_getset(element, ariaOrientation)
mk_not_impl_getset(element, ariaPlaceholder)
mk_not_impl_getset(element, ariaPosInSet)
mk_not_impl_getset(element, ariaPressed)
mk_not_impl_getset(element, ariaReadOnly)
mk_not_impl_getset(element, ariaRelevant)
mk_not_impl_getset(element, ariaRequired)
mk_not_impl_getset(element, ariaRoleDescription)
mk_not_impl_getset(element, ariaRowCount)
mk_not_impl_getset(element, ariaRowIndex)
mk_not_impl_getset(element, ariaRowIndexText)
mk_not_impl_getset(element, ariaRowSpan)
mk_not_impl_getset(element, ariaSelected)
mk_not_impl_getset(element, ariaSetSize)
mk_not_impl_getset(element, ariaSort)
mk_not_impl_getset(element, ariaValueMax)
mk_not_impl_getset(element, ariaValueMin)
mk_not_impl_getset(element, ariaValueNow)
mk_not_impl_getset(element, ariaValueText)
mk_not_impl_getset(element, role)
mk_not_impl_getset(element, ariaActiveDescendantElement)
mk_not_impl_getset(element, ariaControlsElements)
mk_not_impl_getset(element, ariaDescribedByElements)
mk_not_impl_getset(element, ariaDetailsElements)
mk_not_impl_getset(element, ariaErrorMessageElements)
mk_not_impl_getset(element, ariaFlowToElements)
mk_not_impl_getset(element, ariaLabelledByElements)
mk_not_impl_getset(element, ariaOwnsElements)

mk_not_impl_fn(element, after)
mk_not_impl_fn(element, animate)
mk_not_impl_fn(element, append)
mk_not_impl_fn(element, ariaNotify)
mk_not_impl_fn(element, attachShadow)
mk_not_impl_fn(element, before)
mk_not_impl_fn(element, checkVisibility)
mk_not_impl_fn(element, closest)
mk_not_impl_fn(element, computedStyleMap)
mk_not_impl_fn(element, getAnimations)
mk_not_impl_fn(element, getAttributeNS)
mk_not_impl_fn(element, getAttributeNames)
mk_not_impl_fn(element, getAttributeNode)
mk_not_impl_fn(element, getAttributeNodeNS)
mk_not_impl_fn(element, getBoundingClientRect)
mk_not_impl_fn(element, getClientRects)
mk_not_impl_fn(element, getElementsByClassName)
mk_not_impl_fn(element, getElementsByTagName)
mk_not_impl_fn(element, getElementsByTagNameNS)
mk_not_impl_fn(element, getHTML)
mk_not_impl_fn(element, hasAttribute)
mk_not_impl_fn(element, hasAttributeNS)
mk_not_impl_fn(element, hasAttributes)
mk_not_impl_fn(element, hasPointerCapture)
mk_not_impl_fn(element, insertAdjacentElement)
mk_not_impl_fn(element, insertAdjacentHTML)
mk_not_impl_fn(element, insertAdjacentText)
mk_not_impl_fn(element, matches)
mk_not_impl_fn(element, moveBefore)
mk_not_impl_fn(element, prepend)
mk_not_impl_fn(element, querySelector)
mk_not_impl_fn(element, querySelectorAll)
mk_not_impl_fn(element, releasePointerCapture)
mk_not_impl_fn(element, removeAttributeNS)
mk_not_impl_fn(element, removeAttributeNode)
mk_not_impl_fn(element, replaceChildren)
mk_not_impl_fn(element, replaceWith)
mk_not_impl_fn(element, requestFullscreen)
mk_not_impl_fn(element, requestPointerLock)
mk_not_impl_fn(element, scroll)
mk_not_impl_fn(element, scrollBy)
mk_not_impl_fn(element, scrollIntoView)
mk_not_impl_fn(element, scrollIntoViewIfNeeded)
mk_not_impl_fn(element, scrollTo)
mk_not_impl_fn(element, setAttributeNS)
mk_not_impl_fn(element, setAttributeNode)
mk_not_impl_fn(element, setAttributeNodeNS)
mk_not_impl_fn(element, setCapture)
mk_not_impl_fn(element, setHTML)
mk_not_impl_fn(element, setHTMLUnsafe)
mk_not_impl_fn(element, setPointerCapture)
mk_not_impl_fn(element, toggleAttribute)


static const JSCFunctionListEntry element_fn_list[] = {
    JS_CGETSET_DEF("id", element_get_id, element_set_id),
    JS_CFUNC_DEF("addEventListener", 1, event_target_addEventListener),/*TODO1: this should be inherited from event target */
    JS_CFUNC_DEF("getAttribute",     1, element_getAttribute),
    JS_CFUNC_DEF("remove",           0, element_remove),
    JS_CFUNC_DEF("removeAttribute",  1, element_removeAttribute),
    JS_CFUNC_DEF("setAttribute",     2, element_setAttribute),


    mk_not_impl_getset_list_entry(element, assignedSlot),
    mk_not_impl_getset_list_entry(element, attributes),
    mk_not_impl_getset_list_entry(element, childElementCount),
    mk_not_impl_getset_list_entry(element, children),
    mk_not_impl_getset_list_entry(element, classList),
    mk_not_impl_getset_list_entry(element, className),
    mk_not_impl_getset_list_entry(element, clientHeight),
    mk_not_impl_getset_list_entry(element, clientLeft),
    mk_not_impl_getset_list_entry(element, clientTop),
    mk_not_impl_getset_list_entry(element, clientWidth),
    mk_not_impl_getset_list_entry(element, currentCSSZoom),
    mk_not_impl_getset_list_entry(element, customElementRegistry),
    mk_not_impl_getset_list_entry(element, elementTiming),
    mk_not_impl_getset_list_entry(element, firstElementChild),
    mk_not_impl_getset_list_entry(element, lastElementChild),
    mk_not_impl_getset_list_entry(element, localName),
    mk_not_impl_getset_list_entry(element, namespaceURI),
    mk_not_impl_getset_list_entry(element, nextElementSibling),
    mk_not_impl_getset_list_entry(element, outerHTML),
    mk_not_impl_getset_list_entry(element, part),
    mk_not_impl_getset_list_entry(element, prefix),
    mk_not_impl_getset_list_entry(element, previousElementSibling),
    mk_not_impl_getset_list_entry(element, scrollHeight),
    mk_not_impl_getset_list_entry(element, scrollLeft),
    mk_not_impl_getset_list_entry(element, scrollLeftMax),
    mk_not_impl_getset_list_entry(element, scrollTop),
    mk_not_impl_getset_list_entry(element, scrollTopMax),
    mk_not_impl_getset_list_entry(element, scrollWidth),
    mk_not_impl_getset_list_entry(element, shadowRoot),
    mk_not_impl_getset_list_entry(element, slot),
    mk_not_impl_getset_list_entry(element, tagName),
    mk_not_impl_getset_list_entry(element, ariaAtomic),
    mk_not_impl_getset_list_entry(element, ariaAutoComplete),
    mk_not_impl_getset_list_entry(element, ariaBrailleLabel),
    mk_not_impl_getset_list_entry(element, ariaBrailleRoleDescription),
    mk_not_impl_getset_list_entry(element, ariaBusy),
    mk_not_impl_getset_list_entry(element, ariaChecked),
    mk_not_impl_getset_list_entry(element, ariaColCount),
    mk_not_impl_getset_list_entry(element, ariaColIndex),
    mk_not_impl_getset_list_entry(element, ariaColIndexText),
    mk_not_impl_getset_list_entry(element, ariaColSpan),
    mk_not_impl_getset_list_entry(element, ariaCurrent),
    mk_not_impl_getset_list_entry(element, ariaDescription),
    mk_not_impl_getset_list_entry(element, ariaDisabled),
    mk_not_impl_getset_list_entry(element, ariaExpanded),
    mk_not_impl_getset_list_entry(element, ariaHasPopup),
    mk_not_impl_getset_list_entry(element, ariaHidden),
    mk_not_impl_getset_list_entry(element, ariaInvalid),
    mk_not_impl_getset_list_entry(element, ariaKeyShortcuts),
    mk_not_impl_getset_list_entry(element, ariaLabel),
    mk_not_impl_getset_list_entry(element, ariaLevel),
    mk_not_impl_getset_list_entry(element, ariaLive),
    mk_not_impl_getset_list_entry(element, ariaModal),
    mk_not_impl_getset_list_entry(element, ariaMultiline),
    mk_not_impl_getset_list_entry(element, ariaMultiSelectable),
    mk_not_impl_getset_list_entry(element, ariaOrientation),
    mk_not_impl_getset_list_entry(element, ariaPlaceholder),
    mk_not_impl_getset_list_entry(element, ariaPosInSet),
    mk_not_impl_getset_list_entry(element, ariaPressed),
    mk_not_impl_getset_list_entry(element, ariaReadOnly),
    mk_not_impl_getset_list_entry(element, ariaRelevant),
    mk_not_impl_getset_list_entry(element, ariaRequired),
    mk_not_impl_getset_list_entry(element, ariaRoleDescription),
    mk_not_impl_getset_list_entry(element, ariaRowCount),
    mk_not_impl_getset_list_entry(element, ariaRowIndex),
    mk_not_impl_getset_list_entry(element, ariaRowIndexText),
    mk_not_impl_getset_list_entry(element, ariaRowSpan),
    mk_not_impl_getset_list_entry(element, ariaSelected),
    mk_not_impl_getset_list_entry(element, ariaSetSize),
    mk_not_impl_getset_list_entry(element, ariaSort),
    mk_not_impl_getset_list_entry(element, ariaValueMax),
    mk_not_impl_getset_list_entry(element, ariaValueMin),
    mk_not_impl_getset_list_entry(element, ariaValueNow),
    mk_not_impl_getset_list_entry(element, ariaValueText),
    mk_not_impl_getset_list_entry(element, role),
    mk_not_impl_getset_list_entry(element, ariaActiveDescendantElement),
    mk_not_impl_getset_list_entry(element, ariaControlsElements),
    mk_not_impl_getset_list_entry(element, ariaDescribedByElements),
    mk_not_impl_getset_list_entry(element, ariaDetailsElements),
    mk_not_impl_getset_list_entry(element, ariaErrorMessageElements),
    mk_not_impl_getset_list_entry(element, ariaFlowToElements),
    mk_not_impl_getset_list_entry(element, ariaLabelledByElements),
    mk_not_impl_getset_list_entry(element, ariaOwnsElements),

    mk_not_impl_fn_list_entry(element, after),
    mk_not_impl_fn_list_entry(element, animate),
    mk_not_impl_fn_list_entry(element, append),
    mk_not_impl_fn_list_entry(element, ariaNotify),
    mk_not_impl_fn_list_entry(element, attachShadow),
    mk_not_impl_fn_list_entry(element, before),
    mk_not_impl_fn_list_entry(element, checkVisibility),
    mk_not_impl_fn_list_entry(element, closest),
    mk_not_impl_fn_list_entry(element, computedStyleMap),
    mk_not_impl_fn_list_entry(element, getAnimations),
    mk_not_impl_fn_list_entry(element, getAttributeNS),
    mk_not_impl_fn_list_entry(element, getAttributeNames),
    mk_not_impl_fn_list_entry(element, getAttributeNode),
    mk_not_impl_fn_list_entry(element, getAttributeNodeNS),
    mk_not_impl_fn_list_entry(element, getBoundingClientRect),
    mk_not_impl_fn_list_entry(element, getClientRects),
    mk_not_impl_fn_list_entry(element, getElementsByClassName),
    mk_not_impl_fn_list_entry(element, getElementsByTagName),
    mk_not_impl_fn_list_entry(element, getElementsByTagNameNS),
    mk_not_impl_fn_list_entry(element, getHTML),
    mk_not_impl_fn_list_entry(element, hasAttribute),
    mk_not_impl_fn_list_entry(element, hasAttributeNS),
    mk_not_impl_fn_list_entry(element, hasAttributes),
    mk_not_impl_fn_list_entry(element, hasPointerCapture),
    mk_not_impl_fn_list_entry(element, insertAdjacentElement),
    mk_not_impl_fn_list_entry(element, insertAdjacentHTML),
    mk_not_impl_fn_list_entry(element, insertAdjacentText),
    mk_not_impl_fn_list_entry(element, matches),
    mk_not_impl_fn_list_entry(element, moveBefore),
    mk_not_impl_fn_list_entry(element, prepend),
    mk_not_impl_fn_list_entry(element, querySelector),
    mk_not_impl_fn_list_entry(element, querySelectorAll),
    mk_not_impl_fn_list_entry(element, releasePointerCapture),
    mk_not_impl_fn_list_entry(element, removeAttributeNS),
    mk_not_impl_fn_list_entry(element, removeAttributeNode),
    mk_not_impl_fn_list_entry(element, replaceChildren),
    mk_not_impl_fn_list_entry(element, replaceWith),
    mk_not_impl_fn_list_entry(element, requestFullscreen),
    mk_not_impl_fn_list_entry(element, requestPointerLock),
    mk_not_impl_fn_list_entry(element, scroll),
    mk_not_impl_fn_list_entry(element, scrollBy),
    mk_not_impl_fn_list_entry(element, scrollIntoView),
    mk_not_impl_fn_list_entry(element, scrollIntoViewIfNeeded),
    mk_not_impl_fn_list_entry(element, scrollTo),
    mk_not_impl_fn_list_entry(element, setAttributeNS),
    mk_not_impl_fn_list_entry(element, setAttributeNode),
    mk_not_impl_fn_list_entry(element, setAttributeNodeNS),
    mk_not_impl_fn_list_entry(element, setCapture),
    mk_not_impl_fn_list_entry(element, setHTML),
    mk_not_impl_fn_list_entry(element, setHTMLUnsafe),
    mk_not_impl_fn_list_entry(element, setPointerCapture),
    mk_not_impl_fn_list_entry(element, toggleAttribute),

};


/** element **/


/* Document */

static js_fn__(document_createElement)
{
    if (argc != 1) return JS_ThrowTypeError(ctx, "createElement: tagName needed");
    Dom dom = htmldoc_dom(JS_GetOpaque(this, document_class_id));
    if (isnull(dom)) return JS_ThrowTypeError(ctx, "invalid document");
    size_t len;
    const char *tag = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!tag) return JS_EXCEPTION;
    DomElem elem;
    Err e = dom_elem_init(&elem, dom, sv(tag, len));
    JS_FreeCString(ctx, tag);
    if (e) return JS_ThrowTypeError(ctx, "%s", e);
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
    if (e) return JS_ThrowPlainError(ctx, "%s", e);
    if (isnull(head)) return JS_ThrowTypeError(ctx, "dom not properly initialized");
    JSValue head_js = element_js_value_from_dom_elem(ctx, head);
    return head_js;
}


static js_get__(document_body)
{
    HtmlDoc* d = JS_GetOpaque(this, document_class_id);
    DomElem body;
    Err e = dom_get_body_elem(htmldoc_dom(d), &body);
    if (e) return JS_ThrowPlainError(ctx, "%s", e);
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
    if (e) return JS_ThrowPlainError(ctx, "%s", e);
    return rv;
}


static js_set__(document_set_title)
{
    HtmlDoc* d = JS_GetOpaque(this, document_class_id);
    DomElem title;
    Err e = dom_get_title_elem(htmldoc_dom(d), &title);
    if (e) return JS_ThrowPlainError(ctx, "%s", e);

    size_t len;
    const char* new_content = JS_ToCStringLen(ctx, &len, val);

    e = dom_elem_set_text_content(title, sv(new_content, len));
    if (e) return JS_ThrowTypeError(ctx, "%s", e);
    return JS_UNDEFINED;
}


mk_not_impl_getset(document, activeElement)
mk_not_impl_getset(document, activeViewTransition)
mk_not_impl_getset(document, adoptedStyleSheets)
mk_not_impl_getset(document, characterSet)
mk_not_impl_getset(document, childElementCount)
mk_not_impl_getset(document, children)
mk_not_impl_getset(document, compatMode)
mk_not_impl_getset(document, contentType)
mk_not_impl_getset(document, currentScript)
mk_not_impl_getset(document, customElementRegistry)
mk_not_impl_getset(document, doctype)
/* mk_not_impl_getset(document, documentElement) */
mk_not_impl_getset(document, documentURI)
mk_not_impl_getset(document, embeds)
mk_not_impl_getset(document, featurePolicy)
mk_not_impl_getset(document, firstElementChild)
mk_not_impl_getset(document, fonts)
mk_not_impl_getset(document, forms)
mk_not_impl_getset(document, fragmentDirective)
mk_not_impl_getset(document, fullscreenElement)
mk_not_impl_getset(document, hidden)
mk_not_impl_getset(document, images)
mk_not_impl_getset(document, implementation)
mk_not_impl_getset(document, lastElementChild)
mk_not_impl_getset(document, links)
mk_not_impl_getset(document, pictureInPictureElement)
mk_not_impl_getset(document, pictureInPictureEnabled)
mk_not_impl_getset(document, plugins)
mk_not_impl_getset(document, pointerLockElement)
mk_not_impl_getset(document, prerendering)
mk_not_impl_getset(document, scripts)
mk_not_impl_getset(document, scrollingElement)
mk_not_impl_getset(document, styleSheets)
mk_not_impl_getset(document, timeline)
mk_not_impl_getset(document, visibilityState)
mk_not_impl_getset(document, defaultView)
mk_not_impl_getset(document, designMode)
mk_not_impl_getset(document, dir)
mk_not_impl_getset(document, fullscreenEnabled)
mk_not_impl_getset(document, lastModified)
mk_not_impl_getset(document, readyState)
mk_not_impl_getset(document, referrer)
mk_not_impl_getset(document, URL)
mk_not_impl_getset(document, alinkColor)
mk_not_impl_getset(document, all)
mk_not_impl_getset(document, anchors)
mk_not_impl_getset(document, applets)
mk_not_impl_getset(document, bgColor)
mk_not_impl_getset(document, domain)
mk_not_impl_getset(document, fgColor)
mk_not_impl_getset(document, fullscreen)
mk_not_impl_getset(document, lastStyleSheetSet)
mk_not_impl_getset(document, linkColor)
mk_not_impl_getset(document, preferredStyleSheetSet)
mk_not_impl_getset(document, rootElement)
mk_not_impl_getset(document, selectedStyleSheetSet)
mk_not_impl_getset(document, styleSheetSets)
mk_not_impl_getset(document, vlinkColor)
mk_not_impl_getset(document, xmlEncoding)
mk_not_impl_getset(document, xmlStandalone)
mk_not_impl_getset(document, xmlVersion)


mk_not_impl_setter(document_body)
mk_not_impl_setter(document_head)

mk_not_impl_fn(document, adoptNode)
mk_not_impl_fn(document, append)
mk_not_impl_fn(document, ariaNotify)
mk_not_impl_fn(document, browsingTopics)
mk_not_impl_fn(document, captureEvents)
mk_not_impl_fn(document, caretPositionFromPoint)
mk_not_impl_fn(document, caretRangeFromPoint)
mk_not_impl_fn(document, createAttribute)
mk_not_impl_fn(document, createAttributeNS)
mk_not_impl_fn(document, createCDATASection)
mk_not_impl_fn(document, createComment)
mk_not_impl_fn(document, createDocumentFragment)
mk_not_impl_fn(document, createElementNS)
mk_not_impl_fn(document, createEvent)
mk_not_impl_fn(document, createNodeIterator)
mk_not_impl_fn(document, createProcessingInstruction)
mk_not_impl_fn(document, createRange)
mk_not_impl_fn(document, createTouch)
mk_not_impl_fn(document, createTouchList)
mk_not_impl_fn(document, createTreeWalker)
mk_not_impl_fn(document, elementFromPoint)
mk_not_impl_fn(document, elementsFromPoint)
mk_not_impl_fn(document, enableStyleSheetsForSet)
mk_not_impl_fn(document, exitFullscreen)
mk_not_impl_fn(document, exitPictureInPicture)
mk_not_impl_fn(document, exitPointerLock)
mk_not_impl_fn(document, getAnimations)
mk_not_impl_fn(document, getBoxQuads)
mk_not_impl_fn(document, getElementsByClassName)
mk_not_impl_fn(document, getElementsByTagName)
mk_not_impl_fn(document, getElementsByTagNameNS)
mk_not_impl_fn(document, getSelection)
mk_not_impl_fn(document, hasPrivateToken)
mk_not_impl_fn(document, hasRedemptionRecord)
mk_not_impl_fn(document, hasStorageAccess)
mk_not_impl_fn(document, hasUnpartitionedCookieAccess)
mk_not_impl_fn(document, importNode)
mk_not_impl_fn(document, moveBefore)
mk_not_impl_fn(document, mozSetImageElement)
mk_not_impl_fn(document, prepend)
mk_not_impl_fn(document, querySelector)
mk_not_impl_fn(document, querySelectorAll)
mk_not_impl_fn(document, releaseCapture)
mk_not_impl_fn(document, releaseEvents)
mk_not_impl_fn(document, replaceChildren)
mk_not_impl_fn(document, requestStorageAccess)
mk_not_impl_fn(document, requestStorageAccessFor)
mk_not_impl_fn(document, startViewTransition)
mk_not_impl_fn(document, createExpression)
mk_not_impl_fn(document, createNSResolver)
mk_not_impl_fn(document, evaluate)
mk_not_impl_fn(document, clear)
mk_not_impl_fn(document, close)
mk_not_impl_fn(document, execCommand)
mk_not_impl_fn(document, getElementsByName)
mk_not_impl_fn(document, hasFocus)
mk_not_impl_fn(document, open)
mk_not_impl_fn(document, queryCommandEnabled)
mk_not_impl_fn(document, queryCommandIndeterm)
mk_not_impl_fn(document, queryCommandState)
mk_not_impl_fn(document, queryCommandSupported)
mk_not_impl_fn(document, queryCommandValue)
mk_not_impl_fn(document, write)
mk_not_impl_fn(document, writeln)

static js_fn__(document_createTextNode)
{
    (void)argv;(void)this;
    if (argc != 1) return JS_ThrowTypeError(ctx, "createTextNode: data needed");
    return element_js_value_from_dom_elem(ctx, (DomElem){0}); //TODO1: check this
}


static js_get__(location_getter) { (void)this; return get_global(ctx, "location"); }

mk_not_impl_setter(document_location)

static js_get__(document_get_cookie) {
    HtmlDoc* d = JS_GetOpaque(this, document_class_id);
    if (!d) throw("no document");
    JSValue rv = JS_UNDEFINED;

    ArlOf(Str) cookies = (ArlOf(Str)){0};
    Str        joined  = (Str){0};
    Err        e       = Ok;
    tryjmp(e,Clean, htmldoc_get_cookies(d, &cookies));
    foreach__(Str,&cookies, it) {
        StrView line = sv(it);
        for (size_t i = 0; i < 5; ++i) {
            strview_trim_space_left(&line);
            strview_split_word(&line);
        }
        strview_trim_space_left(&line);
        StrView k = strview_split_word(&line);
        if (!k.len) continue;
        tryjmp(e,Clean, str_append(&joined, k));
        tryjmp(e,Clean, str_append(&joined, svl("=")));
        strview_trim_space_left(&line);
        StrView v = strview_split_word(&line);
        if (v.len) tryjmp(e,Clean, str_append(&joined, v));
        if (it < arlfn(Str,end)(&cookies)) tryjmp(e,Clean, str_append(&joined, svl("; ")));

    }

    rv = JS_NewString(ctx, joined.items ? joined.items : "");
Clean:
    arlfn(Str,clean)(&cookies);
    str_clean(&joined);
    //TODO1: ignore errors?
    return rv;
}

static js_set__(document_set_cookie) {
    Err      e = Ok;
    HtmlDoc* d = JS_GetOpaque(this, document_class_id);
    if (!d) throw("no document");
    const char* cookie = JS_ToCString(ctx, val);
    if (!cookie) return JS_UNDEFINED;

    Str   buf  = (Str){0};
    char* host = NULL;
    char* path = NULL;

    Url*    url  = request_url(htmldoc_request(d));
    StrView line = sv(cookie);

    tryjmp(e,Clean, url_append_host_to_str(*url, &host));
    tryjmp(e,Clean, url_append_path_to_str(*url, &path));

    StrView kv;
    while ((kv = strview_split(&line, ';')).len) {
        if (!kv.len) continue;
        str_reset(&buf);
        /* "example.com"    /1* Hostname *1/ */
        /* SEP "FALSE"      /1* Include subdomains *1/ */
        /* SEP "/"          /1* Path *1/ */
        /* SEP "FALSE"      /1* Secure *1/ */
        /* SEP "0"          /1* Expiry in epoch time format. 0 == Session *1/ */
        /* SEP "foo"        /1* Name *1/ */
        /* SEP "bar";       /1* Value *1/ */
        tryjmp(e,Clean, str_append(&buf, sv(host)));
        tryjmp(e,Clean, str_append(&buf, svl("\tFALSE\t")));
        tryjmp(e,Clean, str_append(&buf, sv(path)));
        tryjmp(e,Clean, str_append(&buf, svl("\tTRUE\t0\t")));

        StrView k = strview_split(&kv, '=');
        tryjmp(e,Clean, str_append(&buf, k));
        tryjmp(e,Clean, str_append(&buf, svl("\t")));
        tryjmp(e,Clean, str_append(&buf, kv));

        e = htmldoc_set_cookielist(d, sv(buf));
        if (e) break;
    }
Clean:
    JS_FreeCString(ctx, cookie);
    str_clean(&buf);
    w_curl_free(host);
    w_curl_free(path);
    if (e) throw(e);
    return JS_UNDEFINED;
}

static js_get__(document_documentElement) {
    HtmlDoc* d = JS_GetOpaque(this, document_class_id);
    if (!d) throw("no document");
    Dom dom = htmldoc_dom(d);
    DomNode root = dom_root(dom);
    DomElem doc_elem = dom_elem_from_node(root);
    return element_js_value_from_dom_elem(ctx, doc_elem);
}


static const JSCFunctionListEntry document_fn_list[] = {
    JS_CGETSET_DEF("body",             document_body,            not_impl_setter_name__(document_body)),
    JS_CGETSET_DEF("cookie",           document_get_cookie,      document_set_cookie),
    JS_CGETSET_DEF("head",             document_head,            not_impl_setter_name__(document_head)),
    JS_CGETSET_DEF("location",         location_getter,          not_impl_setter_name__(document_location)),
    JS_CGETSET_DEF("title",            document_get_title,       document_set_title),
    JS_CGETSET_DEF("documentElemeent", document_documentElement, js_set_noop),

    JS_CFUNC_DEF("createElement",  1, document_createElement),
    JS_CFUNC_DEF("getElementById", 1, document_getElementById),
    JS_CFUNC_DEF("createTextNode", 1, document_createTextNode),

    JS_CFUNC_DEF("addEventListener", 1, event_target_addEventListener),/*TODO1: this should be inherited from event target */


    mk_not_impl_getset_list_entry(document, activeElement),
    mk_not_impl_getset_list_entry(document, activeViewTransition),
    mk_not_impl_getset_list_entry(document, adoptedStyleSheets),
    mk_not_impl_getset_list_entry(document, characterSet),
    mk_not_impl_getset_list_entry(document, childElementCount),
    mk_not_impl_getset_list_entry(document, children),
    mk_not_impl_getset_list_entry(document, compatMode),
    mk_not_impl_getset_list_entry(document, contentType),
    mk_not_impl_getset_list_entry(document, currentScript),
    mk_not_impl_getset_list_entry(document, customElementRegistry),
    mk_not_impl_getset_list_entry(document, doctype),
    /* mk_not_impl_getset_list_entry(document, documentElement), */
    mk_not_impl_getset_list_entry(document, documentURI),
    mk_not_impl_getset_list_entry(document, embeds),
    mk_not_impl_getset_list_entry(document, featurePolicy),
    mk_not_impl_getset_list_entry(document, firstElementChild),
    mk_not_impl_getset_list_entry(document, fonts),
    mk_not_impl_getset_list_entry(document, forms),
    mk_not_impl_getset_list_entry(document, fragmentDirective),
    mk_not_impl_getset_list_entry(document, fullscreenElement),
    mk_not_impl_getset_list_entry(document, hidden),
    mk_not_impl_getset_list_entry(document, images),
    mk_not_impl_getset_list_entry(document, implementation),
    mk_not_impl_getset_list_entry(document, lastElementChild),
    mk_not_impl_getset_list_entry(document, links),
    mk_not_impl_getset_list_entry(document, pictureInPictureElement),
    mk_not_impl_getset_list_entry(document, pictureInPictureEnabled),
    mk_not_impl_getset_list_entry(document, plugins),
    mk_not_impl_getset_list_entry(document, pointerLockElement),
    mk_not_impl_getset_list_entry(document, prerendering),
    mk_not_impl_getset_list_entry(document, scripts),
    mk_not_impl_getset_list_entry(document, scrollingElement),
    mk_not_impl_getset_list_entry(document, styleSheets),
    mk_not_impl_getset_list_entry(document, timeline),
    mk_not_impl_getset_list_entry(document, visibilityState),
    mk_not_impl_getset_list_entry(document, defaultView),
    mk_not_impl_getset_list_entry(document, designMode),
    mk_not_impl_getset_list_entry(document, dir),
    mk_not_impl_getset_list_entry(document, fullscreenEnabled),
    mk_not_impl_getset_list_entry(document, lastModified),
    mk_not_impl_getset_list_entry(document, readyState),
    mk_not_impl_getset_list_entry(document, referrer),
    mk_not_impl_getset_list_entry(document, URL),
    mk_not_impl_getset_list_entry(document, alinkColor),
    mk_not_impl_getset_list_entry(document, all),
    mk_not_impl_getset_list_entry(document, anchors),
    mk_not_impl_getset_list_entry(document, applets),
    mk_not_impl_getset_list_entry(document, bgColor),
    mk_not_impl_getset_list_entry(document, domain),
    mk_not_impl_getset_list_entry(document, fgColor),
    mk_not_impl_getset_list_entry(document, fullscreen),
    mk_not_impl_getset_list_entry(document, lastStyleSheetSet),
    mk_not_impl_getset_list_entry(document, linkColor),
    mk_not_impl_getset_list_entry(document, preferredStyleSheetSet),
    mk_not_impl_getset_list_entry(document, rootElement),
    mk_not_impl_getset_list_entry(document, selectedStyleSheetSet),
    mk_not_impl_getset_list_entry(document, styleSheetSets),
    mk_not_impl_getset_list_entry(document, vlinkColor),
    mk_not_impl_getset_list_entry(document, xmlEncoding),
    mk_not_impl_getset_list_entry(document, xmlStandalone),
    mk_not_impl_getset_list_entry(document, xmlVersion),

    mk_not_impl_fn_list_entry(document, adoptNode),
    mk_not_impl_fn_list_entry(document, append),
    mk_not_impl_fn_list_entry(document, ariaNotify),
    mk_not_impl_fn_list_entry(document, browsingTopics),
    mk_not_impl_fn_list_entry(document, captureEvents),
    mk_not_impl_fn_list_entry(document, caretPositionFromPoint),
    mk_not_impl_fn_list_entry(document, caretRangeFromPoint),
    mk_not_impl_fn_list_entry(document, createAttribute),
    mk_not_impl_fn_list_entry(document, createAttributeNS),
    mk_not_impl_fn_list_entry(document, createCDATASection),
    mk_not_impl_fn_list_entry(document, createComment),
    mk_not_impl_fn_list_entry(document, createDocumentFragment),
    mk_not_impl_fn_list_entry(document, createElementNS),
    mk_not_impl_fn_list_entry(document, createEvent),
    mk_not_impl_fn_list_entry(document, createNodeIterator),
    mk_not_impl_fn_list_entry(document, createProcessingInstruction),
    mk_not_impl_fn_list_entry(document, createRange),
    mk_not_impl_fn_list_entry(document, createTouch),
    mk_not_impl_fn_list_entry(document, createTouchList),
    mk_not_impl_fn_list_entry(document, createTreeWalker),
    mk_not_impl_fn_list_entry(document, elementFromPoint),
    mk_not_impl_fn_list_entry(document, elementsFromPoint),
    mk_not_impl_fn_list_entry(document, enableStyleSheetsForSet),
    mk_not_impl_fn_list_entry(document, exitFullscreen),
    mk_not_impl_fn_list_entry(document, exitPictureInPicture),
    mk_not_impl_fn_list_entry(document, exitPointerLock),
    mk_not_impl_fn_list_entry(document, getAnimations),
    mk_not_impl_fn_list_entry(document, getBoxQuads),
    mk_not_impl_fn_list_entry(document, getElementsByClassName),
    mk_not_impl_fn_list_entry(document, getElementsByTagName),
    mk_not_impl_fn_list_entry(document, getElementsByTagNameNS),
    mk_not_impl_fn_list_entry(document, getSelection),
    mk_not_impl_fn_list_entry(document, hasPrivateToken),
    mk_not_impl_fn_list_entry(document, hasRedemptionRecord),
    mk_not_impl_fn_list_entry(document, hasStorageAccess),
    mk_not_impl_fn_list_entry(document, hasUnpartitionedCookieAccess),
    mk_not_impl_fn_list_entry(document, importNode),
    mk_not_impl_fn_list_entry(document, moveBefore),
    mk_not_impl_fn_list_entry(document, mozSetImageElement),
    mk_not_impl_fn_list_entry(document, prepend),
    mk_not_impl_fn_list_entry(document, querySelector),
    mk_not_impl_fn_list_entry(document, querySelectorAll),
    mk_not_impl_fn_list_entry(document, releaseCapture),
    mk_not_impl_fn_list_entry(document, releaseEvents),
    mk_not_impl_fn_list_entry(document, replaceChildren),
    mk_not_impl_fn_list_entry(document, requestStorageAccess),
    mk_not_impl_fn_list_entry(document, requestStorageAccessFor),
    mk_not_impl_fn_list_entry(document, startViewTransition),
    mk_not_impl_fn_list_entry(document, createExpression),
    mk_not_impl_fn_list_entry(document, createNSResolver),
    mk_not_impl_fn_list_entry(document, evaluate),
    mk_not_impl_fn_list_entry(document, clear),
    mk_not_impl_fn_list_entry(document, close),
    mk_not_impl_fn_list_entry(document, execCommand),
    mk_not_impl_fn_list_entry(document, getElementsByName),
    mk_not_impl_fn_list_entry(document, hasFocus),
    mk_not_impl_fn_list_entry(document, open),
    mk_not_impl_fn_list_entry(document, queryCommandEnabled),
    mk_not_impl_fn_list_entry(document, queryCommandIndeterm),
    mk_not_impl_fn_list_entry(document, queryCommandState),
    mk_not_impl_fn_list_entry(document, queryCommandSupported),
    mk_not_impl_fn_list_entry(document, queryCommandValue),
    mk_not_impl_fn_list_entry(document, write),
    mk_not_impl_fn_list_entry(document, writeln),

};


/** document */


/* ---- Location ---- */

/* mk_not_impl_getset(location, href) */
mk_not_impl_getset(location, protocol)
mk_not_impl_getset(location, host)
mk_not_impl_getset(location, hostname)
mk_not_impl_getset(location, port)
mk_not_impl_getset(location, pathname)
mk_not_impl_getset(location, search)
mk_not_impl_getset(location, hash)
mk_not_impl_getset(location, origin)

mk_not_impl_fn(location, assign)
mk_not_impl_fn(location, reload)
mk_not_impl_fn(location, toString)


static js_get__(location_get_href) {
    HtmlDoc* d = JS_GetOpaque(this, location_class_id);
    if (!d) throw("no document in location");
    char* url;
    Err e = url_cstr_malloc(*htmldoc_url(d), &url);
    if (e) throw(e);
    JSValue rv = JS_NewString(ctx, url);
    w_curl_free(url);
    return rv;
}

static JSValue replace_doc_url (JSContext* ctx, JSValueConst this, JSValueConst val) {
    HtmlDoc* d = JS_GetOpaque(this, location_class_id);
    if (!d) throw("no document in location");
    const char* url = JS_ToCString(ctx, val);
    if (!url) return JS_UNDEFINED;

    *jse_post_action(htmldoc_js(d)) = POST_ACTION_LOCATION_HREF_SET;
    JSValue rv = JS_UNDEFINED;
    Err e = url_set_url_or_fragment(htmldoc_url(d), url);
    if (e) rv = JS_ThrowPlainError(ctx, "%s", e);

    JS_FreeCString(ctx, url);
    return rv;
}

static js_set__(location_set_href) { return replace_doc_url(ctx, this, val); }
static js_fn__(location_replace) {
    //TODO1: check error type
    if (argc != 1) return JS_ThrowTypeError(ctx, "invalid parameter to location replace");
    return replace_doc_url(ctx, this, argv[0]);
}


static const JSCFunctionListEntry location_fn_list[] = {
    JS_CGETSET_DEF("href", location_get_href, location_set_href),

    JS_CFUNC_DEF("replace", 1, location_replace),

    mk_not_impl_getset_list_entry(location, protocol),
    mk_not_impl_getset_list_entry(location, host),
    mk_not_impl_getset_list_entry(location, hostname),
    mk_not_impl_getset_list_entry(location, port),
    mk_not_impl_getset_list_entry(location, pathname),
    mk_not_impl_getset_list_entry(location, search),
    mk_not_impl_getset_list_entry(location, hash),
    mk_not_impl_getset_list_entry(location, origin),

    mk_not_impl_fn_list_entry(location, assign),
    mk_not_impl_fn_list_entry(location, reload),
    mk_not_impl_fn_list_entry(location, toString),

};
/** location */


/* ---- Navigator ---- */

mk_not_impl_getset(navigator, activeVRDisplays)
mk_not_impl_getset(navigator, appCodeName)
mk_not_impl_getset(navigator, appName)
mk_not_impl_getset(navigator, appVersion)
mk_not_impl_getset(navigator, audioSession)
mk_not_impl_getset(navigator, bluetooth)
mk_not_impl_getset(navigator, buildID)
mk_not_impl_getset(navigator, clipboard)
mk_not_impl_getset(navigator, connection)
mk_not_impl_getset(navigator, contacts)
mk_not_impl_getset(navigator, cookieEnabled)
mk_not_impl_getset(navigator, credentials)
mk_not_impl_getset(navigator, deviceMemory)
mk_not_impl_getset(navigator, devicePosture)
mk_not_impl_getset(navigator, doNotTrack)
mk_not_impl_getset(navigator, geolocation)
mk_not_impl_getset(navigator, globalPrivacyControl)
mk_not_impl_getset(navigator, gpu)
mk_not_impl_getset(navigator, hardwareConcurrency)
mk_not_impl_getset(navigator, hid)
mk_not_impl_getset(navigator, ink)
mk_not_impl_getset(navigator, keyboard)
mk_not_impl_getset(navigator, language)
mk_not_impl_getset(navigator, languages)
mk_not_impl_getset(navigator, locks)
mk_not_impl_getset(navigator, login)
mk_not_impl_getset(navigator, maxTouchPoints)
mk_not_impl_getset(navigator, mediaCapabilities)
mk_not_impl_getset(navigator, mediaDevices)
mk_not_impl_getset(navigator, mediaSession)
mk_not_impl_getset(navigator, mimeTypes)
mk_not_impl_getset(navigator, onLine)
mk_not_impl_getset(navigator, oscpu)
mk_not_impl_getset(navigator, pdfViewerEnabled)
mk_not_impl_getset(navigator, permissions)
mk_not_impl_getset(navigator, platform)
mk_not_impl_getset(navigator, plugins)
mk_not_impl_getset(navigator, preferences)
mk_not_impl_getset(navigator, presentation)
mk_not_impl_getset(navigator, product)
mk_not_impl_getset(navigator, productSub)
mk_not_impl_getset(navigator, scheduling)
mk_not_impl_getset(navigator, serial)
mk_not_impl_getset(navigator, serviceWorker)
mk_not_impl_getset(navigator, storage)
mk_not_impl_getset(navigator, usb)
mk_not_impl_getset(navigator, userActivation)
mk_not_impl_getset(navigator, userAgent)
mk_not_impl_getset(navigator, userAgentData)
mk_not_impl_getset(navigator, vendor)
mk_not_impl_getset(navigator, vendorSub)
mk_not_impl_getset(navigator, virtualKeyboard)
mk_not_impl_getset(navigator, wakeLock)
mk_not_impl_getset(navigator, webdriver)
mk_not_impl_getset(navigator, windowControlsOverlay)
mk_not_impl_getset(navigator, xr)

mk_not_impl_fn(navigator, canShare)
mk_not_impl_fn(navigator, clearAppBadge)
mk_not_impl_fn(navigator, getAutoplayPolicy)
mk_not_impl_fn(navigator, getBattery)
mk_not_impl_fn(navigator, getGamepads)
mk_not_impl_fn(navigator, getInstalledRelatedApps)
mk_not_impl_fn(navigator, registerProtocolHandler)
mk_not_impl_fn(navigator, requestMIDIAccess)
mk_not_impl_fn(navigator, requestMediaKeySystemAccess)
mk_not_impl_fn(navigator, setAppBadge)
mk_not_impl_fn(navigator, share)
mk_not_impl_fn(navigator, unregisterProtocolHandler)
mk_not_impl_fn(navigator, vibrate)

//TODO:
static js_fn__(navigator_sendBeacon) { (void)(ctx); (void)(this);(void)argc;(void)argv;
    return JS_NewBool(ctx, true);
}

static const JSCFunctionListEntry navigator_fn_list[] = {
    JS_CFUNC_DEF("sendBeacon", 0, navigator_sendBeacon),
    mk_not_impl_getset_list_entry(navigator, activeVRDisplays),
    mk_not_impl_getset_list_entry(navigator, appCodeName),
    mk_not_impl_getset_list_entry(navigator, appName),
    mk_not_impl_getset_list_entry(navigator, appVersion),
    mk_not_impl_getset_list_entry(navigator, audioSession),
    mk_not_impl_getset_list_entry(navigator, bluetooth),
    mk_not_impl_getset_list_entry(navigator, buildID),
    mk_not_impl_getset_list_entry(navigator, clipboard),
    mk_not_impl_getset_list_entry(navigator, connection),
    mk_not_impl_getset_list_entry(navigator, contacts),
    mk_not_impl_getset_list_entry(navigator, cookieEnabled),
    mk_not_impl_getset_list_entry(navigator, credentials),
    mk_not_impl_getset_list_entry(navigator, deviceMemory),
    mk_not_impl_getset_list_entry(navigator, devicePosture),
    mk_not_impl_getset_list_entry(navigator, doNotTrack),
    mk_not_impl_getset_list_entry(navigator, geolocation),
    mk_not_impl_getset_list_entry(navigator, globalPrivacyControl),
    mk_not_impl_getset_list_entry(navigator, gpu),
    mk_not_impl_getset_list_entry(navigator, hardwareConcurrency),
    mk_not_impl_getset_list_entry(navigator, hid),
    mk_not_impl_getset_list_entry(navigator, ink),
    mk_not_impl_getset_list_entry(navigator, keyboard),
    mk_not_impl_getset_list_entry(navigator, language),
    mk_not_impl_getset_list_entry(navigator, languages),
    mk_not_impl_getset_list_entry(navigator, locks),
    mk_not_impl_getset_list_entry(navigator, login),
    mk_not_impl_getset_list_entry(navigator, maxTouchPoints),
    mk_not_impl_getset_list_entry(navigator, mediaCapabilities),
    mk_not_impl_getset_list_entry(navigator, mediaDevices),
    mk_not_impl_getset_list_entry(navigator, mediaSession),
    mk_not_impl_getset_list_entry(navigator, mimeTypes),
    mk_not_impl_getset_list_entry(navigator, onLine),
    mk_not_impl_getset_list_entry(navigator, oscpu),
    mk_not_impl_getset_list_entry(navigator, pdfViewerEnabled),
    mk_not_impl_getset_list_entry(navigator, permissions),
    mk_not_impl_getset_list_entry(navigator, platform),
    mk_not_impl_getset_list_entry(navigator, plugins),
    mk_not_impl_getset_list_entry(navigator, preferences),
    mk_not_impl_getset_list_entry(navigator, presentation),
    mk_not_impl_getset_list_entry(navigator, product),
    mk_not_impl_getset_list_entry(navigator, productSub),
    mk_not_impl_getset_list_entry(navigator, scheduling),
    mk_not_impl_getset_list_entry(navigator, serial),
    mk_not_impl_getset_list_entry(navigator, serviceWorker),
    mk_not_impl_getset_list_entry(navigator, storage),
    mk_not_impl_getset_list_entry(navigator, usb),
    mk_not_impl_getset_list_entry(navigator, userActivation),
    mk_not_impl_getset_list_entry(navigator, userAgent),
    mk_not_impl_getset_list_entry(navigator, userAgentData),
    mk_not_impl_getset_list_entry(navigator, vendor),
    mk_not_impl_getset_list_entry(navigator, vendorSub),
    mk_not_impl_getset_list_entry(navigator, virtualKeyboard),
    mk_not_impl_getset_list_entry(navigator, wakeLock),
    mk_not_impl_getset_list_entry(navigator, webdriver),
    mk_not_impl_getset_list_entry(navigator, windowControlsOverlay),
    mk_not_impl_getset_list_entry(navigator, xr),

    mk_not_impl_fn_list_entry(navigator, canShare),
    mk_not_impl_fn_list_entry(navigator, clearAppBadge),
    mk_not_impl_fn_list_entry(navigator, getAutoplayPolicy),
    mk_not_impl_fn_list_entry(navigator, getBattery),
    mk_not_impl_fn_list_entry(navigator, getGamepads),
    mk_not_impl_fn_list_entry(navigator, getInstalledRelatedApps),
    mk_not_impl_fn_list_entry(navigator, registerProtocolHandler),
    mk_not_impl_fn_list_entry(navigator, requestMIDIAccess),
    mk_not_impl_fn_list_entry(navigator, requestMediaKeySystemAccess),
    mk_not_impl_fn_list_entry(navigator, setAppBadge),
    mk_not_impl_fn_list_entry(navigator, share),
    mk_not_impl_fn_list_entry(navigator, unregisterProtocolHandler),
    mk_not_impl_fn_list_entry(navigator, vibrate),

};
/** navigator */


/* ---- Window ---- */
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

mk_not_impl_getset(window, caches)
mk_not_impl_getset(window, clientInformation)
mk_not_impl_getset(window, closed)
mk_not_impl_getset(window, cookieStore)
mk_not_impl_getset(window, crashReport)
mk_not_impl_getset(window, credentialless)
mk_not_impl_getset(window, crossOriginIsolated)
mk_not_impl_getset(window, crypto)
mk_not_impl_getset(window, customElements)
mk_not_impl_getset(window, devicePixelRatio)
mk_not_impl_getset(window, documentPictureInPicture)
mk_not_impl_getset(window, event)
mk_not_impl_getset(window, external)
mk_not_impl_getset(window, fence)
mk_not_impl_getset(window, frameElement)
mk_not_impl_getset(window, frames)
mk_not_impl_getset(window, fullScreen)
mk_not_impl_getset(window, history)
mk_not_impl_getset(window, indexedDB)
mk_not_impl_getset(window, isSecureContext)
mk_not_impl_getset(window, launchQueue)
mk_not_impl_getset(window, length)
mk_not_impl_getset(window, localStorage)
mk_not_impl_getset(window, locationbar)
mk_not_impl_getset(window, menubar)
mk_not_impl_getset(window, mozInnerScreenX)
mk_not_impl_getset(window, mozInnerScreenY)
mk_not_impl_getset(window, name)
mk_not_impl_getset(window, navigation)
mk_not_impl_getset(window, opener)
mk_not_impl_getset(window, orientation)
mk_not_impl_getset(window, origin)
mk_not_impl_getset(window, originAgentCluster)
mk_not_impl_getset(window, outerHeight)
mk_not_impl_getset(window, outerWidth)
mk_not_impl_getset(window, pageXOffset)
mk_not_impl_getset(window, pageYOffset)
mk_not_impl_getset(window, personalbar)
mk_not_impl_getset(window, scheduler)
mk_not_impl_getset(window, screen)
mk_not_impl_getset(window, screenX)
mk_not_impl_getset(window, screenY)
mk_not_impl_getset(window, scrollMaxX)
mk_not_impl_getset(window, scrollMaxY)
mk_not_impl_getset(window, scrollX)
mk_not_impl_getset(window, scrollY)
mk_not_impl_getset(window, scrollbars)
mk_not_impl_getset(window, sessionStorage)
mk_not_impl_getset(window, sharedStorage)
mk_not_impl_getset(window, speechSynthesis)
mk_not_impl_getset(window, status)
mk_not_impl_getset(window, statusbar)
mk_not_impl_getset(window, toolbar)
mk_not_impl_getset(window, top)
/* mk_not_impl_getset(window, trustedTypes) */
mk_not_impl_getset(window, viewport)
mk_not_impl_getset(window, visualViewport)

mk_not_impl_fn(navigator, alert)
mk_not_impl_fn(navigator, atob)
mk_not_impl_fn(navigator, blur)
mk_not_impl_fn(navigator, btoa)
mk_not_impl_fn(navigator, cancelAnimationFrame)
mk_not_impl_fn(navigator, cancelIdleCallback)
mk_not_impl_fn(navigator, captureEvents)
mk_not_impl_fn(navigator, clearImmediate)
mk_not_impl_fn(navigator, clearInterval)
mk_not_impl_fn(navigator, clearTimeout)
mk_not_impl_fn(navigator, close)
mk_not_impl_fn(navigator, confirm)
mk_not_impl_fn(navigator, createImageBitmap)
mk_not_impl_fn(navigator, dump)
mk_not_impl_fn(navigator, fetch)
mk_not_impl_fn(navigator, fetchLater)
mk_not_impl_fn(navigator, find)
mk_not_impl_fn(navigator, focus)
mk_not_impl_fn(navigator, getComputedStyle)
mk_not_impl_fn(navigator, getDefaultComputedStyle)
mk_not_impl_fn(navigator, getScreenDetails)
mk_not_impl_fn(navigator, getSelection)
mk_not_impl_fn(navigator, matchMedia)
mk_not_impl_fn(navigator, moveBy)
mk_not_impl_fn(navigator, moveTo)
mk_not_impl_fn(navigator, open)
mk_not_impl_fn(navigator, postMessage)
mk_not_impl_fn(navigator, print)
mk_not_impl_fn(navigator, prompt)
mk_not_impl_fn(navigator, queryLocalFonts)
mk_not_impl_fn(navigator, releaseEvents)
mk_not_impl_fn(navigator, reportError)
mk_not_impl_fn(navigator, requestAnimationFrame)
mk_not_impl_fn(navigator, requestFileSystem)
mk_not_impl_fn(navigator, requestIdleCallback)
mk_not_impl_fn(navigator, resizeBy)
mk_not_impl_fn(navigator, resizeTo)
mk_not_impl_fn(navigator, scroll)
mk_not_impl_fn(navigator, scrollBy)
mk_not_impl_fn(navigator, scrollByLines)
mk_not_impl_fn(navigator, scrollByPages)
mk_not_impl_fn(navigator, scrollTo)
mk_not_impl_fn(navigator, setImmediate)
mk_not_impl_fn(navigator, setInterval)
mk_not_impl_fn(navigator, setResizable)
mk_not_impl_fn(navigator, showDirectoryPicker)
mk_not_impl_fn(navigator, showOpenFilePicker)
mk_not_impl_fn(navigator, showSaveFilePicker)
mk_not_impl_fn(navigator, sizeToContent)
mk_not_impl_fn(navigator, stop)
mk_not_impl_fn(navigator, structuredClone)
mk_not_impl_fn(navigator, webkitConvertPointFromNodeToPage)
mk_not_impl_fn(navigator, webkitConvertPointFromPageToNode)


static js_get__(window_innerWidth) { (void)this; return JS_NewFloat64(ctx, *session_ncols(JS_GetContextOpaque(ctx))); }
static js_get__(window_innerHeight) { (void)this; return JS_NewFloat64(ctx, *session_nrows(JS_GetContextOpaque(ctx))); }

static js_get__(window_trustedTypes) { 
    (void)this;
    tryjs(console_log_not_implemented(ctx));
    return JS_UNDEFINED;
}

mk_not_impl_setter(window_parent)
static js_get__(window_get_parent) { return JS_DupValue(ctx, this); }

mk_not_impl_setter(window_trustedTypes)
//TODO0: define functions instead of window methods?
//TODO1: inherit from Event Target
static const JSCFunctionListEntry window_fn_list[] = {
    JS_CGETSET_DEF("innerWidth",   window_innerWidth,   js_set_noop),
    JS_CGETSET_DEF("innerHeight",  window_innerHeight,  js_set_noop),
    JS_CGETSET_DEF("trustedTypes", window_trustedTypes, jse_setter_not_implemented_window_trustedTypes),
    JS_CGETSET_DEF("parent",       window_get_parent,   jse_setter_not_implemented_window_parent),

    mk_not_impl_getset_list_entry(window, caches),
    mk_not_impl_getset_list_entry(window, clientInformation),
    mk_not_impl_getset_list_entry(window, closed),
    mk_not_impl_getset_list_entry(window, cookieStore),
    mk_not_impl_getset_list_entry(window, crashReport),
    mk_not_impl_getset_list_entry(window, credentialless),
    mk_not_impl_getset_list_entry(window, crossOriginIsolated),
    mk_not_impl_getset_list_entry(window, crypto),
    mk_not_impl_getset_list_entry(window, customElements),
    mk_not_impl_getset_list_entry(window, devicePixelRatio),
    mk_not_impl_getset_list_entry(window, documentPictureInPicture),
    mk_not_impl_getset_list_entry(window, event),
    mk_not_impl_getset_list_entry(window, external),
    mk_not_impl_getset_list_entry(window, fence),
    mk_not_impl_getset_list_entry(window, frameElement),
    mk_not_impl_getset_list_entry(window, frames),
    mk_not_impl_getset_list_entry(window, fullScreen),
    mk_not_impl_getset_list_entry(window, history),
    mk_not_impl_getset_list_entry(window, indexedDB),
    mk_not_impl_getset_list_entry(window, isSecureContext),
    mk_not_impl_getset_list_entry(window, launchQueue),
    mk_not_impl_getset_list_entry(window, length),
    mk_not_impl_getset_list_entry(window, localStorage),
    mk_not_impl_getset_list_entry(window, locationbar),
    mk_not_impl_getset_list_entry(window, menubar),
    mk_not_impl_getset_list_entry(window, mozInnerScreenX),
    mk_not_impl_getset_list_entry(window, mozInnerScreenY),
    mk_not_impl_getset_list_entry(window, name),
    mk_not_impl_getset_list_entry(window, navigation),
    mk_not_impl_getset_list_entry(window, opener),
    mk_not_impl_getset_list_entry(window, orientation),
    mk_not_impl_getset_list_entry(window, origin),
    mk_not_impl_getset_list_entry(window, originAgentCluster),
    mk_not_impl_getset_list_entry(window, outerHeight),
    mk_not_impl_getset_list_entry(window, outerWidth),
    mk_not_impl_getset_list_entry(window, pageXOffset),
    mk_not_impl_getset_list_entry(window, pageYOffset),
    /* mk_not_impl_getset_list_entry(window, parent), */
    mk_not_impl_getset_list_entry(window, personalbar),
    mk_not_impl_getset_list_entry(window, scheduler),
    mk_not_impl_getset_list_entry(window, screen),
    mk_not_impl_getset_list_entry(window, screenX),
    mk_not_impl_getset_list_entry(window, screenY),
    mk_not_impl_getset_list_entry(window, scrollMaxX),
    mk_not_impl_getset_list_entry(window, scrollMaxY),
    mk_not_impl_getset_list_entry(window, scrollX),
    mk_not_impl_getset_list_entry(window, scrollY),
    mk_not_impl_getset_list_entry(window, scrollbars),
    mk_not_impl_getset_list_entry(window, sessionStorage),
    mk_not_impl_getset_list_entry(window, sharedStorage),
    mk_not_impl_getset_list_entry(window, speechSynthesis),
    mk_not_impl_getset_list_entry(window, status),
    mk_not_impl_getset_list_entry(window, statusbar),
    mk_not_impl_getset_list_entry(window, toolbar),
    mk_not_impl_getset_list_entry(window, top),
    /* mk_not_impl_getset_list_entry(window, trustedTypes), */
    mk_not_impl_getset_list_entry(window, viewport),
    mk_not_impl_getset_list_entry(window, visualViewport),

    mk_not_impl_fn_list_entry(navigator, alert),
    mk_not_impl_fn_list_entry(navigator, atob),
    mk_not_impl_fn_list_entry(navigator, blur),
    mk_not_impl_fn_list_entry(navigator, btoa),
    mk_not_impl_fn_list_entry(navigator, cancelAnimationFrame),
    mk_not_impl_fn_list_entry(navigator, cancelIdleCallback),
    mk_not_impl_fn_list_entry(navigator, captureEvents),
    mk_not_impl_fn_list_entry(navigator, clearImmediate),
    mk_not_impl_fn_list_entry(navigator, clearInterval),
    mk_not_impl_fn_list_entry(navigator, clearTimeout),
    mk_not_impl_fn_list_entry(navigator, close),
    mk_not_impl_fn_list_entry(navigator, confirm),
    mk_not_impl_fn_list_entry(navigator, createImageBitmap),
    mk_not_impl_fn_list_entry(navigator, dump),
    mk_not_impl_fn_list_entry(navigator, fetch),
    mk_not_impl_fn_list_entry(navigator, fetchLater),
    mk_not_impl_fn_list_entry(navigator, find),
    mk_not_impl_fn_list_entry(navigator, focus),
    mk_not_impl_fn_list_entry(navigator, getComputedStyle),
    mk_not_impl_fn_list_entry(navigator, getDefaultComputedStyle),
    mk_not_impl_fn_list_entry(navigator, getScreenDetails),
    mk_not_impl_fn_list_entry(navigator, getSelection),
    mk_not_impl_fn_list_entry(navigator, matchMedia),
    mk_not_impl_fn_list_entry(navigator, moveBy),
    mk_not_impl_fn_list_entry(navigator, moveTo),
    mk_not_impl_fn_list_entry(navigator, open),
    mk_not_impl_fn_list_entry(navigator, postMessage),
    mk_not_impl_fn_list_entry(navigator, print),
    mk_not_impl_fn_list_entry(navigator, prompt),
    mk_not_impl_fn_list_entry(navigator, queryLocalFonts),
    mk_not_impl_fn_list_entry(navigator, releaseEvents),
    mk_not_impl_fn_list_entry(navigator, reportError),
    mk_not_impl_fn_list_entry(navigator, requestAnimationFrame),
    mk_not_impl_fn_list_entry(navigator, requestFileSystem),
    mk_not_impl_fn_list_entry(navigator, requestIdleCallback),
    mk_not_impl_fn_list_entry(navigator, resizeBy),
    mk_not_impl_fn_list_entry(navigator, resizeTo),
    mk_not_impl_fn_list_entry(navigator, scroll),
    mk_not_impl_fn_list_entry(navigator, scrollBy),
    mk_not_impl_fn_list_entry(navigator, scrollByLines),
    mk_not_impl_fn_list_entry(navigator, scrollByPages),
    mk_not_impl_fn_list_entry(navigator, scrollTo),
    mk_not_impl_fn_list_entry(navigator, setImmediate),
    mk_not_impl_fn_list_entry(navigator, setInterval),
    mk_not_impl_fn_list_entry(navigator, setResizable),
    mk_not_impl_fn_list_entry(navigator, showDirectoryPicker),
    mk_not_impl_fn_list_entry(navigator, showOpenFilePicker),
    mk_not_impl_fn_list_entry(navigator, showSaveFilePicker),
    mk_not_impl_fn_list_entry(navigator, sizeToContent),
    mk_not_impl_fn_list_entry(navigator, stop),
    mk_not_impl_fn_list_entry(navigator, structuredClone),
    mk_not_impl_fn_list_entry(navigator, webkitConvertPointFromNodeToPage),
    mk_not_impl_fn_list_entry(navigator, webkitConvertPointFromPageToNode),

};


static const JSCFunctionListEntry global_fn_list[] = {
    JS_CFUNC_DEF("setTimeout", 2, setTimeout),
};


/* ---- Storage ---- */

mk_not_impl_getset(storage, length)

mk_not_impl_fn(storage, getItem)
mk_not_impl_fn(storage, setItem)
mk_not_impl_fn(storage, removeItem)
mk_not_impl_fn(storage, clear)
mk_not_impl_fn(storage, key)

static const JSCFunctionListEntry storage_fn_list[] = {
    mk_not_impl_getset_list_entry(storage, length),

    mk_not_impl_fn_list_entry(storage, getItem),
    mk_not_impl_fn_list_entry(storage, setItem),
    mk_not_impl_fn_list_entry(storage, removeItem),
    mk_not_impl_fn_list_entry(storage, clear),
    mk_not_impl_fn_list_entry(storage, key),

};


/** storage */

/* ---- Performance ---- */
mk_not_impl_getset(Performance, eventCounts)
mk_not_impl_getset(Performance, interactionCount)
mk_not_impl_getset(Performance, navigation)
mk_not_impl_getset(Performance, timing)
mk_not_impl_getset(Performance, memory)
mk_not_impl_getset(Performance, timeOrigin)

mk_not_impl_fn(Performance, clearMarks)
mk_not_impl_fn(Performance, clearMeasures)
mk_not_impl_fn(Performance, clearResourceTimings)
mk_not_impl_fn(Performance, getEntries)
mk_not_impl_fn(Performance, getEntriesByName)
mk_not_impl_fn(Performance, getEntriesByType)
mk_not_impl_fn(Performance, mark)
mk_not_impl_fn(Performance, measure)
mk_not_impl_fn(Performance, measureUserAgentSpecificMemory)
mk_not_impl_fn(Performance, setResourceTimingBufferSize)
mk_not_impl_fn(Performance, toJSON)

static js_fn__(performance_now) //TODO:
{ (void)(ctx); (void)(this);(void)argc;(void)argv;
    return JS_NewFloat64(ctx, 434.1209f);
}

static const JSCFunctionListEntry performance_fn_list[] = {

    JS_CFUNC_DEF("now", 0, performance_now),

    mk_not_impl_getset_list_entry(Performance, eventCounts),
    mk_not_impl_getset_list_entry(Performance, interactionCount),
    mk_not_impl_getset_list_entry(Performance, navigation),
    mk_not_impl_getset_list_entry(Performance, timing),
    mk_not_impl_getset_list_entry(Performance, memory),
    mk_not_impl_getset_list_entry(Performance, timeOrigin),

    mk_not_impl_fn_list_entry(Performance, clearMarks),
    mk_not_impl_fn_list_entry(Performance, clearMeasures),
    mk_not_impl_fn_list_entry(Performance, clearResourceTimings),
    mk_not_impl_fn_list_entry(Performance, getEntries),
    mk_not_impl_fn_list_entry(Performance, getEntriesByName),
    mk_not_impl_fn_list_entry(Performance, getEntriesByType),
    mk_not_impl_fn_list_entry(Performance, mark),
    mk_not_impl_fn_list_entry(Performance, measure),
    mk_not_impl_fn_list_entry(Performance, measureUserAgentSpecificMemory),
    mk_not_impl_fn_list_entry(Performance, setResourceTimingBufferSize),
    mk_not_impl_fn_list_entry(Performance, toJSON),

};
/** performance */


/* ---- URLSearchParams ---- */
#define T1 Str
#define T2 Str
#define T1Clean str_clean
#define T2Clean str_clean
#define T1Cpy str_append
#define T2Cpy str_append
#include <pair.h>

typedef PairOf(Str,Str) UrlParam;
static int url_param_cmp(const void* X, const void* Y){
    const UrlParam* x = X;
    const UrlParam* y = Y;
    return str_cmp(&x->fst, &y->fst);
}

static int url_param_copy(UrlParam x[_1_], const UrlParam y[_1_]) {
    *x = (UrlParam){0};
    if (str_append(&x->fst, y->fst)) return -1;
    if (y->snd.len && str_append(&x->snd, y->snd)) return -1;
    return 0;
}
#define T UrlParam
#define TCpy url_param_copy
#define TCmp url_param_cmp
#define TClean pairfn(Str,Str,clean)
#include <arl.h>

typedef ArlOf(UrlParam) URLSearchParams;

static void
url_search_params_finalizer(JSRuntime *rt, JSValue val) {
    URLSearchParams* data = JS_GetOpaque(val, URLSearchParams_class_id);
    if (!data) return;
    arlfn(UrlParam,clean)(data);
    js_free_rt(rt, data);
}

#define _str_from_view_(View) (Str){.items=(char*)(View).items, .len=(View).len}
static Err
url_search_params_append_impl(URLSearchParams data[_1_], StrView name, StrView value) {
    UrlParam* pair = &(UrlParam){
        .fst=_str_from_view_(name),
        .snd=_str_from_view_(value),
    };
    if (!arlfn(UrlParam,append)(data, pair))
        fail_e("arl append failure");
    return Ok;
}

static js_fn__(url_search_params_append) {
    if (argc < 2) return JS_ThrowTypeError(ctx, "URLSearchParams.append requires name and value");
    URLSearchParams* data = JS_GetOpaque(this, URLSearchParams_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid URLSearchParams object");
    size_t name_len, value_len;
    const char *name = JS_ToCStringLen(ctx, &name_len, argv[0]);
    const char *value = JS_ToCStringLen(ctx, &value_len, argv[1]);
    if (!name || !value) {
        JS_FreeCString(ctx, name);
        JS_FreeCString(ctx, value);
        return JS_EXCEPTION;
    }
    //TODO0: manage error
    url_search_params_append_impl(data, sv(name, name_len), sv(value, value_len));
    JS_FreeCString(ctx, name);
    JS_FreeCString(ctx, value);
    return JS_UNDEFINED;
}


static void
url_search_params_delete_impl(URLSearchParams data[_1_], StrView name)
{
    arlfn(UrlParam,remove)(data, &(UrlParam){.fst=_str_from_view_(name)});
}
static js_fn__(url_search_params_delete) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "delete requires name");
    URLSearchParams* data = JS_GetOpaque(this, URLSearchParams_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid URLSearchParams object");
    size_t name_len;
    const char *name = JS_ToCStringLen(ctx, &name_len, argv[0]);
    if (!name) return JS_EXCEPTION;
    url_search_params_delete_impl(data, sv(name, name_len));
    JS_FreeCString(ctx, name);
    return JS_UNDEFINED;
}


static StrView
url_search_params_get_first(URLSearchParams *data, StrView name)
{
    UrlParam* p = arlfn(UrlParam,find)(data, &(UrlParam){.fst=_str_from_view_(name)});
    if (p) return sv(p->snd);
    return (StrView){0};
}


static void url_search_params_set_impl(URLSearchParams data[_1_], StrView name, StrView value)
{
    url_search_params_delete_impl(data, name);
    url_search_params_append_impl(data, name, value);
}
static js_fn__(url_search_params_set)
{
    if (argc < 2) return JS_ThrowTypeError(ctx, "set requires name and value");
    URLSearchParams* data = JS_GetOpaque(this, URLSearchParams_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid URLSearchParams object");
    size_t name_len, value_len;
    const char *name = JS_ToCStringLen(ctx, &name_len, argv[0]);
    const char *value = JS_ToCStringLen(ctx, &value_len, argv[1]);
    if (!name || !value) {
        JS_FreeCString(ctx, name);
        JS_FreeCString(ctx, value);
        return JS_EXCEPTION;
    }
    url_search_params_set_impl(data, sv(name, name_len), sv(value, value_len));
    JS_FreeCString(ctx, name);
    JS_FreeCString(ctx, value);
    return JS_UNDEFINED;
}

static js_fn__(url_search_params_get)
{
    if (argc < 1) return JS_ThrowTypeError(ctx, "get requires name");
    URLSearchParams *data = JS_GetOpaque(this, URLSearchParams_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid URLSearchParams object");
    size_t name_len;
    const char *name = JS_ToCStringLen(ctx, &name_len, argv[0]);
    if (!name) return JS_EXCEPTION;
    StrView value = url_search_params_get_first(data, sv(name, name_len));
    JS_FreeCString(ctx, name);
    if (!value.items) return JS_NULL;
    return JS_NewStringLen(ctx, value.items, value.len);
}


static js_fn__(url_search_params_has)
{
    if (argc < 1) return JS_ThrowTypeError(ctx, "has requires name");

    URLSearchParams *data = JS_GetOpaque(this, URLSearchParams_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid URLSearchParams object");

    size_t name_len;
    const char* name = JS_ToCStringLen(ctx, &name_len, argv[0]);
    if (!name) return JS_EXCEPTION;

    bool it_has = false;
    foreach__(UrlParam,data,it)
        if (str_eq_case(it->fst, sv(name,name_len))) { it_has = true; goto Clean; }
Clean:
    JS_FreeCString(ctx, name);
    return JS_NewBool(ctx, it_has);
}


static js_fn__(url_search_params_sort)
{
    (void)argc; (void)argv;
    URLSearchParams *data = JS_GetOpaque(this, URLSearchParams_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid URLSearchParams object");
    qsort(items__(data), len__(data), sizeof(*items__(data)), url_param_cmp);
    return JS_UNDEFINED;
}

static js_fn__(url_search_params_toString)
{
    (void)argc; (void)argv;
    URLSearchParams* data = JS_GetOpaque(this, URLSearchParams_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid URLSearchParams object");
    Str result = {0};
    foreach__(UrlParam, data, it) {
        //TODO1: manage str_append error
        str_append(&result, "&");
        str_append(&result, it->fst);
        str_append(&result, "=");
        str_append(&result, it->snd);
    }

    JSValue ret = JS_UNDEFINED;
    if (!result.len) ret = JS_NewString(ctx, "");
    else ret = JS_NewStringLen(ctx, result.items + 1, result.len - 1);
    str_clean(&result);
    return ret;
}


static js_get__(url_search_params_size)
{
    URLSearchParams* data = JS_GetOpaque(this, URLSearchParams_class_id);
    if (!data) return JS_ThrowTypeError(ctx, "invalid URLSearchParams object");
    return JS_NewInt32(ctx, len__(data));
}
/* static void url_search_params_get_all(URLSearchParams *data, StrView name, ArlOf(Str) *out) */
/* { */
/*     for (size_t i = 0; i < arl_len(&data->names); i++) { */
/*         if (str_eq_view(&data->names.items[i], name)) { */
/*             Str copy; */
/*             str_init_from_view(&copy, str_view(&data->values.items[i])); */
/*             arl_append(out, copy); */
/*         } */
/*     } */
/* } */

static Err
url_search_params_parse_string(URLSearchParams p[_1_], StrView s) {
    while (s.len) {
        StrView kv = strview_split(&s, '&');
        StrView key = strview_split(&kv, '=');
        if (!key.len) continue;
        try(url_search_params_append_impl(p, key, kv));
    }
    return Ok;
}


static js_fn__(url_search_params_ctor)
{
    (void)this;(void)argv;
    JSValue          rv   = JS_UNDEFINED;
    URLSearchParams* data = js_mallocz(ctx, sizeof(URLSearchParams));
    if (!data) return JS_ThrowOutOfMemory(ctx);
    *data = (URLSearchParams){0};

    JSValue proto = JS_GetPropertyStr(ctx, this, "prototype");
    if (JS_IsException(proto)) {
        rv = proto;
        goto Fail;
    }
    rv = JS_NewObjectProtoClass(ctx, proto, URLSearchParams_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(rv))
        goto Fail;

    JS_SetOpaque(rv, data);

    if (argc == 0) return rv;
    if (JS_IsString(argv[0])) {
        size_t len;
        const char *str = JS_ToCStringLen(ctx, &len, argv[0]);
        if (!str) {
            rv = JS_EXCEPTION;
            goto Fail;
        }
        url_search_params_parse_string(data, sv(str, len));
        JS_FreeCString(ctx, str);
    }
    return rv;

    rv = JS_ThrowPlainError(ctx, "URLSearchParams ctor does not implement params"); 
Fail:
    js_free(ctx, data);
    return rv;

}


mk_not_impl_fn(URLSearchParams, getAll)
mk_not_impl_fn(URLSearchParams, entries)
mk_not_impl_fn(URLSearchParams, keys)
mk_not_impl_fn(URLSearchParams, values)

static const JSCFunctionListEntry URLSearchParams_fn_list[] = {
    JS_CFUNC_DEF("append", 2, url_search_params_append),
    JS_CFUNC_DEF("delete", 1, url_search_params_delete),
    JS_CFUNC_DEF("get", 1, url_search_params_get),
    JS_CFUNC_DEF("has", 1, url_search_params_has),
    JS_CFUNC_DEF("set", 2, url_search_params_set),
    JS_CFUNC_DEF("sort", 0, url_search_params_sort),
    JS_CFUNC_DEF("toString", 0, url_search_params_toString),
    JS_CGETSET_DEF("size", url_search_params_size, NULL),

    /* JS_CFUNC_DEF("entries", 0, url_search_params_entries), */
    /* JS_CFUNC_DEF("keys", 0, url_search_params_keys), */
    /* JS_CFUNC_DEF("values", 0, url_search_params_values), */
    /* JS_CFUNC_DEF("getAll", 1, url_search_params_getAll), */
    mk_not_impl_fn_list_entry(URLSearchParams, getAll),
    mk_not_impl_fn_list_entry(URLSearchParams, entries),
    mk_not_impl_fn_list_entry(URLSearchParams, keys),
    mk_not_impl_fn_list_entry(URLSearchParams, values),
};


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


Err
jse_init(Session* session, HtmlDoc* htmldoc) {
    Err e = Ok;

    if (!htmldoc) return err_internal("no HtmlDoc");
    if (!session) return err_internal("no Session");

    JsEngine* js = htmldoc_js(htmldoc);
    if (!js) return err_internal("no JsEngine in HtmlDoc");

    js->rt = JS_NewRuntime();
    if (!js->rt) err_internal("could not initialize quickjs runtime");

    js->ctx = JS_NewContext(js->rt);
    if (!js->ctx) { e=err_internal("could not initialize quickjs runtime"); goto Fail; }


    JS_SetContextOpaque(js->ctx, session);

    JSValue global = JS_GetGlobalObject(js->ctx);

    try(set_property_str(js->ctx, global, "window",     JS_DupValue(js->ctx, global)));
    try(set_property_str(js->ctx, global, "self",       JS_DupValue(js->ctx, global)));
    try(set_property_str(js->ctx, global, "top",        JS_DupValue(js->ctx, global)));
    try(set_property_str(js->ctx, global, "parent",     JS_DupValue(js->ctx, global)));
    try(set_property_str(js->ctx, global, "globalThis", JS_DupValue(js->ctx, global)));

    tryjmp(e, Fail, init_js_class(&mk_jsclass(node, NULL), js->ctx));
    tryjmp(e, Fail, init_js_class(&mk_jsclass(element, &node_class_id), js->ctx));
    tryjmp(e, Fail, init_js_class(&mk_jsclass(storage, NULL), js->ctx));
    tryjmp(e, Fail,
        init_js_class(
            &mk_jsclass_ctor_fin(URLSearchParams, NULL, url_search_params_ctor, url_search_params_finalizer), js->ctx));

    tryjmp(e, Fail, init_js_class(&mk_jsclass(console, NULL), js->ctx));
    tryjmp(e, Fail, init_js_class(&mk_jsclass(document, NULL), js->ctx));
    tryjmp(e, Fail, init_js_class(&mk_jsclass(location, NULL), js->ctx));
    tryjmp(e, Fail, init_js_class(&mk_jsclass(navigator, NULL), js->ctx));
    tryjmp(e, Fail, init_js_class(&mk_jsclass(performance, NULL), js->ctx));

    tryjmp(e,Fail, singleton_add(console, global, js->ctx, jse_consolebuf(htmldoc_js(htmldoc))));
    tryjmp(e,Fail, singleton_add(document, global, js->ctx, htmldoc));
    tryjmp(e,Fail, singleton_add(location, global, js->ctx, htmldoc));
    tryjmp(e,Fail, singleton_add(navigator, global, js->ctx, htmldoc));
    tryjmp(e,Fail, singleton_add(performance, global, js->ctx, htmldoc));


    tryjmp(e,Fail, set_property_fn_list(js->ctx, global, global_fn_list));
    tryjmp(e,Fail, set_property_fn_list(js->ctx, global, window_fn_list));

    JS_FreeValue(js->ctx, global);
    return Ok;
Fail:
    JS_FreeValue(js->ctx, global);

    jse_clean(js);
    return e;
}



#else /* quickjs disabled: */
typedef int _quickjs_disabled_;
#endif
