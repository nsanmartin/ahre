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


#define validate_jsv(Value) _Generic((Value), JSValue: Value)
#define tryjs(Expr) do{\
    Err ahre_jsv_=validate_jsv((Expr));if (JS_IsException(ahre_jsv_)) return ahre_jsv_;}while(0) 

#define js_err_not_impl JS_ThrowPlainError(ctx, "Ahre jse: fn not implemented " file_line__ );

#define err_jse(Msg) err_fmt("error jse: %s %s\n", Msg, file_line__)

#define js_fn__(FnName) JSValue FnName(JSContext* ctx, JSValueConst this, int argc, JSValueConst *argv)
#define js_fn_n__(FnName, ...) JSValue FnName(JSContext* ctx, JSValueConst this, int argc, JSValueConst *argv, __VA_ARGS__)
#define js_get__(FnName) JSValue FnName(JSContext *ctx, JSValueConst this)
#define js_get_n__(FnName, ...) JSValue FnName(JSContext *ctx, JSValueConst this, __VA_ARGS__)
#define js_set__(FnName) JSValue FnName(JSContext* ctx, JSValueConst this, JSValueConst val)
#define js_set_n__(FnName, ...) JSValue FnName(JSContext* ctx, JSValueConst this, JSValueConst val, __VA_ARGS__)


typedef struct {
    const char* name;
    JSClassID class_id[1];
    JSCFunctionListEntry fn_list[];
} JsClass;

static JSClassID console_class_id   = 0;
static JSClassID node_class_id      = 0;
static JSClassID element_class_id   = 0;
/* static JSClassID dom_token_list_class_id = 0; */
/* static JSClassID classList_class_id = 0; */
static JSClassID document_class_id  = 0;
static JSClassID window_class_id  = 0;
/* static JSClassID location_class_id  = 0; */
/* static JSClassID navigator_class_id = 0; */
/* static JSClassID storage_class_id   = 0; */

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


#define set_property_str(Ctx, Obj, Name, Value) (\
     -1 == JS_SetPropertyStr(Ctx, Obj, Name, Value) ? err_fmt("ahjs error: could not set propery (in %s)", __func__) : Ok)

#define set_property_fn_list(Ctx, Obj, Arr) (\
    JS_SetPropertyFunctionList(Ctx, Obj, Arr, sizeof(Arr)/sizeof(*Arr)) == 0 \
    ? Ok : err_fmt("ahjs error: could not set property fn list (in %s)",  __func__ ))


/* ---- Console ---- */


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

