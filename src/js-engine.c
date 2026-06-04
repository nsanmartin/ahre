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
static JSClassID window_class_id      = 0;
static JSClassID location_class_id    = 0;
static JSClassID navigator_class_id   = 0;
static JSClassID storage_class_id     = 0;
static JSClassID performance_class_id = 0;
/* static JSClassID dom_token_list_class_id = 0; */
/* static JSClassID classList_class_id = 0; */

/*
 * I'm using many macros here to make this file shorted and write less
*/

#define validate_jsv(Value) _Generic((Value), JSValue: Value)
#define tryjs(Expr) do{\
    Err ahre_jsv_=validate_jsv((Expr));if (JS_IsException(ahre_jsv_)) return ahre_jsv_;}while(0) 

#define err_jse(Msg) err_fmt("error jse: %s %s\n", Msg, file_line__)

#define js_fn__(FnName) JSValue FnName(JSContext* ctx, JSValueConst this, int argc, JSValueConst *argv)
#define js_fn_n__(FnName, ...) JSValue FnName(JSContext* ctx, JSValueConst this, int argc, JSValueConst *argv, __VA_ARGS__)
#define js_get__(FnName) JSValue FnName(JSContext *ctx, JSValueConst this)
#define js_get_n__(FnName, ...) JSValue FnName(JSContext *ctx, JSValueConst this, __VA_ARGS__)
#define js_set__(FnName) JSValue FnName(JSContext* ctx, JSValueConst this, JSValueConst val)
#define js_set_n__(FnName, ...) JSValue FnName(JSContext* ctx, JSValueConst this, JSValueConst val, __VA_ARGS__)

#define xstringify(N) #N
#define stringify(N) xstringify(N)

