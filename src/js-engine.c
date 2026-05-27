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

#define xstringify(N) #N
#define stringify(N) xstringify(N)

#define not_impl_fn_name__(Name) jse_fn_not_implemented_ ## Name
#define mk_not_impl_fn(Name) \
    static js_fn__(not_impl_fn_name__(Name)) {(void)this; (void)argc; (void)argv;\
    return JS_ThrowPlainError(ctx, "ahjs: fn '"# Name "' not implemented." # Name); }

#define not_impl_getter_name__(Name) jse_getter_not_implemented_ ## Name
#define mk_not_impl_getter(Name) \
    static js_get__(not_impl_getter_name__(Name)) {(void)this;\
    return JS_ThrowPlainError(ctx, "ahjs: getter '"# Name "' not implemented."); }

#define not_impl_setter_name__(Name) jse_setter_not_implemented_ ## Name
#define mk_not_impl_setter(Name) \
    static js_set__(not_impl_setter_name__(Name)) {(void)this; (void)val;\
    return JS_ThrowPlainError(ctx, "ahjs: setter '"# Name "' not implemented." # Name); }

#define concat__(A,B) A ## _ ## B

#define mk_not_impl_getset(NS, Attr) \
    mk_not_impl_getter(NS ## _ ## Attr) \
    mk_not_impl_setter(NS ## _ ## Attr)

#define mk_not_impl_getset_list_entry(NS, Attr) \
    JS_CGETSET_DEF(stringify(Attr), not_impl_getter_name__(NS ## _ ## Attr), not_impl_setter_name__(NS ## _ ## Attr))


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

static const JSCFunctionListEntry node_fn_list[] = {
    JS_CGETSET_DEF("textContent",            jse_get_not_implemented, _set_textContent),

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


mk_not_impl_getset(node, assignedSlot)
mk_not_impl_getset(node, attributes)
mk_not_impl_getset(node, childElementCount)
mk_not_impl_getset(node, children)
mk_not_impl_getset(node, classList)
mk_not_impl_getset(node, className)
mk_not_impl_getset(node, clientHeight)
mk_not_impl_getset(node, clientLeft)
mk_not_impl_getset(node, clientTop)
mk_not_impl_getset(node, clientWidth)
mk_not_impl_getset(node, currentCSSZoom)
mk_not_impl_getset(node, customElementRegistry)
mk_not_impl_getset(node, elementTiming)
mk_not_impl_getset(node, firstElementChild)
mk_not_impl_getset(node, innerHTML)
mk_not_impl_getset(node, lastElementChild)
mk_not_impl_getset(node, localName)
mk_not_impl_getset(node, namespaceURI)
mk_not_impl_getset(node, nextElementSibling)
mk_not_impl_getset(node, outerHTML)
mk_not_impl_getset(node, part)
mk_not_impl_getset(node, prefix)
mk_not_impl_getset(node, previousElementSibling)
mk_not_impl_getset(node, scrollHeight)
mk_not_impl_getset(node, scrollLeft)
mk_not_impl_getset(node, scrollLeftMax)
mk_not_impl_getset(node, scrollTop)
mk_not_impl_getset(node, scrollTopMax)
mk_not_impl_getset(node, scrollWidth)
mk_not_impl_getset(node, shadowRoot)
mk_not_impl_getset(node, slot)
mk_not_impl_getset(node, tagName)
mk_not_impl_getset(node, ariaAtomic)
mk_not_impl_getset(node, ariaAutoComplete)
mk_not_impl_getset(node, ariaBrailleLabel)
mk_not_impl_getset(node, ariaBrailleRoleDescription)
mk_not_impl_getset(node, ariaBusy)
mk_not_impl_getset(node, ariaChecked)
mk_not_impl_getset(node, ariaColCount)
mk_not_impl_getset(node, ariaColIndex)
mk_not_impl_getset(node, ariaColIndexText)
mk_not_impl_getset(node, ariaColSpan)
mk_not_impl_getset(node, ariaCurrent)
mk_not_impl_getset(node, ariaDescription)
mk_not_impl_getset(node, ariaDisabled)
mk_not_impl_getset(node, ariaExpanded)
mk_not_impl_getset(node, ariaHasPopup)
mk_not_impl_getset(node, ariaHidden)
mk_not_impl_getset(node, ariaInvalid)
mk_not_impl_getset(node, ariaKeyShortcuts)
mk_not_impl_getset(node, ariaLabel)
mk_not_impl_getset(node, ariaLevel)
mk_not_impl_getset(node, ariaLive)
mk_not_impl_getset(node, ariaModal)
mk_not_impl_getset(node, ariaMultiline)
mk_not_impl_getset(node, ariaMultiSelectable)
mk_not_impl_getset(node, ariaOrientation)
mk_not_impl_getset(node, ariaPlaceholder)
mk_not_impl_getset(node, ariaPosInSet)
mk_not_impl_getset(node, ariaPressed)
mk_not_impl_getset(node, ariaReadOnly)
mk_not_impl_getset(node, ariaRelevant)
mk_not_impl_getset(node, ariaRequired)
mk_not_impl_getset(node, ariaRoleDescription)
mk_not_impl_getset(node, ariaRowCount)
mk_not_impl_getset(node, ariaRowIndex)
mk_not_impl_getset(node, ariaRowIndexText)
mk_not_impl_getset(node, ariaRowSpan)
mk_not_impl_getset(node, ariaSelected)
mk_not_impl_getset(node, ariaSetSize)
mk_not_impl_getset(node, ariaSort)
mk_not_impl_getset(node, ariaValueMax)
mk_not_impl_getset(node, ariaValueMin)
mk_not_impl_getset(node, ariaValueNow)
mk_not_impl_getset(node, ariaValueText)
mk_not_impl_getset(node, role)
mk_not_impl_getset(node, ariaActiveDescendantElement)
mk_not_impl_getset(node, ariaControlsElements)
mk_not_impl_getset(node, ariaDescribedByElements)
mk_not_impl_getset(node, ariaDetailsElements)
mk_not_impl_getset(node, ariaErrorMessageElements)
mk_not_impl_getset(node, ariaFlowToElements)
mk_not_impl_getset(node, ariaLabelledByElements)
mk_not_impl_getset(node, ariaOwnsElements)

static const JSCFunctionListEntry element_fn_list[] = {
    JS_CGETSET_DEF("id", element_get_id, element_set_id),

    mk_not_impl_getset_list_entry(node, assignedSlot),
    mk_not_impl_getset_list_entry(node, attributes),
    mk_not_impl_getset_list_entry(node, childElementCount),
    mk_not_impl_getset_list_entry(node, children),
    mk_not_impl_getset_list_entry(node, classList),
    mk_not_impl_getset_list_entry(node, className),
    mk_not_impl_getset_list_entry(node, clientHeight),
    mk_not_impl_getset_list_entry(node, clientLeft),
    mk_not_impl_getset_list_entry(node, clientTop),
    mk_not_impl_getset_list_entry(node, clientWidth),
    mk_not_impl_getset_list_entry(node, currentCSSZoom),
    mk_not_impl_getset_list_entry(node, customElementRegistry),
    mk_not_impl_getset_list_entry(node, elementTiming),
    mk_not_impl_getset_list_entry(node, firstElementChild),
    mk_not_impl_getset_list_entry(node, innerHTML),
    mk_not_impl_getset_list_entry(node, lastElementChild),
    mk_not_impl_getset_list_entry(node, localName),
    mk_not_impl_getset_list_entry(node, namespaceURI),
    mk_not_impl_getset_list_entry(node, nextElementSibling),
    mk_not_impl_getset_list_entry(node, outerHTML),
    mk_not_impl_getset_list_entry(node, part),
    mk_not_impl_getset_list_entry(node, prefix),
    mk_not_impl_getset_list_entry(node, previousElementSibling),
    mk_not_impl_getset_list_entry(node, scrollHeight),
    mk_not_impl_getset_list_entry(node, scrollLeft),
    mk_not_impl_getset_list_entry(node, scrollLeftMax),
    mk_not_impl_getset_list_entry(node, scrollTop),
    mk_not_impl_getset_list_entry(node, scrollTopMax),
    mk_not_impl_getset_list_entry(node, scrollWidth),
    mk_not_impl_getset_list_entry(node, shadowRoot),
    mk_not_impl_getset_list_entry(node, slot),
    mk_not_impl_getset_list_entry(node, tagName),
    mk_not_impl_getset_list_entry(node, ariaAtomic),
    mk_not_impl_getset_list_entry(node, ariaAutoComplete),
    mk_not_impl_getset_list_entry(node, ariaBrailleLabel),
    mk_not_impl_getset_list_entry(node, ariaBrailleRoleDescription),
    mk_not_impl_getset_list_entry(node, ariaBusy),
    mk_not_impl_getset_list_entry(node, ariaChecked),
    mk_not_impl_getset_list_entry(node, ariaColCount),
    mk_not_impl_getset_list_entry(node, ariaColIndex),
    mk_not_impl_getset_list_entry(node, ariaColIndexText),
    mk_not_impl_getset_list_entry(node, ariaColSpan),
    mk_not_impl_getset_list_entry(node, ariaCurrent),
    mk_not_impl_getset_list_entry(node, ariaDescription),
    mk_not_impl_getset_list_entry(node, ariaDisabled),
    mk_not_impl_getset_list_entry(node, ariaExpanded),
    mk_not_impl_getset_list_entry(node, ariaHasPopup),
    mk_not_impl_getset_list_entry(node, ariaHidden),
    mk_not_impl_getset_list_entry(node, ariaInvalid),
    mk_not_impl_getset_list_entry(node, ariaKeyShortcuts),
    mk_not_impl_getset_list_entry(node, ariaLabel),
    mk_not_impl_getset_list_entry(node, ariaLevel),
    mk_not_impl_getset_list_entry(node, ariaLive),
    mk_not_impl_getset_list_entry(node, ariaModal),
    mk_not_impl_getset_list_entry(node, ariaMultiline),
    mk_not_impl_getset_list_entry(node, ariaMultiSelectable),
    mk_not_impl_getset_list_entry(node, ariaOrientation),
    mk_not_impl_getset_list_entry(node, ariaPlaceholder),
    mk_not_impl_getset_list_entry(node, ariaPosInSet),
    mk_not_impl_getset_list_entry(node, ariaPressed),
    mk_not_impl_getset_list_entry(node, ariaReadOnly),
    mk_not_impl_getset_list_entry(node, ariaRelevant),
    mk_not_impl_getset_list_entry(node, ariaRequired),
    mk_not_impl_getset_list_entry(node, ariaRoleDescription),
    mk_not_impl_getset_list_entry(node, ariaRowCount),
    mk_not_impl_getset_list_entry(node, ariaRowIndex),
    mk_not_impl_getset_list_entry(node, ariaRowIndexText),
    mk_not_impl_getset_list_entry(node, ariaRowSpan),
    mk_not_impl_getset_list_entry(node, ariaSelected),
    mk_not_impl_getset_list_entry(node, ariaSetSize),
    mk_not_impl_getset_list_entry(node, ariaSort),
    mk_not_impl_getset_list_entry(node, ariaValueMax),
    mk_not_impl_getset_list_entry(node, ariaValueMin),
    mk_not_impl_getset_list_entry(node, ariaValueNow),
    mk_not_impl_getset_list_entry(node, ariaValueText),
    mk_not_impl_getset_list_entry(node, role),
    mk_not_impl_getset_list_entry(node, ariaActiveDescendantElement),
    mk_not_impl_getset_list_entry(node, ariaControlsElements),
    mk_not_impl_getset_list_entry(node, ariaDescribedByElements),
    mk_not_impl_getset_list_entry(node, ariaDetailsElements),
    mk_not_impl_getset_list_entry(node, ariaErrorMessageElements),
    mk_not_impl_getset_list_entry(node, ariaFlowToElements),
    mk_not_impl_getset_list_entry(node, ariaLabelledByElements),
    mk_not_impl_getset_list_entry(node, ariaOwnsElements),

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
mk_not_impl_getset(document, documentElement)
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
mk_not_impl_getset(document, cookie)
mk_not_impl_getset(document, defaultView)
mk_not_impl_getset(document, designMode)
mk_not_impl_getset(document, dir)
mk_not_impl_getset(document, fullscreenEnabled)
mk_not_impl_getset(document, lastModified)
mk_not_impl_getset(document, location)
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

static const JSCFunctionListEntry document_fn_list[] = {
    JS_CGETSET_DEF("body",   document_body,      jse_set_not_implemented),
    JS_CGETSET_DEF("head",   document_head,      jse_set_not_implemented),
    JS_CGETSET_DEF("title",  document_get_title, document_set_title),

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
    mk_not_impl_getset_list_entry(document, documentElement),
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
    mk_not_impl_getset_list_entry(document, cookie),
    mk_not_impl_getset_list_entry(document, defaultView),
    mk_not_impl_getset_list_entry(document, designMode),
    mk_not_impl_getset_list_entry(document, dir),
    mk_not_impl_getset_list_entry(document, fullscreenEnabled),
    mk_not_impl_getset_list_entry(document, lastModified),
    mk_not_impl_getset_list_entry(document, location),
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
mk_not_impl_getset(window, innerHeight)
mk_not_impl_getset(window, innerWidth)
mk_not_impl_getset(window, isSecureContext)
mk_not_impl_getset(window, launchQueue)
mk_not_impl_getset(window, length)
mk_not_impl_getset(window, localStorage)
mk_not_impl_getset(window, location)
mk_not_impl_getset(window, locationbar)
mk_not_impl_getset(window, menubar)
mk_not_impl_getset(window, mozInnerScreenX)
mk_not_impl_getset(window, mozInnerScreenY)
mk_not_impl_getset(window, name)
mk_not_impl_getset(window, navigation)
mk_not_impl_getset(window, navigator)
mk_not_impl_getset(window, opener)
mk_not_impl_getset(window, orientation)
mk_not_impl_getset(window, origin)
mk_not_impl_getset(window, originAgentCluster)
mk_not_impl_getset(window, outerHeight)
mk_not_impl_getset(window, outerWidth)
mk_not_impl_getset(window, pageXOffset)
mk_not_impl_getset(window, pageYOffset)
mk_not_impl_getset(window, parent)
mk_not_impl_getset(window, performance)
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
mk_not_impl_getset(window, self)
mk_not_impl_getset(window, sessionStorage)
mk_not_impl_getset(window, sharedStorage)
mk_not_impl_getset(window, speechSynthesis)
mk_not_impl_getset(window, status)
mk_not_impl_getset(window, statusbar)
mk_not_impl_getset(window, toolbar)
mk_not_impl_getset(window, top)
mk_not_impl_getset(window, trustedTypes)
mk_not_impl_getset(window, viewport)
mk_not_impl_getset(window, visualViewport)
mk_not_impl_getset(window, window)

//TODO1: inherit from Event Target
static const JSCFunctionListEntry window_fn_list[] = {
    /* JS_CGETSET_DEF("document",                 jse_get_not_implemented, jse_set_not_implemented), */

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
    mk_not_impl_getset_list_entry(window, innerHeight),
    mk_not_impl_getset_list_entry(window, innerWidth),
    mk_not_impl_getset_list_entry(window, isSecureContext),
    mk_not_impl_getset_list_entry(window, launchQueue),
    mk_not_impl_getset_list_entry(window, length),
    mk_not_impl_getset_list_entry(window, localStorage),
    mk_not_impl_getset_list_entry(window, location),
    mk_not_impl_getset_list_entry(window, locationbar),
    mk_not_impl_getset_list_entry(window, menubar),
    mk_not_impl_getset_list_entry(window, mozInnerScreenX),
    mk_not_impl_getset_list_entry(window, mozInnerScreenY),
    mk_not_impl_getset_list_entry(window, name),
    mk_not_impl_getset_list_entry(window, navigation),
    mk_not_impl_getset_list_entry(window, navigator),
    mk_not_impl_getset_list_entry(window, opener),
    mk_not_impl_getset_list_entry(window, orientation),
    mk_not_impl_getset_list_entry(window, origin),
    mk_not_impl_getset_list_entry(window, originAgentCluster),
    mk_not_impl_getset_list_entry(window, outerHeight),
    mk_not_impl_getset_list_entry(window, outerWidth),
    mk_not_impl_getset_list_entry(window, pageXOffset),
    mk_not_impl_getset_list_entry(window, pageYOffset),
    mk_not_impl_getset_list_entry(window, parent),
    mk_not_impl_getset_list_entry(window, performance),
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
    mk_not_impl_getset_list_entry(window, self),
    mk_not_impl_getset_list_entry(window, sessionStorage),
    mk_not_impl_getset_list_entry(window, sharedStorage),
    mk_not_impl_getset_list_entry(window, speechSynthesis),
    mk_not_impl_getset_list_entry(window, status),
    mk_not_impl_getset_list_entry(window, statusbar),
    mk_not_impl_getset_list_entry(window, toolbar),
    mk_not_impl_getset_list_entry(window, top),
    mk_not_impl_getset_list_entry(window, trustedTypes),
    mk_not_impl_getset_list_entry(window, viewport),
    mk_not_impl_getset_list_entry(window, visualViewport),
    mk_not_impl_getset_list_entry(window, window),

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