static const JSCFunctionListEntry console_fn_list[] = {
    JS_CFUNC_DEF("assert",           0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("clear",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("count",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("countReset",       0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("debug",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("dir",              0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("dirxml",           0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("error",            1,     console_error),
    JS_CFUNC_DEF("exception",        0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("group",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("groupCollapsed",   0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("groupEnd",         0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("info",             0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("log",              1,     console_log),
    JS_CFUNC_DEF("profile",          0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("profileEnd",       0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("table",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("time",             0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("timeEnd",          0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("timeLog",          0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("timeStamp",        0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("trace",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("warn",             1,     console_warn),
};



static Err
console_init(JSValue console[_1_], JSContext* ctx, HtmlDoc d[_1_]) {
    try(init_class(&console_class_id, &(JSClassDef){ "Console", .finalizer=NULL }, ctx));
    try(init_instance(console, console_class_id, ctx));
    try(set_property_fn_list(ctx, *console, console_fn_list));
    Str* buf = jse_consolebuf(htmldoc_js(d));
    if (JS_SetOpaque(*console, buf)) err_jse("could not set the console buffer");
    return Ok;
}

static Err
global_add_console(JSValue global, HtmlDoc d[_1_], JSContext* ctx) {
    JSValue console   = JS_UNDEFINED;
    try( console_init(&console, ctx, d));
    try( set_property_str(ctx, global, "console", console));
    return Ok;
}
/** console **/


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


/* ---- Node ---- */


static js_set_n__(_set_textContent_impl, DomElem elem) {
    (void)this;
    if (isnull(elem)) return JS_ThrowTypeError(ctx, "ahre: cannot set textContext of NULL");
    size_t len;
    const char* new_content = JS_ToCStringLen(ctx, &len, val);

    Err e = dom_elem_set_text_content(elem, sv(new_content, len));
    if (e) return JS_ThrowTypeError(ctx, e);
    return JS_UNDEFINED;
}
       

static js_set__(_set_textContent)
{
    DomElem elem = js_value_to_dom_elem(this);
    return _set_textContent_impl(ctx, this, val, elem);
}


static const JSCFunctionListEntry node_fn_list[] = {
    JS_CGETSET_DEF("baseURI",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("childNodes",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("firstChild",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("isConnected",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("lastChild",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("nextSibling",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("nodeName",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("nodeType",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("nodeValue",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ownerDocument",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("parentNode",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("parentElement",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("previousSibling",        jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("textContent",            jse_get_not_implemented, _set_textContent),
    JS_CFUNC_DEF("appendChild",              0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("cloneNode",                0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("compareDocumentPosition",  0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("contains",                 0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getRootNode",              0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("hasChildNodes",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("insertBefore",             0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("isDefaultNamespace",       0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("isEqualNode",              0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("isSameNode",               0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("lookupPrefix",             0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("lookupNamespaceURI",       0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("normalize",                0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("removeChild",              0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("replaceChild",             0,     jse_fn_not_implemented)
};

static Err
node_class_init(JSContext* ctx) {
    try(init_class(&node_class_id, &(JSClassDef){ "Node", .finalizer=NULL }, ctx));
    JSValue node_proto = JS_NewObject(ctx);
    try(set_property_fn_list(ctx, node_proto, node_fn_list));
    JS_SetClassProto(ctx, node_class_id, node_proto);
    return Ok;
}


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
    if (e) return JS_ThrowTypeError(ctx, e);
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
    if (e) return JS_ThrowTypeError(ctx, e);
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


static const JSCFunctionListEntry element_fn_list[] = {

    JS_CGETSET_DEF("assignedSlot",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("attributes",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("childElementCount",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("children",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("classList",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("className",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("clientHeight",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("clientLeft",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("clientTop",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("clientWidth",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("currentCSSZoom",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("customElementRegistry",      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("elementTiming",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("firstElementChild",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("id",                         element_get_id, element_set_id),
    JS_CGETSET_DEF("innerHTML",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("lastElementChild",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("localName",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("namespaceURI",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("nextElementSibling",         jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("outerHTML",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("part",                       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("prefix",                     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("previousElementSibling",     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollHeight",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollLeft",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollLeftMax",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollTop",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollTopMax",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollWidth",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("shadowRoot",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("slot",                       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("tagName",                    jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaAtomic",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaAutoComplete",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaBrailleLabel",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaBrailleRoleDescription", jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaBusy",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaChecked",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaColCount",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaColIndex",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaColIndexText",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaColSpan",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaCurrent",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaDescription",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaDisabled",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaExpanded",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaHasPopup",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaHidden",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaInvalid",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaKeyShortcuts",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaLabel",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaLevel",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaLive",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaModal",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaMultiline",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaMultiSelectable",        jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaOrientation",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaPlaceholder",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaPosInSet",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaPressed",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaReadOnly",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaRelevant",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaRequired",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaRoleDescription",        jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaRowCount",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaRowIndex",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaRowIndexText",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaRowSpan",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaSelected",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaSetSize",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaSort",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaValueMax",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaValueMin",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaValueNow",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaValueText",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("role",                       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaActiveDescendantElement",jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaControlsElements",       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaDescribedByElements",    jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaDetailsElements",        jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaErrorMessageElements",   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaFlowToElements",         jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaLabelledByElements",     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("ariaOwnsElements",           jse_get_not_implemented, jse_set_not_implemented),

    JS_CFUNC_DEF("after",                    0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("animate",                  0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("append",                   0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("ariaNotify",               0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("attachShadow",             0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("before",                   0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("checkVisibility",          0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("closest",                  0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("computedStyleMap",         0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getAnimations",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getAttribute",             1,     element_getAttribute),
    JS_CFUNC_DEF("getAttributeNS",           0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getAttributeNames",        0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getAttributeNode",         0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getAttributeNodeNS",       0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getBoundingClientRect",    0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getClientRects",           0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getElementsByClassName",   0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getElementsByTagName",     0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getElementsByTagNameNS",   0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("getHTML",                  0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("hasAttribute",             0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("hasAttributeNS",           0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("hasAttributes",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("hasPointerCapture",        0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("insertAdjacentElement",    0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("insertAdjacentHTML",       0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("insertAdjacentText",       0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("matches",                  0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("moveBefore",               0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("prepend",                  0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("querySelector",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("querySelectorAll",         0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("releasePointerCapture",    0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("remove",                   0,     element_remove),
    JS_CFUNC_DEF("removeAttribute",          1,     element_removeAttribute),
    JS_CFUNC_DEF("removeAttributeNS",        0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("removeAttributeNode",      0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("replaceChildren",          0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("replaceWith",              0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("requestFullscreen",        0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("requestPointerLock",       0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("scroll",                   0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("scrollBy",                 0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("scrollIntoView",           0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("scrollIntoViewIfNeeded",   0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("scrollTo",                 0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("setAttribute",             2,     element_setAttribute),
    JS_CFUNC_DEF("setAttributeNS",           0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("setAttributeNode",         0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("setAttributeNodeNS",       0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("setCapture",               0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("setHTML",                  0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("setHTMLUnsafe",            0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("setPointerCapture",        0,     jse_fn_not_implemented),
    JS_CFUNC_DEF("toggleAttribute",          0,     jse_fn_not_implemented)
};


static Err
element_class_init(JSContext* ctx) {
    try(init_class(&element_class_id, &(JSClassDef){ "Element", .finalizer=NULL }, ctx));
    JSValue element_proto = JS_NewObject(ctx);
    try(set_property_fn_list(ctx, element_proto, element_fn_list));
    try(set_property_fn_list(ctx, element_proto, node_fn_list));
    JS_SetClassProto(ctx, element_class_id, element_proto);
    return Ok;
}


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
    try( set_property_str(ctx, global, "document", document));
    return Ok;
}

/** document */


/* ---- Location ---- */

static const JSCFunctionListEntry __attribute__((unused)) location_fn_list[] = {
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

//0static Err
//0location_init(JSValue location[_1_], JSContext* ctx, HtmlDoc d[_1_]) {
//0
//0    try(init_class(&location_class_id, &(JSClassDef){ "location", .finalizer=NULL }, ctx));
//0    try(init_instance(location, location_class_id, ctx));
//0
//0    if (JS_SetOpaque(*location, d)) err_jse("could not set the location buffer");
//0    try(set_property_fn_list(ctx, *location, location_fn_list));
//0    return Ok;
//0}
//0
//0static Err
//0global_add_location(JSValue global, HtmlDoc d[_1_], JSContext* ctx) {
//0    JSValue location  = JS_UNDEFINED;
//0    try( location_init(&location, ctx, d));
//0    try( set_property_str(ctx, global, "location", location));
//0    return Ok;
//0}

/** locatiopn **/

/* ---- Navigator ---- */
static const JSCFunctionListEntry  __attribute__((unused)) navigator_fn_list[] = {
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

//0static Err
//0global_add_navigator(JSValue global, HtmlDoc d[_1_], JSContext* ctx) {
//0    (void)d;
//0
//0    JSValue navigator   = JS_UNDEFINED;
//0    try(init_class(&navigator_class_id, &(JSClassDef){"navigator",.finalizer=NULL}, ctx));
//0    JSValue navigator_proto = JS_NewObject(ctx);
//0    if (JS_IsException(navigator_proto)) return err_jse("new object failed");
//0    try(set_property_fn_list(ctx, navigator_proto, navigator_fn_list));
//0    JS_SetClassProto(ctx, navigator_class_id, navigator_proto);
//0    try( set_property_str(ctx, global, "navigator", navigator));
//0    return Ok;
//0}

/** navigaror **/


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

//TODO1: inherit from Event Target
static const JSCFunctionListEntry window_fn_list[] = {
    JS_CGETSET_DEF("caches",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("clientInformation",        jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("closed",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("cookieStore",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("crashReport",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("credentialless",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("crossOriginIsolated",      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("crypto",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("customElements",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("devicePixelRatio",         jse_get_not_implemented, jse_set_not_implemented),
    /* JS_CGETSET_DEF("document",                 jse_get_not_implemented, jse_set_not_implemented), */
    JS_CGETSET_DEF("documentPictureInPicture", jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("event",                    jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("external",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("fence",                    jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("frameElement",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("frames",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("fullScreen",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("history",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("indexedDB",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("innerHeight",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("innerWidth",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("isSecureContext",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("launchQueue",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("length",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("localStorage",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("location",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("locationbar",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("menubar",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("mozInnerScreenX",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("mozInnerScreenY",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("name",                     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("navigation",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("navigator",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("opener",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("orientation",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("origin",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("originAgentCluster",       jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("outerHeight",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("outerWidth",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("pageXOffset",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("pageYOffset",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("parent",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("performance",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("personalbar",              jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scheduler",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("screen",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("screenX",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("screenY",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollMaxX",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollMaxY",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollX",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollY",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("scrollbars",               jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("self",                     jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("sessionStorage",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("sharedStorage",            jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("speechSynthesis",          jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("status",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("statusbar",                jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("toolbar",                  jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("top",                      jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("trustedTypes",             jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("viewport",                 jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("visualViewport",           jse_get_not_implemented, jse_set_not_implemented),
    JS_CGETSET_DEF("window",                   jse_get_not_implemented, jse_set_not_implemented),
    JS_CFUNC_DEF("alert",                            0, jse_fn_not_implemented),
    JS_CFUNC_DEF("atob",                             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("blur",                             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("btoa",                             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("cancelAnimationFrame",             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("cancelIdleCallback",               0, jse_fn_not_implemented),
    JS_CFUNC_DEF("captureEvents",                    0, jse_fn_not_implemented),
    JS_CFUNC_DEF("clearImmediate",                   0, jse_fn_not_implemented),
    JS_CFUNC_DEF("clearInterval",                    0, jse_fn_not_implemented),
    JS_CFUNC_DEF("clearTimeout",                     0, jse_fn_not_implemented),
    JS_CFUNC_DEF("close",                            0, jse_fn_not_implemented),
    JS_CFUNC_DEF("confirm",                          0, jse_fn_not_implemented),
    JS_CFUNC_DEF("createImageBitmap",                0, jse_fn_not_implemented),
    JS_CFUNC_DEF("dump",                             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("fetch",                            0, jse_fn_not_implemented),
    JS_CFUNC_DEF("fetchLater",                       0, jse_fn_not_implemented),
    JS_CFUNC_DEF("find",                             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("focus",                            0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getComputedStyle",                 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getDefaultComputedStyle",          0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getScreenDetails",                 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("getSelection",                     0, jse_fn_not_implemented),
    JS_CFUNC_DEF("matchMedia",                       0, jse_fn_not_implemented),
    JS_CFUNC_DEF("moveBy",                           0, jse_fn_not_implemented),
    JS_CFUNC_DEF("moveTo",                           0, jse_fn_not_implemented),
    JS_CFUNC_DEF("open",                             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("postMessage",                      0, jse_fn_not_implemented),
    JS_CFUNC_DEF("print",                            0, jse_fn_not_implemented),
    JS_CFUNC_DEF("prompt",                           0, jse_fn_not_implemented),
    JS_CFUNC_DEF("queryLocalFonts",                  0, jse_fn_not_implemented),
    // this makes  JS_DefineAutiInitProperty ot abort claiming property already exists?
    /* JS_CFUNC_DEF("queueMicrotask",                   0, jse_fn_not_implemented), */
    JS_CFUNC_DEF("releaseEvents",                    0, jse_fn_not_implemented),
    JS_CFUNC_DEF("reportError",                      0, jse_fn_not_implemented),
    JS_CFUNC_DEF("requestAnimationFrame",            0, jse_fn_not_implemented),
    JS_CFUNC_DEF("requestFileSystem",                0, jse_fn_not_implemented),
    JS_CFUNC_DEF("requestIdleCallback",              0, jse_fn_not_implemented),
    JS_CFUNC_DEF("resizeBy",                         0, jse_fn_not_implemented),
    JS_CFUNC_DEF("resizeTo",                         0, jse_fn_not_implemented),
    JS_CFUNC_DEF("scroll",                           0, jse_fn_not_implemented),
    JS_CFUNC_DEF("scrollBy",                         0, jse_fn_not_implemented),
    JS_CFUNC_DEF("scrollByLines",                    0, jse_fn_not_implemented),
    JS_CFUNC_DEF("scrollByPages",                    0, jse_fn_not_implemented),
    JS_CFUNC_DEF("scrollTo",                         0, jse_fn_not_implemented),
    JS_CFUNC_DEF("setImmediate",                     0, jse_fn_not_implemented),
    JS_CFUNC_DEF("setInterval",                      0, jse_fn_not_implemented),
    JS_CFUNC_DEF("setResizable",                     0, jse_fn_not_implemented),
    JS_CFUNC_DEF("setTimeout",                       2, setTimeout),
    JS_CFUNC_DEF("showDirectoryPicker",              0, jse_fn_not_implemented),
    JS_CFUNC_DEF("showOpenFilePicker",               0, jse_fn_not_implemented),
    JS_CFUNC_DEF("showSaveFilePicker",               0, jse_fn_not_implemented),
    JS_CFUNC_DEF("sizeToContent",                    0, jse_fn_not_implemented),
    JS_CFUNC_DEF("stop",                             0, jse_fn_not_implemented),
    JS_CFUNC_DEF("structuredClone",                  0, jse_fn_not_implemented),
    JS_CFUNC_DEF("webkitConvertPointFromNodeToPage", 0, jse_fn_not_implemented),
    JS_CFUNC_DEF("webkitConvertPointFromPageToNode", 0, jse_fn_not_implemented),
};



static Err
window_init (JSValue window[_1_], JSContext* ctx, HtmlDoc d[_1_]) {
    try(init_class(&window_class_id, &(JSClassDef){ "Window", .finalizer=NULL }, ctx));
    try(init_instance(window, window_class_id, ctx));
    try(set_property_fn_list(ctx, *window, window_fn_list));
    if (JS_SetOpaque(*window, d)) err_jse("could not set the window buffer");
    return Ok;
}

static Err
global_add_window(JSValue global, HtmlDoc d[_1_], JSContext* ctx) {

    JSValue window   = JS_UNDEFINED;
    try(window_init(&window, ctx, d));
    try(set_property_str(ctx, global, "window", window));
    
    return Ok;
}


static const JSCFunctionListEntry global_fn_list[] = {
    JS_CFUNC_DEF("setTimeout", 2, setTimeout),
};
/* ---- Storage ---- */

static const JSCFunctionListEntry __attribute__((unused)) storage_fn_list[] = {
    JS_CFUNC_DEF("getItem",    1, jse_fn_not_implemented),
    JS_CFUNC_DEF("setItem",    2, jse_fn_not_implemented),
    JS_CFUNC_DEF("removeItem", 1, jse_fn_not_implemented),
    JS_CFUNC_DEF("clear",      0, jse_fn_not_implemented),
    JS_CFUNC_DEF("key",        1, jse_fn_not_implemented),
    JS_CGETSET_DEF("length", jse_get_not_implemented, jse_set_not_implemented),
};


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

    tryjmp(e, Fail, node_class_init(js->ctx));
    tryjmp(e, Fail, element_class_init(js->ctx));


    tryjmp(e,Fail, global_add_document(global, htmldoc, js->ctx));
    tryjmp(e,Fail, global_add_console(global, htmldoc, js->ctx));
    tryjmp(e,Fail, global_add_window(global, htmldoc, js->ctx));
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