#define not_impl_fn_name__(Name) jse_fn_not_implemented_ ## Name
#define mk_not_imple_fn_with_name(Name) \
    static js_fn__(not_impl_fn_name__(Name)) {(void)this; (void)argc; (void)argv;\
    return JS_ThrowPlainError(ctx, "ahjs: fn '"# Name "' not implemented."); }

#define mk_not_impl_fn(NS, Fn) mk_not_imple_fn_with_name(NS ## _ ## Fn)

#define mk_not_impl_fn_list_entry(NS, Attr) \
    JS_CFUNC_DEF(stringify(Attr), 0, not_impl_fn_name__(NS ## _ ## Attr))

#define not_impl_getter_name__(Name) jse_getter_not_implemented_ ## Name
#define mk_not_impl_getter(Name) \
    static js_get__(not_impl_getter_name__(Name)) {(void)this;\
    return JS_ThrowPlainError(ctx, "ahjs: getter '"# Name "' not implemented."); }

#define not_impl_setter_name__(Name) jse_setter_not_implemented_ ## Name
#define mk_not_impl_setter(Name) \
    static js_set__(not_impl_setter_name__(Name)) {(void)this; (void)val;\
    return JS_ThrowPlainError(ctx, "ahjs: setter '"# Name "' not implemented." # Name); }

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
singleton_init_add__(
    JSValue                     global,
    JSContext*                  ctx,
    JSClassID                   class_id[1],
    const char*                 class_name,
    const JSCFunctionListEntry* fn_list,
    size_t                      fn_list_len,
    void*                       opaque
) {
    JSValue obj;
    try(init_class(class_id, &(JSClassDef){ class_name, .finalizer=NULL }, ctx));
    try(init_instance(&obj, *class_id, ctx));
    try(set_property_fn_list_len(ctx, obj, fn_list, fn_list_len));
    if (JS_SetOpaque(obj, opaque)) err_jse("could not set the singleton");
    try( set_property_str(ctx, global, class_name, obj));
    return Ok;
}


#define singleton_init_add(ClassName, Global, Ctx, Opaque) \
    singleton_init_add__(\
        Global,\
        Ctx,\
        & ClassName ## _ ## class_id,\
        stringify(ClassName),\
        ClassName ## _ ## fn_list,\
        arr_len(ClassName ## _ ## fn_list),\
        Opaque\
    )


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

mk_not_impl_fn(node, appendChild)
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

static const JSCFunctionListEntry node_fn_list[] = {
    JS_CGETSET_DEF("textContent", not_impl_getter_name__(node_textContent), node_set_textContent),

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

    mk_not_impl_fn_list_entry(node, appendChild),
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

    JS_CFUNC_DEF("getAttribute",             1,     element_getAttribute),
    JS_CFUNC_DEF("remove",                   0,     element_remove),
    JS_CFUNC_DEF("removeAttribute",          1,     element_removeAttribute),
    JS_CFUNC_DEF("setAttribute",             2,     element_setAttribute),

};


static Err
element_class_init(JSContext* ctx) {
    try(init_class(&element_class_id, &(JSClassDef){ "Element", .finalizer=NULL }, ctx));
    JSValue element_proto = JS_NewObject(ctx);
    /* element inherits methods from node */
    try(set_property_fn_list(ctx, element_proto, node_fn_list));
    try(set_property_fn_list(ctx, element_proto, element_fn_list));
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
mk_not_impl_fn(document, createTextNode)
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

static js_get__(location_getter) { (void)this; return get_global(ctx, "location"); }

mk_not_impl_setter(document_location)


static const JSCFunctionListEntry document_fn_list[] = {
    JS_CGETSET_DEF("body",     document_body,      not_impl_setter_name__(document_body)),
    JS_CGETSET_DEF("head",     document_head,      not_impl_setter_name__(document_head)),
    JS_CGETSET_DEF("location", location_getter,    not_impl_setter_name__(document_location)),
    JS_CGETSET_DEF("title",    document_get_title, document_set_title),

    JS_CFUNC_DEF("createElement",  1, document_createElement),
    JS_CFUNC_DEF("getElementById", 1, document_getElementById),

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
    mk_not_impl_fn_list_entry(document, createTextNode),
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

mk_not_impl_getset(location, href)
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
mk_not_impl_fn(location, replace)
mk_not_impl_fn(location, toString)

static const JSCFunctionListEntry location_fn_list[] = {
    mk_not_impl_getset_list_entry(location, href),
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
    mk_not_impl_fn_list_entry(location, replace),
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
mk_not_impl_fn(navigator, sendBeacon)
mk_not_impl_fn(navigator, setAppBadge)
mk_not_impl_fn(navigator, share)
mk_not_impl_fn(navigator, unregisterProtocolHandler)
mk_not_impl_fn(navigator, vibrate)

static const JSCFunctionListEntry navigator_fn_list[] = {
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
    mk_not_impl_fn_list_entry(navigator, sendBeacon),
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
mk_not_impl_getset(window, innerHeight)
mk_not_impl_getset(window, innerWidth)
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
mk_not_impl_getset(window, parent)
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


mk_not_impl_setter(window_location)
mk_not_impl_setter(window_navigator)
mk_not_impl_setter(window_document)
mk_not_impl_setter(window_performance)


static js_get__(window_navigator) { (void)this; return get_global(ctx, "navigator"); }
static js_get__(window_document) { (void)this; return get_global(ctx, "document"); }
static js_get__(window_performance) { (void)this; return get_global(ctx, "performance"); }


//TODO1: inherit from Event Target
static const JSCFunctionListEntry window_fn_list[] = {
    JS_CGETSET_DEF("document",    window_document,    not_impl_setter_name__(window_document)),
    JS_CGETSET_DEF("location",    location_getter,    not_impl_setter_name__(window_location)),
    JS_CGETSET_DEF("navigator",   window_navigator,   not_impl_setter_name__(window_navigator)),
    JS_CGETSET_DEF("performance", window_performance, not_impl_setter_name__(window_performance)),
    JS_CFUNC_DEF("setTimeout", 2, setTimeout),
    // TODO1: this makes  JS_DefineAutiInitProperty ot abort claiming property already exists?,
    /* JS_CFUNC_DEF("queueMicrotask",                   0, jse_fn_not_implemented), */

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
    mk_not_impl_getset_list_entry(window, parent),
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


static Err
storage_class_init(JSContext* ctx) {
    try(init_class(&storage_class_id, &(JSClassDef){ "storage", .finalizer=NULL }, ctx));
    JSValue storage_proto = JS_NewObject(ctx);
    try(set_property_fn_list(ctx, storage_proto, storage_fn_list));
    JS_SetClassProto(ctx, storage_class_id, storage_proto);
    return Ok;
}

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
mk_not_impl_fn(Performance, now)
mk_not_impl_fn(Performance, setResourceTimingBufferSize)
mk_not_impl_fn(Performance, toJSON)

static const JSCFunctionListEntry performance_fn_list[] = {

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
    mk_not_impl_fn_list_entry(Performance, now),
    mk_not_impl_fn_list_entry(Performance, setResourceTimingBufferSize),
    mk_not_impl_fn_list_entry(Performance, toJSON),

};
/** performance */

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

    JSValue global = JS_GetGlobalObject(js->ctx);

    tryjmp(e, Fail, node_class_init(js->ctx));
    tryjmp(e, Fail, element_class_init(js->ctx));
    tryjmp(e, Fail, storage_class_init(js->ctx));

    tryjmp(e,Fail, singleton_init_add(console, global, js->ctx, jse_consolebuf(htmldoc_js(htmldoc))));
    tryjmp(e,Fail, singleton_init_add(location, global, js->ctx, htmldoc));
    tryjmp(e,Fail, singleton_init_add(navigator, global, js->ctx, htmldoc));
    tryjmp(e,Fail, singleton_init_add(performance, global, js->ctx, htmldoc));
    tryjmp(e,Fail, singleton_init_add(document, global, js->ctx, htmldoc));
    tryjmp(e,Fail, singleton_init_add(window, global, js->ctx, htmldoc));

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
