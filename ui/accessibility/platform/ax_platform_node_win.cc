// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <atlbase.h>
#include <atlcom.h>
#include <limits.h>
#include <oleacc.h>
#include <stdint.h>

#include "base/containers/hash_tables.h"
#include "base/lazy_instance.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/enum_variant.h"
#include "base/win/scoped_comptr.h"
#include "base/win/scoped_variant.h"
#include "third_party/iaccessible2/ia2_api_all.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_action_data.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_role_properties.h"
#include "ui/accessibility/ax_text_utils.h"
#include "ui/accessibility/ax_tree_data.h"
#include "ui/accessibility/platform/ax_platform_node_delegate.h"
#include "ui/accessibility/platform/ax_platform_node_win.h"
#include "ui/base/win/atl_module.h"
#include "ui/gfx/geometry/rect_conversions.h"

//
// Macros to use at the top of any AXPlatformNodeWin function that implements
// a COM interface. Because COM objects are reference counted and clients
// are completely untrusted, it's important to always first check that our
// object is still valid, and then check that all pointer arguments are
// not NULL.
//

#define COM_OBJECT_VALIDATE() \
    if (!delegate_) \
      return E_FAIL;
#define COM_OBJECT_VALIDATE_1_ARG(arg) \
  if (!delegate_)                      \
    return E_FAIL;                     \
  if (!arg)                            \
    return E_INVALIDARG;
#define COM_OBJECT_VALIDATE_2_ARGS(arg1, arg2) \
  if (!delegate_)                              \
    return E_FAIL;                             \
  if (!arg1)                                   \
    return E_INVALIDARG;                       \
  if (!arg2)                                   \
    return E_INVALIDARG;
#define COM_OBJECT_VALIDATE_3_ARGS(arg1, arg2, arg3) \
  if (!delegate_)                                    \
    return E_FAIL;                                   \
  if (!arg1)                                         \
    return E_INVALIDARG;                             \
  if (!arg2)                                         \
    return E_INVALIDARG;                             \
  if (!arg3)                                         \
    return E_INVALIDARG;
#define COM_OBJECT_VALIDATE_4_ARGS(arg1, arg2, arg3, arg4) \
  if (!delegate_)                                          \
    return E_FAIL;                                         \
  if (!arg1)                                               \
    return E_INVALIDARG;                                   \
  if (!arg2)                                               \
    return E_INVALIDARG;                                   \
  if (!arg3)                                               \
    return E_INVALIDARG;                                   \
  if (!arg4)                                               \
    return E_INVALIDARG;
#define COM_OBJECT_VALIDATE_VAR_ID_AND_GET_TARGET(var_id, target) \
  if (!delegate_)                                                 \
    return E_FAIL;                                                \
  target = GetTargetFromChildID(var_id);                          \
  if (!target)                                                    \
    return E_INVALIDARG;                                          \
  if (!target->delegate_)                                         \
    return E_INVALIDARG;
#define COM_OBJECT_VALIDATE_VAR_ID_1_ARG_AND_GET_TARGET(var_id, arg, target) \
  if (!delegate_)                                                            \
    return E_FAIL;                                                           \
  if (!arg)                                                                  \
    return E_INVALIDARG;                                                     \
  target = GetTargetFromChildID(var_id);                                     \
  if (!target)                                                               \
    return E_INVALIDARG;                                                     \
  if (!target->delegate_)                                                    \
    return E_INVALIDARG;
#define COM_OBJECT_VALIDATE_VAR_ID_2_ARGS_AND_GET_TARGET(var_id, arg1, arg2, \
                                                         target)             \
  if (!delegate_)                                                            \
    return E_FAIL;                                                           \
  if (!arg1)                                                                 \
    return E_INVALIDARG;                                                     \
  if (!arg2)                                                                 \
    return E_INVALIDARG;                                                     \
  target = GetTargetFromChildID(var_id);                                     \
  if (!target)                                                               \
    return E_INVALIDARG;                                                     \
  if (!target->delegate_)                                                    \
    return E_INVALIDARG;
#define COM_OBJECT_VALIDATE_VAR_ID_3_ARGS_AND_GET_TARGET(var_id, arg1, arg2, \
                                                         arg3, target)       \
  if (!delegate_)                                                            \
    return E_FAIL;                                                           \
  if (!arg1)                                                                 \
    return E_INVALIDARG;                                                     \
  if (!arg2)                                                                 \
    return E_INVALIDARG;                                                     \
  if (!arg3)                                                                 \
    return E_INVALIDARG;                                                     \
  target = GetTargetFromChildID(var_id);                                     \
  if (!target)                                                               \
    return E_INVALIDARG;                                                     \
  if (!target->delegate_)                                                    \
    return E_INVALIDARG;
#define COM_OBJECT_VALIDATE_VAR_ID_4_ARGS_AND_GET_TARGET(var_id, arg1, arg2, \
                                                         arg3, arg4, target) \
  if (!delegate_)                                                            \
    return E_FAIL;                                                           \
  if (!arg1)                                                                 \
    return E_INVALIDARG;                                                     \
  if (!arg2)                                                                 \
    return E_INVALIDARG;                                                     \
  if (!arg3)                                                                 \
    return E_INVALIDARG;                                                     \
  if (!arg4)                                                                 \
    return E_INVALIDARG;                                                     \
  target = GetTargetFromChildID(var_id);                                     \
  if (!target)                                                               \
    return E_INVALIDARG;                                                     \
  if (!target->delegate_)                                                    \
    return E_INVALIDARG;

namespace ui {

namespace {

typedef base::hash_set<AXPlatformNodeWin*> AXPlatformNodeWinSet;
// Set of all AXPlatformNodeWin objects that were the target of an
// alert event.
base::LazyInstance<AXPlatformNodeWinSet>::DestructorAtExit g_alert_targets =
    LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<base::ObserverList<IAccessible2UsageObserver>>::
    DestructorAtExit g_iaccessible2_usage_observer_list =
        LAZY_INSTANCE_INITIALIZER;

}  // namespace

//
// IAccessible2UsageObserver
//

IAccessible2UsageObserver::IAccessible2UsageObserver() {
}

IAccessible2UsageObserver::~IAccessible2UsageObserver() {
}

// static
base::ObserverList<IAccessible2UsageObserver>&
    GetIAccessible2UsageObserverList() {
  return g_iaccessible2_usage_observer_list.Get();
}

//
// AXPlatformNode::Create
//

// static
AXPlatformNode* AXPlatformNode::Create(AXPlatformNodeDelegate* delegate) {
  // Make sure ATL is initialized in this module.
  ui::win::CreateATLModuleIfNeeded();

  CComObject<AXPlatformNodeWin>* instance = nullptr;
  HRESULT hr = CComObject<AXPlatformNodeWin>::CreateInstance(&instance);
  DCHECK(SUCCEEDED(hr));
  instance->Init(delegate);
  instance->AddRef();
  return instance;
}

// static
AXPlatformNode* AXPlatformNode::FromNativeViewAccessible(
    gfx::NativeViewAccessible accessible) {
  if (!accessible)
    return nullptr;
  base::win::ScopedComPtr<AXPlatformNodeWin> ax_platform_node;
  accessible->QueryInterface(ax_platform_node.GetAddressOf());
  return ax_platform_node.Get();
}

//
// AXPlatformNodeWin
//

AXPlatformNodeWin::AXPlatformNodeWin() {
}

AXPlatformNodeWin::~AXPlatformNodeWin() {
}

//
// AXPlatformNodeBase implementation.
//

void AXPlatformNodeWin::Dispose() {
  Release();
}

void AXPlatformNodeWin::Destroy() {
  RemoveAlertTarget();
  AXPlatformNodeBase::Destroy();
}

//
// AXPlatformNode implementation.
//

gfx::NativeViewAccessible AXPlatformNodeWin::GetNativeViewAccessible() {
  return this;
}

void AXPlatformNodeWin::NotifyAccessibilityEvent(ui::AXEvent event_type) {
  HWND hwnd = delegate_->GetTargetForNativeAccessibilityEvent();
  if (!hwnd)
    return;

  // Menu items fire selection events but Windows screen readers work reliably
  // with focus events. Remap here.
  if (event_type == ui::AX_EVENT_SELECTION &&
      GetData().role == ui::AX_ROLE_MENU_ITEM)
    event_type = ui::AX_EVENT_FOCUS;

  int native_event = MSAAEvent(event_type);
  if (native_event < EVENT_MIN)
    return;

  ::NotifyWinEvent(native_event, hwnd, OBJID_CLIENT, -unique_id_);

  // Keep track of objects that are a target of an alert event.
  if (event_type == ui::AX_EVENT_ALERT)
    AddAlertTarget();
}

int AXPlatformNodeWin::GetIndexInParent() {
  base::win::ScopedComPtr<IDispatch> parent_dispatch;
  base::win::ScopedComPtr<IAccessible> parent_accessible;
  if (S_OK != get_accParent(parent_dispatch.GetAddressOf()))
    return -1;
  if (S_OK != parent_dispatch.CopyTo(parent_accessible.GetAddressOf()))
    return -1;

  LONG child_count = 0;
  if (S_OK != parent_accessible->get_accChildCount(&child_count))
    return -1;
  for (LONG index = 1; index <= child_count; ++index) {
    base::win::ScopedVariant childid_index(index);
    base::win::ScopedComPtr<IDispatch> child_dispatch;
    base::win::ScopedComPtr<IAccessible> child_accessible;
    if (S_OK == parent_accessible->get_accChild(
                    childid_index, child_dispatch.GetAddressOf()) &&
        S_OK == child_dispatch.CopyTo(child_accessible.GetAddressOf())) {
      if (child_accessible.Get() == this)
        return index - 1;
    }
  }

  return -1;
}

//
// IAccessible implementation.
//

STDMETHODIMP AXPlatformNodeWin::accHitTest(
    LONG x_left, LONG y_top, VARIANT* child) {
  COM_OBJECT_VALIDATE_1_ARG(child);

  gfx::Point point(x_left, y_top);
  if (!delegate_->GetScreenBoundsRect().Contains(point)) {
    // Return S_FALSE and VT_EMPTY when outside the object's boundaries.
    child->vt = VT_EMPTY;
    return S_FALSE;
  }

  gfx::NativeViewAccessible hit_child = delegate_->HitTestSync(x_left, y_top);
  if (!hit_child) {
    child->vt = VT_EMPTY;
    return S_FALSE;
  }

  if (hit_child == this) {
    // This object is the best match, so return CHILDID_SELF. It's tempting to
    // simplify the logic and use VT_DISPATCH everywhere, but the Windows
    // call AccessibleObjectFromPoint will keep calling accHitTest until some
    // object returns CHILDID_SELF.
    child->vt = VT_I4;
    child->lVal = CHILDID_SELF;
    return S_OK;
  }

  // Call accHitTest recursively on the result, which may be a recursive call
  // to this function or it may be overridden, for example in the case of a
  // WebView.
  HRESULT result = hit_child->accHitTest(x_left, y_top, child);

  // If the recursive call returned CHILDID_SELF, we have to convert that
  // into a VT_DISPATCH for the return value to this call.
  if (S_OK == result && child->vt == VT_I4 && child->lVal == CHILDID_SELF) {
    child->vt = VT_DISPATCH;
    child->pdispVal = hit_child;
    // Always increment ref when returning a reference to a COM object.
    child->pdispVal->AddRef();
  }
  return result;
}

HRESULT AXPlatformNodeWin::accDoDefaultAction(VARIANT var_id) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_AND_GET_TARGET(var_id, target);
  AXActionData data;
  data.action = ui::AX_ACTION_DO_DEFAULT;

  if (target->delegate_->AccessibilityPerformAction(data))
    return S_OK;
  return E_FAIL;
}

STDMETHODIMP AXPlatformNodeWin::accLocation(
    LONG* x_left, LONG* y_top, LONG* width, LONG* height, VARIANT var_id) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_4_ARGS_AND_GET_TARGET(var_id, x_left, y_top, width,
                                                   height, target);

  gfx::Rect bounds = target->delegate_->GetScreenBoundsRect();
  *x_left = bounds.x();
  *y_top = bounds.y();
  *width  = bounds.width();
  *height = bounds.height();

  if (bounds.IsEmpty())
    return S_FALSE;

  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::accNavigate(
    LONG nav_dir, VARIANT start, VARIANT* end) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_1_ARG_AND_GET_TARGET(start, end, target);
  end->vt = VT_EMPTY;
  if ((nav_dir == NAVDIR_FIRSTCHILD || nav_dir == NAVDIR_LASTCHILD) &&
      V_VT(&start) == VT_I4 && V_I4(&start) != CHILDID_SELF) {
    // MSAA states that navigating to first/last child can only be from self.
    return E_INVALIDARG;
  }

  IAccessible* result = nullptr;
  switch (nav_dir) {

    case NAVDIR_FIRSTCHILD:
      if (delegate_->GetChildCount() > 0)
        result = delegate_->ChildAtIndex(0);
      break;

    case NAVDIR_LASTCHILD:
      if (delegate_->GetChildCount() > 0)
        result = delegate_->ChildAtIndex(delegate_->GetChildCount() - 1);
      break;

    case NAVDIR_NEXT: {
      AXPlatformNodeBase* next = target->GetNextSibling();
      if (next)
        result = next->GetNativeViewAccessible();
      break;
    }

    case NAVDIR_PREVIOUS: {
      AXPlatformNodeBase* previous = target->GetPreviousSibling();
      if (previous)
        result = previous->GetNativeViewAccessible();
      break;
    }

    case NAVDIR_DOWN: {
      // This direction is not implemented except in tables.
      if (!ui::IsTableLikeRole(GetData().role) &&
          !ui::IsCellOrTableHeaderRole(GetData().role))
        return E_NOTIMPL;

      AXPlatformNodeBase* next = target->GetTableCell(
          GetTableRow() + GetTableRowSpan(), GetTableColumn());
      if (!next)
        return S_OK;

      result = next->GetNativeViewAccessible();
      break;
    }

    case NAVDIR_UP: {
      // This direction is not implemented except in tables.
      if (!ui::IsTableLikeRole(GetData().role) &&
          !ui::IsCellOrTableHeaderRole(GetData().role))
        return E_NOTIMPL;

      AXPlatformNodeBase* next =
          target->GetTableCell(GetTableRow() - 1, GetTableColumn());
      if (!next)
        return S_OK;

      result = next->GetNativeViewAccessible();
      break;
    }

    case NAVDIR_LEFT: {
      // This direction is not implemented except in tables.
      if (!ui::IsTableLikeRole(GetData().role) &&
          !ui::IsCellOrTableHeaderRole(GetData().role))
        return E_NOTIMPL;

      AXPlatformNodeBase* next =
          target->GetTableCell(GetTableRow(), GetTableColumn() - 1);
      if (!next)
        return S_OK;

      result = next->GetNativeViewAccessible();
      break;
    }

    case NAVDIR_RIGHT: {
      // This direction is not implemented except in tables.

      if (!ui::IsTableLikeRole(GetData().role) &&
          !ui::IsCellOrTableHeaderRole(GetData().role))
        return E_NOTIMPL;

      AXPlatformNodeBase* next = target->GetTableCell(
          GetTableRow(), GetTableColumn() + GetTableColumnSpan());
      if (!next)
        return S_OK;

      result = next->GetNativeViewAccessible();
      break;
    }
  }

  if (!result)
    return S_FALSE;

  end->vt = VT_DISPATCH;
  end->pdispVal = result;
  // Always increment ref when returning a reference to a COM object.
  end->pdispVal->AddRef();

  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_accChild(VARIANT var_child,
                                             IDispatch** disp_child) {
  *disp_child = nullptr;
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_AND_GET_TARGET(var_child, target);

  *disp_child = target;
  (*disp_child)->AddRef();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_accChildCount(LONG* child_count) {
  COM_OBJECT_VALIDATE_1_ARG(child_count);
  *child_count = delegate_->GetChildCount();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_accDefaultAction(
    VARIANT var_id, BSTR* def_action) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_1_ARG_AND_GET_TARGET(var_id, def_action, target);

  int action;
  if (!target->GetIntAttribute(AX_ATTR_DEFAULT_ACTION_VERB, &action)) {
    *def_action = nullptr;
    return S_FALSE;
  }

  base::string16 action_verb =
      ActionVerbToLocalizedString(static_cast<AXDefaultActionVerb>(action));
  if (action_verb.empty()) {
    *def_action = nullptr;
    return S_FALSE;
  }

  *def_action = SysAllocString(action_verb.c_str());
  DCHECK(def_action);
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_accDescription(
    VARIANT var_id, BSTR* desc) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_1_ARG_AND_GET_TARGET(var_id, desc, target);

  return target->GetStringAttributeAsBstr(ui::AX_ATTR_DESCRIPTION, desc);
}

STDMETHODIMP AXPlatformNodeWin::get_accFocus(VARIANT* focus_child) {
  COM_OBJECT_VALIDATE_1_ARG(focus_child);
  gfx::NativeViewAccessible focus_accessible = delegate_->GetFocus();
  if (focus_accessible == this) {
    focus_child->vt = VT_I4;
    focus_child->lVal = CHILDID_SELF;
  } else if (focus_accessible) {
    focus_child->vt = VT_DISPATCH;
    focus_child->pdispVal = focus_accessible;
    focus_child->pdispVal->AddRef();
    return S_OK;
  } else {
    focus_child->vt = VT_EMPTY;
  }

  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_accKeyboardShortcut(
    VARIANT var_id, BSTR* acc_key) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_1_ARG_AND_GET_TARGET(var_id, acc_key, target);

  return target->GetStringAttributeAsBstr(ui::AX_ATTR_KEY_SHORTCUTS, acc_key);
}

STDMETHODIMP AXPlatformNodeWin::get_accName(
    VARIANT var_id, BSTR* name) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_1_ARG_AND_GET_TARGET(var_id, name, target);

  HRESULT result = target->GetStringAttributeAsBstr(ui::AX_ATTR_NAME, name);
  if (FAILED(result) && MSAARole() == ROLE_SYSTEM_DOCUMENT && GetParent()) {
    // Hack: Some versions of JAWS crash if they get an empty name on
    // a document that's the child of an iframe, so always return a
    // nonempty string for this role.  https://crbug.com/583057
    base::string16 str = L" ";

    *name = SysAllocString(str.c_str());
    DCHECK(*name);
  }

  return result;
}

STDMETHODIMP AXPlatformNodeWin::get_accParent(
    IDispatch** disp_parent) {
  COM_OBJECT_VALIDATE_1_ARG(disp_parent);
  *disp_parent = GetParent();
  if (*disp_parent) {
    (*disp_parent)->AddRef();
    return S_OK;
  }

  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_accRole(
    VARIANT var_id, VARIANT* role) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_1_ARG_AND_GET_TARGET(var_id, role, target);

  // For historical reasons, we return a string (typically
  // containing the HTML tag name) as the MSAA role, rather
  // than a int.
  std::string role_string =
      base::ToUpperASCII(target->StringOverrideForMSAARole());
  if (!role_string.empty()) {
    role->vt = VT_BSTR;
    std::wstring wsTmp(role_string.begin(), role_string.end());
    role->bstrVal = SysAllocString(wsTmp.c_str());
    return S_OK;
  }

  role->vt = VT_I4;
  role->lVal = target->MSAARole();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_accState(
    VARIANT var_id, VARIANT* state) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_1_ARG_AND_GET_TARGET(var_id, state, target);
  state->vt = VT_I4;
  state->lVal = target->MSAAState();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_accHelp(
    VARIANT var_id, BSTR* help) {
  COM_OBJECT_VALIDATE_1_ARG(help);
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_accValue(VARIANT var_id, BSTR* value) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_1_ARG_AND_GET_TARGET(var_id, value, target);

  // get_accValue() has two sets of special cases depending on the node's role.
  // The first set apply without regard for the nodes |value| attribute. That is
  // the nodes value attribute isn't consider for the first set of special
  // cases. For example, if the node role is AX_ROLE_COLOR_WELL, we do not care
  // at all about the node's AX_ATTR_VALUE attribute. The second set of special
  // cases only apply if the value attribute for the node is empty.  That is, if
  // AX_ATTR_VALUE is empty, we do something special.

  base::string16 result;

  //
  // Color Well special case (Use AX_ATTR_COLOR_VALUE)
  //
  if (target->GetData().role == ui::AX_ROLE_COLOR_WELL) {
    unsigned int color = static_cast<unsigned int>(target->GetIntAttribute(
        ui::AX_ATTR_COLOR_VALUE));  // todo, why the static cast?

    unsigned int red = SkColorGetR(color);
    unsigned int green = SkColorGetG(color);
    unsigned int blue = SkColorGetB(color);
    base::string16 value_text;
    value_text = base::UintToString16(red * 100 / 255) + L"% red " +
                 base::UintToString16(green * 100 / 255) + L"% green " +
                 base::UintToString16(blue * 100 / 255) + L"% blue";
    *value = SysAllocString(value_text.c_str());
    DCHECK(*value);
    return S_OK;
  }

  //
  // Document special case (Use the document's url)
  //
  if (target->GetData().role == ui::AX_ROLE_ROOT_WEB_AREA ||
      target->GetData().role == ui::AX_ROLE_WEB_AREA) {
    result = base::UTF8ToUTF16(target->delegate_->GetTreeData().url);
    *value = SysAllocString(result.c_str());
    DCHECK(*value);
    return S_OK;
  }

  //
  // Links (Use AX_ATTR_URL)
  //
  if (target->GetData().role == ui::AX_ROLE_LINK ||
      target->GetData().role == ui::AX_ROLE_IMAGE_MAP_LINK) {
    result = target->GetString16Attribute(ui::AX_ATTR_URL);
    *value = SysAllocString(result.c_str());
    DCHECK(*value);
    return S_OK;
  }

  // After this point, the role based special cases should test for an empty
  // result.

  result = target->GetString16Attribute(ui::AX_ATTR_VALUE);

  //
  // RangeValue (Use AX_ATTR_VALUE_FOR_RANGE)
  //
  if (result.empty() && target->IsRangeValueSupported()) {
    float fval;
    if (target->GetFloatAttribute(ui::AX_ATTR_VALUE_FOR_RANGE, &fval)) {
      result = base::UTF8ToUTF16(base::DoubleToString(fval));
      *value = SysAllocString(result.c_str());
      DCHECK(*value);
      return S_OK;
    }
  }

  // Last resort (Use innerText)
  if (result.empty() &&
      (target->IsSimpleTextControl() || target->IsRichTextControl()) &&
      !target->IsNativeTextControl()) {
    result = target->GetInnerText();
  }

  *value = SysAllocString(result.c_str());
  DCHECK(*value);
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::put_accValue(VARIANT var_id,
                                             BSTR new_value) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_AND_GET_TARGET(var_id, target);

  AXActionData data;
  data.action = ui::AX_ACTION_SET_VALUE;
  data.value = new_value;
  if (target->delegate_->AccessibilityPerformAction(data))
    return S_OK;
  return E_FAIL;
}

STDMETHODIMP AXPlatformNodeWin::get_accSelection(VARIANT* selected) {
  COM_OBJECT_VALIDATE_1_ARG(selected);

  if (GetData().role != ui::AX_ROLE_LIST_BOX)
    return E_NOTIMPL;

  unsigned long selected_count = 0;
  for (auto i = 0; i < delegate_->GetChildCount(); ++i) {
    AXPlatformNodeWin* node = static_cast<AXPlatformNodeWin*>(
        FromNativeViewAccessible(delegate_->ChildAtIndex(i)));

    if (node && node->GetData().state & (1 << ui::AX_STATE_SELECTED))
      ++selected_count;
  }

  if (selected_count == 0) {
    selected->vt = VT_EMPTY;
    return S_OK;
  }

  if (selected_count == 1) {
    for (auto i = 0; i < delegate_->GetChildCount(); ++i) {
      AXPlatformNodeWin* node = static_cast<AXPlatformNodeWin*>(
          FromNativeViewAccessible(delegate_->ChildAtIndex(i)));

      if (node && node->GetData().state & (1 << ui::AX_STATE_SELECTED)) {
        selected->vt = VT_DISPATCH;
        selected->pdispVal = node;
        node->AddRef();
        return S_OK;
      }
    }
  }

  // Multiple items are selected.
  base::win::EnumVariant* enum_variant =
      new base::win::EnumVariant(selected_count);
  enum_variant->AddRef();
  unsigned long index = 0;
  for (auto i = 0; i < delegate_->GetChildCount(); ++i) {
    AXPlatformNodeWin* node = static_cast<AXPlatformNodeWin*>(
        FromNativeViewAccessible(delegate_->ChildAtIndex(i)));

    if (node && node->GetData().state & (1 << ui::AX_STATE_SELECTED)) {
      enum_variant->ItemAt(index)->vt = VT_DISPATCH;
      enum_variant->ItemAt(index)->pdispVal = node;
      node->AddRef();
      ++index;
    }
  }
  selected->vt = VT_UNKNOWN;
  selected->punkVal = static_cast<IUnknown*>(
      static_cast<base::win::IUnknownImpl*>(enum_variant));
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::accSelect(
    LONG flagsSelect, VARIANT var_id) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_AND_GET_TARGET(var_id, target);

  if (flagsSelect & SELFLAG_TAKEFOCUS) {
    ui::AXActionData action_data;
    action_data.action = ui::AX_ACTION_FOCUS;
    target->delegate_->AccessibilityPerformAction(action_data);
    return S_OK;
  }

  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_accHelpTopic(
    BSTR* help_file, VARIANT var_id, LONG* topic_id) {
  AXPlatformNodeWin* target;
  COM_OBJECT_VALIDATE_VAR_ID_2_ARGS_AND_GET_TARGET(var_id, help_file, topic_id,
                                                   target);
  if (help_file) {
    *help_file = nullptr;
  }
  if (topic_id) {
    *topic_id = static_cast<LONG>(-1);
  }
  return E_NOTIMPL;
}

STDMETHODIMP AXPlatformNodeWin::put_accName(
    VARIANT var_id, BSTR put_name) {
  // Deprecated.
  return E_NOTIMPL;
}

//
// IAccessible2 implementation.
//

STDMETHODIMP AXPlatformNodeWin::role(LONG* role) {
  COM_OBJECT_VALIDATE_1_ARG(role);

  *role = IA2Role();
  // If we didn't explicitly set the IAccessible2 role, make it the same
  // as the MSAA role.
  if (!*role)
    *role = MSAARole();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_states(AccessibleStates* states) {
  COM_OBJECT_VALIDATE_1_ARG(states);
  *states = IA2State();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_uniqueID(LONG* unique_id) {
  COM_OBJECT_VALIDATE_1_ARG(unique_id);
  *unique_id = -unique_id_;
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_windowHandle(HWND* window_handle) {
  COM_OBJECT_VALIDATE_1_ARG(window_handle);
  *window_handle = delegate_->GetTargetForNativeAccessibilityEvent();
  return *window_handle ? S_OK : S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_relationTargetsOfType(
    BSTR type_bstr,
    long max_targets,
    IUnknown ***targets,
    long *n_targets) {
  COM_OBJECT_VALIDATE_2_ARGS(targets, n_targets);

  *n_targets = 0;
  *targets = nullptr;

  // Only respond to requests for relations of type "alerts".
  base::string16 type(type_bstr);
  if (type != L"alerts")
    return S_FALSE;

  // Collect all of the objects that have had an alert fired on them that
  // are a descendant of this object.
  std::vector<AXPlatformNodeWin*> alert_targets;
  for (auto iter = g_alert_targets.Get().begin();
       iter != g_alert_targets.Get().end();
       ++iter) {
    AXPlatformNodeWin* target = *iter;
    if (IsDescendant(target))
      alert_targets.push_back(target);
  }

  long count = static_cast<long>(alert_targets.size());
  if (count == 0)
    return S_FALSE;

  // Don't return more targets than max_targets - but note that the caller
  // is allowed to specify max_targets=0 to mean no limit.
  if (max_targets > 0 && count > max_targets)
    count = max_targets;

  // Return the number of targets.
  *n_targets = count;

  // Allocate COM memory for the result array and populate it.
  *targets = static_cast<IUnknown**>(
      CoTaskMemAlloc(count * sizeof(IUnknown*)));
  for (long i = 0; i < count; ++i) {
    (*targets)[i] = static_cast<IAccessible*>(alert_targets[i]);
    (*targets)[i]->AddRef();
  }
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_attributes(BSTR* attributes) {
  COM_OBJECT_VALIDATE_1_ARG(attributes);
  base::string16 attributes_str;

  // Text fields need to report the attribute "text-model:a1" to instruct
  // screen readers to use IAccessible2 APIs to handle text editing in this
  // object (as opposed to treating it like a native Windows text box).
  // The text-model:a1 attribute is documented here:
  // http://www.linuxfoundation.org/collaborate/workgroups/accessibility/ia2/ia2_implementation_guide
  if (GetData().role == ui::AX_ROLE_TEXT_FIELD) {
    attributes_str = L"text-model:a1;";
  }

  *attributes = SysAllocString(attributes_str.c_str());
  DCHECK(*attributes);
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_indexInParent(LONG* index_in_parent) {
  COM_OBJECT_VALIDATE_1_ARG(index_in_parent);
  *index_in_parent = GetIndexInParent();
  if (*index_in_parent < 0)
    return E_FAIL;

  return S_OK;
}

//
// IAccessible2 methods not implemented.
//

STDMETHODIMP AXPlatformNodeWin::get_attribute(BSTR name, VARIANT* attribute) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_extendedRole(BSTR* extended_role) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_nRelations(LONG* n_relations) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_relation(LONG relation_index,
                                             IAccessibleRelation** relation) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_relations(LONG max_relations,
                                              IAccessibleRelation** relations,
                                              LONG* n_relations) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::scrollTo(enum IA2ScrollType scroll_type) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::scrollToPoint(
    enum IA2CoordinateType coordinate_type,
    LONG x,
    LONG y) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_groupPosition(LONG* group_level,
                                                  LONG* similar_items_in_group,
                                                  LONG* position_in_group) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_localizedExtendedRole(
    BSTR* localized_extended_role) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_nExtendedStates(LONG* n_extended_states) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_extendedStates(LONG max_extended_states,
                                                   BSTR** extended_states,
                                                   LONG* n_extended_states) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_localizedExtendedStates(
    LONG max_localized_extended_states,
    BSTR** localized_extended_states,
    LONG* n_localized_extended_states) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_locale(IA2Locale* locale) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_accessibleWithCaret(IUnknown** accessible,
                                                        long* caret_offset) {
  return E_NOTIMPL;
}

//
// IAccessibleTable methods.
//

STDMETHODIMP AXPlatformNodeWin::get_accessibleAt(long row,
                                                 long column,
                                                 IUnknown** accessible) {
  if (!accessible)
    return E_INVALIDARG;

  AXPlatformNodeBase* cell =
      GetTableCell(static_cast<int>(row), static_cast<int>(column));
  if (cell) {
    auto* node_win = static_cast<AXPlatformNodeWin*>(cell);
    node_win->AddRef();

    *accessible = static_cast<IAccessible*>(node_win);
    return S_OK;
  }

  *accessible = nullptr;
  return E_INVALIDARG;
}

STDMETHODIMP AXPlatformNodeWin::get_caption(IUnknown** accessible) {
  if (!accessible)
    return E_INVALIDARG;

  // TODO(dmazzoni): implement
  *accessible = nullptr;
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_childIndex(long row,
                                               long column,
                                               long* cell_index) {
  if (!cell_index)
    return E_INVALIDARG;

  auto* cell = GetTableCell(static_cast<int>(row), static_cast<int>(column));
  if (cell) {
    *cell_index = static_cast<LONG>(cell->GetTableCellIndex());
    return S_OK;
  }

  *cell_index = 0;
  return E_INVALIDARG;
}

STDMETHODIMP AXPlatformNodeWin::get_columnDescription(long column,
                                                      BSTR* description) {
  if (!description)
    return E_INVALIDARG;

  int columns = GetTableColumnCount();
  if (column < 0 || column >= columns)
    return E_INVALIDARG;

  int rows = GetTableRowCount();
  if (rows <= 0) {
    *description = nullptr;
    return S_FALSE;
  }

  for (int i = 0; i < rows; ++i) {
    auto* cell = GetTableCell(i, column);
    if (cell && cell->GetData().role == ui::AX_ROLE_COLUMN_HEADER) {
      base::string16 cell_name = cell->GetString16Attribute(ui::AX_ATTR_NAME);
      if (cell_name.size() > 0) {
        *description = SysAllocString(cell_name.c_str());
        return S_OK;
      }

      cell_name = cell->GetString16Attribute(ui::AX_ATTR_DESCRIPTION);
      if (cell_name.size() > 0) {
        *description = SysAllocString(cell_name.c_str());
        return S_OK;
      }
    }
  }

  *description = nullptr;
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_columnExtentAt(long row,
                                                   long column,
                                                   long* n_columns_spanned) {
  if (!n_columns_spanned)
    return E_INVALIDARG;

  auto* cell = GetTableCell(static_cast<int>(row), static_cast<int>(column));
  if (!cell)
    return E_INVALIDARG;

  *n_columns_spanned = cell->GetTableColumnSpan();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_columnHeader(
    IAccessibleTable** accessible_table,
    long* starting_row_index) {
  // TODO(dmazzoni): implement
  return E_NOTIMPL;
}

STDMETHODIMP AXPlatformNodeWin::get_columnIndex(long cell_index,
                                                long* column_index) {
  if (!column_index)
    return E_INVALIDARG;

  auto* cell = GetTableCell(cell_index);
  if (!cell)
    return E_INVALIDARG;
  *column_index = cell->GetTableColumn();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_nColumns(long* column_count) {
  if (!column_count)
    return E_INVALIDARG;

  *column_count = GetTableColumnCount();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_nRows(long* row_count) {
  if (!row_count)
    return E_INVALIDARG;

  *row_count = GetTableRowCount();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_nSelectedChildren(long* cell_count) {
  if (!cell_count)
    return E_INVALIDARG;

  // TODO(dmazzoni): add support for selected cells/rows/columns in tables.
  *cell_count = 0;
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_nSelectedColumns(long* column_count) {
  if (!column_count)
    return E_INVALIDARG;

  *column_count = 0;
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_nSelectedRows(long* row_count) {
  if (!row_count)
    return E_INVALIDARG;

  *row_count = 0;
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_rowDescription(long row,
                                                   BSTR* description) {
  if (!description)
    return E_INVALIDARG;

  if (row < 0 || row >= GetTableRowCount())
    return E_INVALIDARG;

  int columns = GetTableColumnCount();
  if (columns <= 0) {
    *description = nullptr;
    return S_FALSE;
  }

  for (int i = 0; i < columns; ++i) {
    auto* cell = GetTableCell(row, i);
    if (cell && cell->GetData().role == ui::AX_ROLE_ROW_HEADER) {
      base::string16 cell_name = cell->GetString16Attribute(ui::AX_ATTR_NAME);
      if (cell_name.size() > 0) {
        *description = SysAllocString(cell_name.c_str());
        return S_OK;
      }
      cell_name = cell->GetString16Attribute(ui::AX_ATTR_DESCRIPTION);
      if (cell_name.size() > 0) {
        *description = SysAllocString(cell_name.c_str());
        return S_OK;
      }
    }
  }

  *description = nullptr;
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_rowExtentAt(long row,
                                                long column,
                                                long* n_rows_spanned) {
  if (!n_rows_spanned)
    return E_INVALIDARG;

  auto* cell = GetTableCell(row, column);
  if (!cell)
    return E_INVALIDARG;

  *n_rows_spanned = GetTableRowSpan();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_rowHeader(
    IAccessibleTable** accessible_table,
    long* starting_column_index) {
  // TODO(dmazzoni): implement
  return E_NOTIMPL;
}

STDMETHODIMP AXPlatformNodeWin::get_rowIndex(long cell_index, long* row_index) {
  if (!row_index)
    return E_INVALIDARG;

  auto* cell = GetTableCell(cell_index);
  if (!cell)
    return E_INVALIDARG;

  *row_index = cell->GetTableRow();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_selectedChildren(long max_children,
                                                     long** children,
                                                     long* n_children) {
  if (!children || !n_children)
    return E_INVALIDARG;

  // TODO(dmazzoni): Implement this.
  *n_children = 0;
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_selectedColumns(long max_columns,
                                                    long** columns,
                                                    long* n_columns) {
  if (!columns || !n_columns)
    return E_INVALIDARG;

  // TODO(dmazzoni): Implement this.
  *n_columns = 0;
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_selectedRows(long max_rows,
                                                 long** rows,
                                                 long* n_rows) {
  if (!rows || !n_rows)
    return E_INVALIDARG;

  // TODO(dmazzoni): Implement this.
  *n_rows = 0;
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_summary(IUnknown** accessible) {
  if (!accessible)
    return E_INVALIDARG;

  // TODO(dmazzoni): implement
  *accessible = nullptr;
  return S_FALSE;
}

STDMETHODIMP AXPlatformNodeWin::get_isColumnSelected(long column,
                                                     boolean* is_selected) {
  if (!is_selected)
    return E_INVALIDARG;

  // TODO(dmazzoni): Implement this.
  *is_selected = false;
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_isRowSelected(long row,
                                                  boolean* is_selected) {
  if (!is_selected)
    return E_INVALIDARG;

  // TODO(dmazzoni): Implement this.
  *is_selected = false;
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_isSelected(long row,
                                               long column,
                                               boolean* is_selected) {
  if (!is_selected)
    return E_INVALIDARG;

  // TODO(dmazzoni): Implement this.
  *is_selected = false;
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_rowColumnExtentsAtIndex(
    long index,
    long* row,
    long* column,
    long* row_extents,
    long* column_extents,
    boolean* is_selected) {
  if (!row || !column || !row_extents || !column_extents || !is_selected)
    return E_INVALIDARG;

  auto* cell = GetTableCell(index);
  if (!cell)
    return E_INVALIDARG;

  *row = cell->GetTableRow();
  *column = cell->GetTableColumn();
  *row_extents = GetTableRowSpan();
  *column_extents = GetTableColumnSpan();
  *is_selected = false;  // Not supported.

  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::selectRow(long row) {
  return E_NOTIMPL;
}

STDMETHODIMP AXPlatformNodeWin::selectColumn(long column) {
  return E_NOTIMPL;
}

STDMETHODIMP AXPlatformNodeWin::unselectRow(long row) {
  return E_NOTIMPL;
}

STDMETHODIMP AXPlatformNodeWin::unselectColumn(long column) {
  return E_NOTIMPL;
}

STDMETHODIMP
AXPlatformNodeWin::get_modelChange(IA2TableModelChange* model_change) {
  return E_NOTIMPL;
}

//
// IAccessibleTable2 methods.
//

STDMETHODIMP AXPlatformNodeWin::get_cellAt(long row,
                                           long column,
                                           IUnknown** cell) {
  if (!cell)
    return E_INVALIDARG;

  AXPlatformNodeBase* table_cell =
      GetTableCell(static_cast<int>(row), static_cast<int>(column));
  if (table_cell) {
    auto* node_win = static_cast<AXPlatformNodeWin*>(table_cell);
    node_win->AddRef();
    *cell = static_cast<IAccessible*>(node_win);
    return S_OK;
  }

  *cell = nullptr;
  return E_INVALIDARG;
}

STDMETHODIMP AXPlatformNodeWin::get_nSelectedCells(long* cell_count) {
  return get_nSelectedChildren(cell_count);
}

STDMETHODIMP AXPlatformNodeWin::get_selectedCells(IUnknown*** cells,
                                                  long* n_selected_cells) {
  if (!cells || !n_selected_cells)
    return E_INVALIDARG;

  // TODO(dmazzoni): Implement this.
  *n_selected_cells = 0;
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_selectedColumns(long** columns,
                                                    long* n_columns) {
  if (!columns || !n_columns)
    return E_INVALIDARG;

  // TODO(dmazzoni): Implement this.
  *n_columns = 0;
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_selectedRows(long** rows, long* n_rows) {
  if (!rows || !n_rows)
    return E_INVALIDARG;

  // TODO(dmazzoni): Implement this.
  *n_rows = 0;
  return S_OK;
}

//
// IAccessibleTableCell methods.
//

STDMETHODIMP AXPlatformNodeWin::get_columnExtent(long* n_columns_spanned) {
  if (!n_columns_spanned)
    return E_INVALIDARG;

  *n_columns_spanned = GetTableColumnSpan();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_columnHeaderCells(
    IUnknown*** cell_accessibles,
    long* n_column_header_cells) {
  if (!cell_accessibles || !n_column_header_cells)
    return E_INVALIDARG;

  *n_column_header_cells = 0;
  auto* table = GetTable();
  if (!table) {
    return S_FALSE;
  }

  int column = GetTableColumn();
  int columns = GetTableColumnCount();
  int rows = GetTableRowCount();
  if (columns <= 0 || rows <= 0 || column < 0 || column >= columns)
    return S_FALSE;

  for (int i = 0; i < rows; ++i) {
    auto* cell = GetTableCell(i, column);
    if (cell && cell->GetData().role == ui::AX_ROLE_COLUMN_HEADER)
      (*n_column_header_cells)++;
  }

  *cell_accessibles = static_cast<IUnknown**>(
      CoTaskMemAlloc((*n_column_header_cells) * sizeof(cell_accessibles[0])));
  int index = 0;
  for (int i = 0; i < rows; ++i) {
    AXPlatformNodeBase* cell = GetTableCell(i, column);
    if (cell && cell->GetData().role == ui::AX_ROLE_COLUMN_HEADER) {
      auto* node_win = static_cast<AXPlatformNodeWin*>(cell);
      node_win->AddRef();

      (*cell_accessibles)[index] = static_cast<IAccessible*>(node_win);
      ++index;
    }
  }

  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_columnIndex(long* column_index) {
  if (!column_index)
    return E_INVALIDARG;

  *column_index = GetTableColumn();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_rowExtent(long* n_rows_spanned) {
  if (!n_rows_spanned)
    return E_INVALIDARG;

  *n_rows_spanned = GetTableRowSpan();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_rowHeaderCells(IUnknown*** cell_accessibles,
                                                   long* n_row_header_cells) {
  if (!cell_accessibles || !n_row_header_cells)
    return E_INVALIDARG;

  *n_row_header_cells = 0;
  auto* table = GetTable();
  if (!table) {
    return S_FALSE;
  }

  int row = GetTableRow();
  int columns = GetTableColumnCount();
  int rows = GetTableRowCount();
  if (columns <= 0 || rows <= 0 || row < 0 || row >= rows)
    return S_FALSE;

  for (int i = 0; i < columns; ++i) {
    auto* cell = GetTableCell(row, i);
    if (cell && cell->GetData().role == ui::AX_ROLE_ROW_HEADER)
      (*n_row_header_cells)++;
  }

  *cell_accessibles = static_cast<IUnknown**>(
      CoTaskMemAlloc((*n_row_header_cells) * sizeof(cell_accessibles[0])));
  int index = 0;
  for (int i = 0; i < columns; ++i) {
    AXPlatformNodeBase* cell = GetTableCell(row, i);
    if (cell && cell->GetData().role == ui::AX_ROLE_ROW_HEADER) {
      auto* node_win = static_cast<AXPlatformNodeWin*>(cell);
      node_win->AddRef();

      (*cell_accessibles)[index] = static_cast<IAccessible*>(node_win);
      ++index;
    }
  }

  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_rowIndex(long* row_index) {
  if (!row_index)
    return E_INVALIDARG;

  *row_index = GetTableRow();
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_isSelected(boolean* is_selected) {
  if (!is_selected)
    return E_INVALIDARG;

  *is_selected = false;
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_rowColumnExtents(long* row_index,
                                                     long* column_index,
                                                     long* row_extents,
                                                     long* column_extents,
                                                     boolean* is_selected) {
  if (!row_index || !column_index || !row_extents || !column_extents ||
      !is_selected) {
    return E_INVALIDARG;
  }

  *row_index = GetTableRow();
  *column_index = GetTableColumn();
  *row_extents = GetTableRowSpan();
  *column_extents = GetTableColumnSpan();
  *is_selected = false;  // Not supported.

  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_table(IUnknown** table) {
  if (!table)
    return E_INVALIDARG;

  auto* find_table = GetTable();
  if (!find_table) {
    *table = nullptr;
    return S_FALSE;
  }

  // The IAccessibleTable interface is still on the AXPlatformNodeWin
  // class.
  auto* node_win = static_cast<AXPlatformNodeWin*>(find_table);
  node_win->AddRef();

  *table = static_cast<IAccessibleTable*>(node_win);
  return S_OK;
}

//
// IAccessibleText
//

STDMETHODIMP AXPlatformNodeWin::get_nCharacters(LONG* n_characters) {
  COM_OBJECT_VALIDATE_1_ARG(n_characters);
  base::string16 text = TextForIAccessibleText();
  *n_characters = static_cast<LONG>(text.size());

  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_caretOffset(LONG* offset) {
  COM_OBJECT_VALIDATE_1_ARG(offset);
  *offset = static_cast<LONG>(GetIntAttribute(ui::AX_ATTR_TEXT_SEL_END));
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_nSelections(LONG* n_selections) {
  COM_OBJECT_VALIDATE_1_ARG(n_selections);
  int sel_start = GetIntAttribute(ui::AX_ATTR_TEXT_SEL_START);
  int sel_end = GetIntAttribute(ui::AX_ATTR_TEXT_SEL_END);
  if (sel_start != sel_end)
    *n_selections = 1;
  else
    *n_selections = 0;
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_selection(LONG selection_index,
                                              LONG* start_offset,
                                              LONG* end_offset) {
  COM_OBJECT_VALIDATE_2_ARGS(start_offset, end_offset);
  if (selection_index != 0)
    return E_INVALIDARG;

  *start_offset = static_cast<LONG>(
      GetIntAttribute(ui::AX_ATTR_TEXT_SEL_START));
  *end_offset = static_cast<LONG>(
      GetIntAttribute(ui::AX_ATTR_TEXT_SEL_END));
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_text(LONG start_offset,
                                         LONG end_offset,
                                         BSTR* text) {
  COM_OBJECT_VALIDATE_1_ARG(text);
  int sel_end = GetIntAttribute(ui::AX_ATTR_TEXT_SEL_START);
  base::string16 text_str = TextForIAccessibleText();
  LONG len = static_cast<LONG>(text_str.size());

  if (start_offset == IA2_TEXT_OFFSET_LENGTH) {
    start_offset = len;
  } else if (start_offset == IA2_TEXT_OFFSET_CARET) {
    start_offset = static_cast<LONG>(sel_end);
  }
  if (end_offset == IA2_TEXT_OFFSET_LENGTH) {
    end_offset = static_cast<LONG>(text_str.size());
  } else if (end_offset == IA2_TEXT_OFFSET_CARET) {
    end_offset = static_cast<LONG>(sel_end);
  }

  // The spec allows the arguments to be reversed.
  if (start_offset > end_offset) {
    LONG tmp = start_offset;
    start_offset = end_offset;
    end_offset = tmp;
  }

  // The spec does not allow the start or end offsets to be out or range;
  // we must return an error if so.
  if (start_offset < 0)
    return E_INVALIDARG;
  if (end_offset > len)
    return E_INVALIDARG;

  base::string16 substr =
      text_str.substr(start_offset, end_offset - start_offset);
  if (substr.empty())
    return S_FALSE;

  *text = SysAllocString(substr.c_str());
  DCHECK(*text);
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::get_textAtOffset(
    LONG offset,
    enum IA2TextBoundaryType boundary_type,
    LONG* start_offset, LONG* end_offset,
    BSTR* text) {
  COM_OBJECT_VALIDATE_3_ARGS(start_offset, end_offset, text);
  // The IAccessible2 spec says we don't have to implement the "sentence"
  // boundary type, we can just let the screen reader handle it.
  if (boundary_type == IA2_TEXT_BOUNDARY_SENTENCE) {
    *start_offset = 0;
    *end_offset = 0;
    *text = nullptr;
    return S_FALSE;
  }

  const base::string16& text_str = TextForIAccessibleText();

  *start_offset = FindBoundary(
      text_str, boundary_type, offset, ui::BACKWARDS_DIRECTION);
  *end_offset = FindBoundary(
      text_str, boundary_type, offset, ui::FORWARDS_DIRECTION);
  return get_text(*start_offset, *end_offset, text);
}

STDMETHODIMP AXPlatformNodeWin::get_textBeforeOffset(
    LONG offset,
    enum IA2TextBoundaryType boundary_type,
    LONG* start_offset, LONG* end_offset,
    BSTR* text) {
  if (!start_offset || !end_offset || !text)
    return E_INVALIDARG;

  // The IAccessible2 spec says we don't have to implement the "sentence"
  // boundary type, we can just let the screenreader handle it.
  if (boundary_type == IA2_TEXT_BOUNDARY_SENTENCE) {
    *start_offset = 0;
    *end_offset = 0;
    *text = nullptr;
    return S_FALSE;
  }

  const base::string16& text_str = TextForIAccessibleText();

  *start_offset = FindBoundary(
      text_str, boundary_type, offset, ui::BACKWARDS_DIRECTION);
  *end_offset = offset;
  return get_text(*start_offset, *end_offset, text);
}

STDMETHODIMP AXPlatformNodeWin::get_textAfterOffset(
    LONG offset,
    enum IA2TextBoundaryType boundary_type,
    LONG* start_offset, LONG* end_offset,
    BSTR* text) {
  if (!start_offset || !end_offset || !text)
    return E_INVALIDARG;

  // The IAccessible2 spec says we don't have to implement the "sentence"
  // boundary type, we can just let the screenreader handle it.
  if (boundary_type == IA2_TEXT_BOUNDARY_SENTENCE) {
    *start_offset = 0;
    *end_offset = 0;
    *text = nullptr;
    return S_FALSE;
  }

  const base::string16& text_str = TextForIAccessibleText();

  *start_offset = offset;
  *end_offset = FindBoundary(
      text_str, boundary_type, offset, ui::FORWARDS_DIRECTION);
  return get_text(*start_offset, *end_offset, text);
}

STDMETHODIMP AXPlatformNodeWin::get_offsetAtPoint(
    LONG x, LONG y, enum IA2CoordinateType coord_type, LONG* offset) {
  COM_OBJECT_VALIDATE_1_ARG(offset);
  // We don't support this method, but we have to return something
  // rather than E_NOTIMPL or screen readers will complain.
  *offset = 0;
  return S_OK;
}

STDMETHODIMP AXPlatformNodeWin::addSelection(LONG start_offset,
                                             LONG end_offset) {
  // We only support one selection.
  return setSelection(0, start_offset, end_offset);
}

STDMETHODIMP AXPlatformNodeWin::removeSelection(LONG selection_index) {
  if (selection_index != 0)
    return E_INVALIDARG;
  // Simply collapse the selection to the position of the caret if a caret is
  // visible, otherwise set the selection to 0.
  return setCaretOffset(GetIntAttribute(ui::AX_ATTR_TEXT_SEL_END));
}

STDMETHODIMP AXPlatformNodeWin::setCaretOffset(LONG offset) {
  return setSelection(0, offset, offset);
}

STDMETHODIMP AXPlatformNodeWin::setSelection(LONG selection_index,
                                             LONG start_offset,
                                             LONG end_offset) {
  if (selection_index != 0)
    return E_INVALIDARG;

  HandleSpecialTextOffset(&start_offset);
  HandleSpecialTextOffset(&end_offset);
  if (start_offset < 0 ||
      start_offset > static_cast<LONG>(TextForIAccessibleText().length())) {
    return E_INVALIDARG;
  }
  if (end_offset < 0 ||
      end_offset > static_cast<LONG>(TextForIAccessibleText().length())) {
    return E_INVALIDARG;
  }

  if (SetTextSelection(static_cast<int>(start_offset),
                       static_cast<int>(end_offset))) {
    return S_OK;
  }
  return E_FAIL;
}

//
// IAccessibleText methods not implemented.
//

STDMETHODIMP AXPlatformNodeWin::get_newText(IA2TextSegment* new_text) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_oldText(IA2TextSegment* old_text) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_characterExtents(
    LONG offset,
    enum IA2CoordinateType coord_type,
    LONG* x,
    LONG* y,
    LONG* width,
    LONG* height) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::scrollSubstringTo(
    LONG start_index,
    LONG end_index,
    enum IA2ScrollType scroll_type) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::scrollSubstringToPoint(
    LONG start_index,
    LONG end_index,
    enum IA2CoordinateType coordinate_type,
    LONG x,
    LONG y) {
  return E_NOTIMPL;
}
STDMETHODIMP AXPlatformNodeWin::get_attributes(LONG offset,
                                               LONG* start_offset,
                                               LONG* end_offset,
                                               BSTR* text_attributes) {
  return E_NOTIMPL;
}

//
// IServiceProvider implementation.
//

STDMETHODIMP AXPlatformNodeWin::QueryService(
    REFGUID guidService, REFIID riid, void** object) {
  COM_OBJECT_VALIDATE_1_ARG(object);

  if (riid == IID_IAccessible2) {
    for (IAccessible2UsageObserver& observer :
         GetIAccessible2UsageObserverList()) {
      observer.OnIAccessible2Used();
    }
  }

  if (guidService == IID_IAccessible ||
      guidService == IID_IAccessible2 ||
      guidService == IID_IAccessible2_2 ||
      guidService == IID_IAccessibleText) {
    return QueryInterface(riid, object);
  }

  *object = nullptr;
  return E_FAIL;
}

//
// Private member functions.
//
int AXPlatformNodeWin::MSAARole() {
  // If this is a web area for a presentational iframe, give it a role of
  // something other than DOCUMENT so that the fact that it's a separate doc
  // is not exposed to AT.
  if (IsWebAreaForPresentationalIframe()) {
    return ROLE_SYSTEM_GROUPING;
  }

  switch (GetData().role) {
    case ui::AX_ROLE_ALERT:
      return ROLE_SYSTEM_ALERT;

    case ui::AX_ROLE_ALERT_DIALOG:
      return ROLE_SYSTEM_DIALOG;

    case ui::AX_ROLE_ANCHOR:
      return ROLE_SYSTEM_LINK;

    case ui::AX_ROLE_APPLICATION:
      return ROLE_SYSTEM_APPLICATION;

    case ui::AX_ROLE_ARTICLE:
      return ROLE_SYSTEM_DOCUMENT;

    case ui::AX_ROLE_AUDIO:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_BANNER:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_BUSY_INDICATOR:
      return ROLE_SYSTEM_ANIMATION;

    case ui::AX_ROLE_BUTTON:
      return ROLE_SYSTEM_PUSHBUTTON;

    case ui::AX_ROLE_CANVAS:
      return ROLE_SYSTEM_GRAPHIC;

    case ui::AX_ROLE_CAPTION:
      return ROLE_SYSTEM_TEXT;

    case ui::AX_ROLE_CELL:
      return ROLE_SYSTEM_CELL;

    case ui::AX_ROLE_CHECK_BOX:
      return ROLE_SYSTEM_CHECKBUTTON;

    case ui::AX_ROLE_COLOR_WELL:
      return ROLE_SYSTEM_TEXT;

    case ui::AX_ROLE_COLUMN:
      return ROLE_SYSTEM_COLUMN;

    case ui::AX_ROLE_COLUMN_HEADER:
      return ROLE_SYSTEM_COLUMNHEADER;

    case ui::AX_ROLE_COMBO_BOX:
      return ROLE_SYSTEM_COMBOBOX;

    case ui::AX_ROLE_COMPLEMENTARY:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_CONTENT_INFO:
      return ROLE_SYSTEM_TEXT;

    case ui::AX_ROLE_DATE:
    case ui::AX_ROLE_DATE_TIME:
      return ROLE_SYSTEM_DROPLIST;

    case ui::AX_ROLE_DESCRIPTION_LIST_DETAIL:
      return ROLE_SYSTEM_TEXT;

    case ui::AX_ROLE_DESCRIPTION_LIST:
      return ROLE_SYSTEM_LIST;

    case ui::AX_ROLE_DESCRIPTION_LIST_TERM:
      return ROLE_SYSTEM_LISTITEM;

    case ui::AX_ROLE_DETAILS:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_DIALOG:
      return ROLE_SYSTEM_DIALOG;

    case ui::AX_ROLE_DISCLOSURE_TRIANGLE:
      return ROLE_SYSTEM_PUSHBUTTON;

    case ui::AX_ROLE_DOCUMENT:
    case ui::AX_ROLE_ROOT_WEB_AREA:
    case ui::AX_ROLE_WEB_AREA:
      return ROLE_SYSTEM_DOCUMENT;

    case ui::AX_ROLE_EMBEDDED_OBJECT:
      if (delegate_->GetChildCount()) {
        return ROLE_SYSTEM_GROUPING;
      } else {
        return ROLE_SYSTEM_CLIENT;
      }

    case ui::AX_ROLE_FIGURE:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_FEED:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_GENERIC_CONTAINER:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_GRID:
      return ROLE_SYSTEM_TABLE;

    case ui::AX_ROLE_GROUP:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_HEADING:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_IFRAME:
      return ROLE_SYSTEM_DOCUMENT;

    case ui::AX_ROLE_IFRAME_PRESENTATIONAL:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_IMAGE:
      return ROLE_SYSTEM_GRAPHIC;

    case ui::AX_ROLE_IMAGE_MAP_LINK:
      return ROLE_SYSTEM_LINK;

    case ui::AX_ROLE_INPUT_TIME:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_LABEL_TEXT:
    case ui::AX_ROLE_LEGEND:
      return ROLE_SYSTEM_TEXT;

    case ui::AX_ROLE_LINK:
      return ROLE_SYSTEM_LINK;

    case ui::AX_ROLE_LIST:
      return ROLE_SYSTEM_LIST;

    case ui::AX_ROLE_LIST_BOX:
      return ROLE_SYSTEM_LIST;

    case ui::AX_ROLE_LIST_BOX_OPTION:
      return ROLE_SYSTEM_LISTITEM;

    case ui::AX_ROLE_LIST_ITEM:
      return ROLE_SYSTEM_LISTITEM;

    case ui::AX_ROLE_MAIN:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_MARK:
      return ROLE_SYSTEM_TEXT;

    case ui::AX_ROLE_MARQUEE:
      return ROLE_SYSTEM_ANIMATION;

    case ui::AX_ROLE_MATH:
      return ROLE_SYSTEM_EQUATION;

    case ui::AX_ROLE_MENU:
    case ui::AX_ROLE_MENU_BUTTON:
      return ROLE_SYSTEM_MENUPOPUP;

    case ui::AX_ROLE_MENU_BAR:
      return ROLE_SYSTEM_MENUBAR;

    case ui::AX_ROLE_MENU_ITEM:
      return ROLE_SYSTEM_MENUITEM;

    case ui::AX_ROLE_MENU_ITEM_CHECK_BOX:
      return ROLE_SYSTEM_MENUITEM;

    case ui::AX_ROLE_MENU_ITEM_RADIO:
      return ROLE_SYSTEM_MENUITEM;

    case ui::AX_ROLE_MENU_LIST_POPUP:
      return ROLE_SYSTEM_MENUPOPUP;

    case ui::AX_ROLE_MENU_LIST_OPTION:
      return ROLE_SYSTEM_MENUITEM;

    case ui::AX_ROLE_METER:
      return ROLE_SYSTEM_PROGRESSBAR;

    case ui::AX_ROLE_NAVIGATION:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_NOTE:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_OUTLINE:
      return ROLE_SYSTEM_OUTLINE;

    case ui::AX_ROLE_POP_UP_BUTTON: {
      std::string html_tag = GetData().GetStringAttribute(ui::AX_ATTR_HTML_TAG);
      if (html_tag == "select")
        return ROLE_SYSTEM_COMBOBOX;
      return ROLE_SYSTEM_BUTTONMENU;
    }
    case ui::AX_ROLE_PRE:
      return ROLE_SYSTEM_TEXT;

    case ui::AX_ROLE_PROGRESS_INDICATOR:
      return ROLE_SYSTEM_PROGRESSBAR;

    case ui::AX_ROLE_RADIO_BUTTON:
      return ROLE_SYSTEM_RADIOBUTTON;

    case ui::AX_ROLE_RADIO_GROUP:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_REGION: {
      std::string html_tag = GetData().GetStringAttribute(ui::AX_ATTR_HTML_TAG);
      if (html_tag == "section")
        return ROLE_SYSTEM_GROUPING;
      return ROLE_SYSTEM_PANE;
    }

    case ui::AX_ROLE_ROW: {
      // Role changes depending on whether row is inside a treegrid
      // https://www.w3.org/TR/core-aam-1.1/#role-map-row
      return IsInTreeGrid() ? ROLE_SYSTEM_OUTLINEITEM : ROLE_SYSTEM_ROW;
    }

    case ui::AX_ROLE_ROW_HEADER:
      return ROLE_SYSTEM_ROWHEADER;

    case ui::AX_ROLE_RUBY:
      return ROLE_SYSTEM_TEXT;

    case ui::AX_ROLE_RULER:
      return ROLE_SYSTEM_CLIENT;

    case ui::AX_ROLE_SCROLL_AREA:
      return ROLE_SYSTEM_CLIENT;

    case ui::AX_ROLE_SCROLL_BAR:
      return ROLE_SYSTEM_SCROLLBAR;

    case ui::AX_ROLE_SEARCH:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_SLIDER:
      return ROLE_SYSTEM_SLIDER;

    case ui::AX_ROLE_SPIN_BUTTON:
      return ROLE_SYSTEM_SPINBUTTON;

    case ui::AX_ROLE_SPIN_BUTTON_PART:
      return ROLE_SYSTEM_PUSHBUTTON;

    case ui::AX_ROLE_ANNOTATION:
    case ui::AX_ROLE_LIST_MARKER:
    case ui::AX_ROLE_STATIC_TEXT:
      return ROLE_SYSTEM_STATICTEXT;

    case ui::AX_ROLE_STATUS:
      return ROLE_SYSTEM_STATUSBAR;

    case ui::AX_ROLE_SPLITTER:
      return ROLE_SYSTEM_SEPARATOR;

    case ui::AX_ROLE_SVG_ROOT:
      return ROLE_SYSTEM_GRAPHIC;

    case ui::AX_ROLE_TAB:
      return ROLE_SYSTEM_PAGETAB;

    case ui::AX_ROLE_TABLE:
      return ROLE_SYSTEM_TABLE;

    case ui::AX_ROLE_TABLE_HEADER_CONTAINER:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_TAB_LIST:
      return ROLE_SYSTEM_PAGETABLIST;

    case ui::AX_ROLE_TAB_PANEL:
      return ROLE_SYSTEM_PROPERTYPAGE;

    case ui::AX_ROLE_TERM:
      return ROLE_SYSTEM_LISTITEM;

    case ui::AX_ROLE_TOGGLE_BUTTON:
      return ROLE_SYSTEM_PUSHBUTTON;

    case ui::AX_ROLE_TEXT_FIELD:
    case ui::AX_ROLE_SEARCH_BOX:
      return ROLE_SYSTEM_TEXT;

    case ui::AX_ROLE_ABBR:
    case ui::AX_ROLE_TIME:
      return ROLE_SYSTEM_TEXT;

    case ui::AX_ROLE_TIMER:
      return ROLE_SYSTEM_CLOCK;

    case ui::AX_ROLE_TOOLBAR:
      return ROLE_SYSTEM_TOOLBAR;

    case ui::AX_ROLE_TOOLTIP:
      return ROLE_SYSTEM_TOOLTIP;

    case ui::AX_ROLE_TREE:
      return ROLE_SYSTEM_OUTLINE;

    case ui::AX_ROLE_TREE_GRID:
      return ROLE_SYSTEM_OUTLINE;

    case ui::AX_ROLE_TREE_ITEM:
      return ROLE_SYSTEM_OUTLINEITEM;

    case ui::AX_ROLE_LINE_BREAK:
      return ROLE_SYSTEM_WHITESPACE;

    case ui::AX_ROLE_VIDEO:
      return ROLE_SYSTEM_GROUPING;

    case ui::AX_ROLE_WINDOW:
      return ROLE_SYSTEM_WINDOW;

    // TODO(dmazzoni): figure out the proper MSAA role for roles listed below.
    case AX_ROLE_BLOCKQUOTE:
    case AX_ROLE_BUTTON_DROP_DOWN:
    case AX_ROLE_CARET:
    case AX_ROLE_CLIENT:
    case AX_ROLE_DEFINITION:
    case AX_ROLE_DESKTOP:
    case AX_ROLE_DIRECTORY:
    case AX_ROLE_FIGCAPTION:
    case AX_ROLE_FOOTER:
    case AX_ROLE_FORM:
    case AX_ROLE_IGNORED:
    case AX_ROLE_IMAGE_MAP:
    case AX_ROLE_INLINE_TEXT_BOX:
    case AX_ROLE_LOCATION_BAR:
    case AX_ROLE_LOG:
    case AX_ROLE_NONE:
    case AX_ROLE_PANE:
    case AX_ROLE_PARAGRAPH:
    case AX_ROLE_PRESENTATIONAL:
    case AX_ROLE_SEAMLESS_WEB_AREA:
    case AX_ROLE_SLIDER_THUMB:
    case AX_ROLE_SWITCH:
    case AX_ROLE_TAB_GROUP:
    case AX_ROLE_TITLE_BAR:
    case AX_ROLE_UNKNOWN:
    case AX_ROLE_WEB_VIEW:
      return ROLE_SYSTEM_CLIENT;
  }

  NOTREACHED();
  return ROLE_SYSTEM_CLIENT;
}

std::string AXPlatformNodeWin::StringOverrideForMSAARole() {
  std::string html_tag = GetData().GetStringAttribute(ui::AX_ATTR_HTML_TAG);

  switch (GetData().role) {
    case ui::AX_ROLE_BLOCKQUOTE:
    case ui::AX_ROLE_DEFINITION:
    case ui::AX_ROLE_IMAGE_MAP:
      return html_tag;

    case ui::AX_ROLE_CANVAS:
      if (GetData().GetBoolAttribute(ui::AX_ATTR_CANVAS_HAS_FALLBACK)) {
        return html_tag;
      }
      break;

    case ui::AX_ROLE_FORM:
      // This could be a div with the role of form
      // so we return just the string "form".
      return "form";

    case ui::AX_ROLE_HEADING:
      if (!html_tag.empty())
        return html_tag;
      break;

    case ui::AX_ROLE_PARAGRAPH:
      return html_tag;

    case ui::AX_ROLE_GENERIC_CONTAINER:
      // TODO(dougt) why can't we always use div in this case?
      if (html_tag.empty())
        return "div";
      return html_tag;

    case ui::AX_ROLE_SWITCH:
      return "switch";

    default:
      return "";
  }

  return "";
}

bool AXPlatformNodeWin::IsWebAreaForPresentationalIframe() {
  if (GetData().role != ui::AX_ROLE_WEB_AREA &&
      GetData().role != ui::AX_ROLE_ROOT_WEB_AREA) {
    return false;
  }

  auto* parent = FromNativeViewAccessible(GetParent());
  if (!parent)
    return false;

  return parent->GetData().role == ui::AX_ROLE_IFRAME_PRESENTATIONAL;
}

int32_t AXPlatformNodeWin::IA2State() {
  const AXNodeData& data = GetData();

  int32_t ia2_state = IA2_STATE_OPAQUE;

  const auto checked_state = static_cast<ui::AXCheckedState>(
      GetIntAttribute(ui::AX_ATTR_CHECKED_STATE));
  if (checked_state) {
    ia2_state |= IA2_STATE_CHECKABLE;
  }

  if (HasIntAttribute(ui::AX_ATTR_INVALID_STATE) &&
      GetIntAttribute(ui::AX_ATTR_INVALID_STATE) != ui::AX_INVALID_STATE_FALSE)
    ia2_state |= IA2_STATE_INVALID_ENTRY;
  if (data.HasState(ui::AX_STATE_REQUIRED))
    ia2_state |= IA2_STATE_REQUIRED;
  if (data.HasState(ui::AX_STATE_VERTICAL))
    ia2_state |= IA2_STATE_VERTICAL;
  if (data.HasState(ui::AX_STATE_HORIZONTAL))
    ia2_state |= IA2_STATE_HORIZONTAL;

  const bool is_editable = data.HasState(ui::AX_STATE_EDITABLE);
  if (is_editable)
    ia2_state |= IA2_STATE_EDITABLE;

  if (IsRichTextControl() || ui::IsEditField(data.role)) {
    // Support multi/single line states if root editable or appropriate role.
    // We support the edit box roles even if the area is not actually editable,
    // because it is technically feasible for JS to implement the edit box
    // by controlling selection.
    if (data.HasState(ui::AX_STATE_MULTILINE)) {
      ia2_state |= IA2_STATE_MULTI_LINE;
    } else {
      ia2_state |= IA2_STATE_SINGLE_LINE;
    }
  }

  if (!GetStringAttribute(ui::AX_ATTR_AUTO_COMPLETE).empty())
    ia2_state |= IA2_STATE_SUPPORTS_AUTOCOMPLETION;

  if (GetBoolAttribute(ui::AX_ATTR_MODAL))
    ia2_state |= IA2_STATE_MODAL;

  switch (data.role) {
    case ui::AX_ROLE_MENU_LIST_POPUP:
      ia2_state &= ~(IA2_STATE_EDITABLE);
      break;
    case ui::AX_ROLE_MENU_LIST_OPTION:
      ia2_state &= ~(IA2_STATE_EDITABLE);
      break;
    case ui::AX_ROLE_SCROLL_AREA:
      ia2_state &= ~(IA2_STATE_EDITABLE);
      break;
    case ui::AX_ROLE_TEXT_FIELD:
    case ui::AX_ROLE_SEARCH_BOX:
      if (data.HasState(ui::AX_STATE_MULTILINE)) {
        ia2_state |= IA2_STATE_MULTI_LINE;
      } else {
        ia2_state |= IA2_STATE_SINGLE_LINE;
      }
      ia2_state |= IA2_STATE_SELECTABLE_TEXT;
      break;
    default:
      break;
  }
  return ia2_state;
}

// IA2Role() only returns a role if the MSAA role doesn't suffice,
// otherwise this method returns 0. See AXPlatformNodeWin::role().
int32_t AXPlatformNodeWin::IA2Role() {
  // If this is a web area for a presentational iframe, give it a role of
  // something other than DOCUMENT so that the fact that it's a separate doc
  // is not exposed to AT.
  if (IsWebAreaForPresentationalIframe()) {
    return ROLE_SYSTEM_GROUPING;
  }

  int32_t ia2_role = 0;

  switch (GetData().role) {
    case ui::AX_ROLE_BANNER:
      ia2_role = IA2_ROLE_HEADER;
      break;
    case ui::AX_ROLE_BLOCKQUOTE:
      ia2_role = IA2_ROLE_SECTION;
      break;
    case ui::AX_ROLE_CANVAS:
      if (GetBoolAttribute(ui::AX_ATTR_CANVAS_HAS_FALLBACK)) {
        ia2_role = IA2_ROLE_CANVAS;
      }
      break;
    case ui::AX_ROLE_CAPTION:
      ia2_role = IA2_ROLE_CAPTION;
      break;
    case ui::AX_ROLE_COLOR_WELL:
      ia2_role = IA2_ROLE_COLOR_CHOOSER;
      break;
    case ui::AX_ROLE_COMPLEMENTARY:
      ia2_role = IA2_ROLE_NOTE;
      break;
    case ui::AX_ROLE_CONTENT_INFO:
      ia2_role = IA2_ROLE_PARAGRAPH;
      break;
    case ui::AX_ROLE_DATE:
    case ui::AX_ROLE_DATE_TIME:
      ia2_role = IA2_ROLE_DATE_EDITOR;
      break;
    case ui::AX_ROLE_DEFINITION:
      ia2_role = IA2_ROLE_PARAGRAPH;
      break;
    case ui::AX_ROLE_DESCRIPTION_LIST_DETAIL:
      ia2_role = IA2_ROLE_PARAGRAPH;
      break;
    case ui::AX_ROLE_EMBEDDED_OBJECT:
      if (!delegate_->GetChildCount()) {
        ia2_role = IA2_ROLE_EMBEDDED_OBJECT;
      }
      break;
    case ui::AX_ROLE_FIGCAPTION:
      ia2_role = IA2_ROLE_CAPTION;
      break;
    case ui::AX_ROLE_FORM:
      ia2_role = IA2_ROLE_FORM;
      break;
    case ui::AX_ROLE_FOOTER:
      ia2_role = IA2_ROLE_FOOTER;
      break;
    case ui::AX_ROLE_GENERIC_CONTAINER:
      ia2_role = IA2_ROLE_SECTION;
      break;
    case ui::AX_ROLE_HEADING:
      ia2_role = IA2_ROLE_HEADING;
      break;
    case ui::AX_ROLE_IFRAME:
      ia2_role = IA2_ROLE_INTERNAL_FRAME;
      break;
    case ui::AX_ROLE_IMAGE_MAP:
      ia2_role = IA2_ROLE_IMAGE_MAP;
      break;
    case ui::AX_ROLE_LABEL_TEXT:
    case ui::AX_ROLE_LEGEND:
      ia2_role = IA2_ROLE_LABEL;
      break;
    case ui::AX_ROLE_MAIN:
      ia2_role = IA2_ROLE_PARAGRAPH;
      break;
    case ui::AX_ROLE_MARK:
      ia2_role = IA2_ROLE_TEXT_FRAME;
      break;
    case ui::AX_ROLE_MENU_ITEM_CHECK_BOX:
      ia2_role = IA2_ROLE_CHECK_MENU_ITEM;
      break;
    case ui::AX_ROLE_MENU_ITEM_RADIO:
      ia2_role = IA2_ROLE_RADIO_MENU_ITEM;
      break;
    case ui::AX_ROLE_NAVIGATION:
      ia2_role = IA2_ROLE_SECTION;
      break;
    case ui::AX_ROLE_NOTE:
      ia2_role = IA2_ROLE_NOTE;
      break;
    case ui::AX_ROLE_PARAGRAPH:
      ia2_role = IA2_ROLE_PARAGRAPH;
      break;
    case ui::AX_ROLE_PRE:
      ia2_role = IA2_ROLE_PARAGRAPH;
      break;
    case ui::AX_ROLE_REGION: {
      base::string16 html_tag = GetString16Attribute(ui::AX_ATTR_HTML_TAG);

      if (html_tag == L"section") {
        ia2_role = IA2_ROLE_SECTION;
      }
    } break;
    case ui::AX_ROLE_RUBY:
      ia2_role = IA2_ROLE_TEXT_FRAME;
      break;
    case ui::AX_ROLE_RULER:
      ia2_role = IA2_ROLE_RULER;
      break;
    case ui::AX_ROLE_SCROLL_AREA:
      ia2_role = IA2_ROLE_SCROLL_PANE;
      break;
    case ui::AX_ROLE_SEARCH:
      ia2_role = IA2_ROLE_SECTION;
      break;
    case ui::AX_ROLE_SWITCH:
      ia2_role = IA2_ROLE_TOGGLE_BUTTON;
      break;
    case ui::AX_ROLE_TABLE_HEADER_CONTAINER:
      ia2_role = IA2_ROLE_SECTION;
      break;
    case ui::AX_ROLE_TOGGLE_BUTTON:
      ia2_role = IA2_ROLE_TOGGLE_BUTTON;
      break;
    case ui::AX_ROLE_ABBR:
    case ui::AX_ROLE_TIME:
      ia2_role = IA2_ROLE_TEXT_FRAME;
      break;
    default:
      break;
  }
  return ia2_role;
}

bool AXPlatformNodeWin::ShouldNodeHaveReadonlyState(
    const AXNodeData& data) const {
  if (data.GetBoolAttribute(ui::AX_ATTR_ARIA_READONLY))
    return true;

  if (!data.HasState(ui::AX_STATE_READ_ONLY))
    return false;

  switch (data.role) {
    case ui::AX_ROLE_ARTICLE:
    case ui::AX_ROLE_BUSY_INDICATOR:
    case ui::AX_ROLE_DEFINITION:
    case ui::AX_ROLE_DESCRIPTION_LIST:
    case ui::AX_ROLE_DESCRIPTION_LIST_TERM:
    case ui::AX_ROLE_DOCUMENT:
    case ui::AX_ROLE_IFRAME:
    case ui::AX_ROLE_IMAGE:
    case ui::AX_ROLE_IMAGE_MAP:
    case ui::AX_ROLE_IMAGE_MAP_LINK:
    case ui::AX_ROLE_LIST:
    case ui::AX_ROLE_LIST_ITEM:
    case ui::AX_ROLE_PROGRESS_INDICATOR:
    case ui::AX_ROLE_ROOT_WEB_AREA:
    case ui::AX_ROLE_RULER:
    case ui::AX_ROLE_SCROLL_AREA:
    case ui::AX_ROLE_TERM:
    case ui::AX_ROLE_TIMER:
    case ui::AX_ROLE_TOOLBAR:
    case ui::AX_ROLE_TOOLTIP:
    case ui::AX_ROLE_WEB_AREA:
      return true;

    case ui::AX_ROLE_GRID:
      // TODO(aleventhal) this changed between ARIA 1.0 and 1.1,
      // need to determine whether grids/treegrids should really be readonly
      // or editable by default
      // msaa_state |= STATE_SYSTEM_READONLY;
      break;

    case ui::AX_ROLE_TEXT_FIELD:
    case ui::AX_ROLE_SEARCH_BOX:
      if (data.HasState(ui::AX_STATE_READ_ONLY))
        return true;

    default:
      break;
  }
  return false;
}

bool AXPlatformNodeWin::ShouldNodeHaveFocusableState(
    const AXNodeData& data) const {
  switch (data.role) {
    case ui::AX_ROLE_DOCUMENT:
    case ui::AX_ROLE_ROOT_WEB_AREA:
    case ui::AX_ROLE_WEB_AREA:
      return true;

    case ui::AX_ROLE_IFRAME:
      return false;

    case ui::AX_ROLE_LIST_BOX_OPTION:
    case ui::AX_ROLE_MENU_LIST_OPTION:
      if (data.HasState(ui::AX_STATE_SELECTABLE))
        return true;

    default:
      break;
  }

  return data.HasState(ui::AX_STATE_FOCUSABLE);
}

int AXPlatformNodeWin::MSAAState() {
  const AXNodeData& data = GetData();
  int msaa_state = 0;

  // Map the AXState to MSAA state. Note that some of the states are not
  // currently handled.

  if (data.HasState(ui::AX_STATE_BUSY))
    msaa_state |= STATE_SYSTEM_BUSY;

  if (data.HasState(ui::AX_STATE_COLLAPSED))
    msaa_state |= STATE_SYSTEM_COLLAPSED;

  if (data.HasState(ui::AX_STATE_DEFAULT))
    msaa_state |= STATE_SYSTEM_DEFAULT;

  if (data.HasState(ui::AX_STATE_DISABLED))
    msaa_state |= STATE_SYSTEM_UNAVAILABLE;

  // TODO(dougt) unhandled ux::AX_STATE_EDITABLE

  if (data.HasState(ui::AX_STATE_EXPANDED))
    msaa_state |= STATE_SYSTEM_EXPANDED;

  if (ShouldNodeHaveFocusableState(data))
    msaa_state |= STATE_SYSTEM_FOCUSABLE;

  if (data.HasState(ui::AX_STATE_HASPOPUP))
    msaa_state |= STATE_SYSTEM_HASPOPUP;

  // TODO(dougt) unhandled ux::AX_STATE_HORIZONTAL

  if (data.HasState(ui::AX_STATE_HOVERED)) {
    // Expose whether or not the mouse is over an element, but suppress
    // this for tests because it can make the test results flaky depending
    // on the position of the mouse.
    if (delegate_->ShouldIgnoreHoveredStateForTesting())
      msaa_state |= STATE_SYSTEM_HOTTRACKED;
  }

  // TODO(dougt) Why do we set any state on AX_ROLE_IGNORED?
  if (data.HasState(ui::AX_STATE_INVISIBLE) ||
      GetData().role == ui::AX_ROLE_IGNORED) {
    msaa_state |= STATE_SYSTEM_INVISIBLE;
  }
  if (data.HasState(ui::AX_STATE_LINKED))
    msaa_state |= STATE_SYSTEM_LINKED;

  // TODO(dougt) unhandled ux::AX_STATE_MULTILINE

  if (data.HasState(ui::AX_STATE_MULTISELECTABLE)) {
    msaa_state |= STATE_SYSTEM_EXTSELECTABLE;
    msaa_state |= STATE_SYSTEM_MULTISELECTABLE;
  }

  if (data.HasState(ui::AX_STATE_OFFSCREEN))
    msaa_state |= STATE_SYSTEM_OFFSCREEN;

  if (data.HasState(ui::AX_STATE_PROTECTED))
    msaa_state |= STATE_SYSTEM_PROTECTED;

  // READONLY state is complex on windows.  We set STATE_SYSTEM_READONLY
  // on *some* roles even if the node data isn't marked as AX_STATE_READ_ONLY.
  if (ShouldNodeHaveReadonlyState(data))
    msaa_state |= STATE_SYSTEM_READONLY;

  // TODO(dougt) unhandled ux::AX_STATE_REQUIRED
  // TODO(dougt) unhandled ux::AX_STATE_RICHLY_EDITABLE

  if (data.HasState(ui::AX_STATE_SELECTABLE))
    msaa_state |= STATE_SYSTEM_SELECTABLE;

  if (data.HasState(ui::AX_STATE_SELECTED))
    msaa_state |= STATE_SYSTEM_SELECTED;

  // TODO(dougt) unhandled VERTICAL

  if (data.HasState(ui::AX_STATE_VISITED))
    msaa_state |= STATE_SYSTEM_TRAVERSED;

  //
  // Checked state
  //
  const auto checked_state = static_cast<ui::AXCheckedState>(
      GetIntAttribute(ui::AX_ATTR_CHECKED_STATE));
  switch (checked_state) {
    case ui::AX_CHECKED_STATE_TRUE:
      msaa_state |= data.role == ui::AX_ROLE_TOGGLE_BUTTON
                        ? STATE_SYSTEM_PRESSED
                        : STATE_SYSTEM_CHECKED;
      break;
    case ui::AX_CHECKED_STATE_MIXED:
      msaa_state |= STATE_SYSTEM_MIXED;
      break;
    default:
      break;
  }

  //
  // Handle STATE_SYSTEM_FOCUSED
  //
  gfx::NativeViewAccessible focus = delegate_->GetFocus();
  if (focus == GetNativeViewAccessible())
    msaa_state |= STATE_SYSTEM_FOCUSED;

  // On Windows, the "focus" bit should be set on certain containers, like
  // menu bars, when visible.
  //
  // TODO(dmazzoni): this should probably check if focus is actually inside
  // the menu bar, but we don't currently track focus inside menu pop-ups,
  // and Chrome only has one menu visible at a time so this works for now.
  if (data.role == ui::AX_ROLE_MENU_BAR &&
      !(data.HasState(ui::AX_STATE_INVISIBLE))) {
    msaa_state |= STATE_SYSTEM_FOCUSED;
  }

  // Handle STATE_SYSTEM_LINKED
  if (GetData().role == ui::AX_ROLE_IMAGE_MAP_LINK ||
      GetData().role == ui::AX_ROLE_LINK) {
    msaa_state |= STATE_SYSTEM_LINKED;
  }

  return msaa_state;
}

int AXPlatformNodeWin::MSAAEvent(ui::AXEvent event) {
  switch (event) {
    case ui::AX_EVENT_ALERT:
      return EVENT_SYSTEM_ALERT;
    case ui::AX_EVENT_FOCUS:
      return EVENT_OBJECT_FOCUS;
    case ui::AX_EVENT_MENU_START:
      return EVENT_SYSTEM_MENUSTART;
    case ui::AX_EVENT_MENU_END:
      return EVENT_SYSTEM_MENUEND;
    case ui::AX_EVENT_MENU_POPUP_START:
      return EVENT_SYSTEM_MENUPOPUPSTART;
    case ui::AX_EVENT_MENU_POPUP_END:
      return EVENT_SYSTEM_MENUPOPUPEND;
    case ui::AX_EVENT_SELECTION:
      return EVENT_OBJECT_SELECTION;
    case ui::AX_EVENT_SELECTION_ADD:
      return EVENT_OBJECT_SELECTIONADD;
    case ui::AX_EVENT_SELECTION_REMOVE:
      return EVENT_OBJECT_SELECTIONREMOVE;
    case ui::AX_EVENT_TEXT_CHANGED:
      return EVENT_OBJECT_NAMECHANGE;
    case ui::AX_EVENT_TEXT_SELECTION_CHANGED:
      return IA2_EVENT_TEXT_CARET_MOVED;
    case ui::AX_EVENT_VALUE_CHANGED:
      return EVENT_OBJECT_VALUECHANGE;
    default:
      return -1;
  }
}

HRESULT AXPlatformNodeWin::GetStringAttributeAsBstr(
    ui::AXStringAttribute attribute,
    BSTR* value_bstr) const {
  base::string16 str;

  if (!GetString16Attribute(attribute, &str))
    return S_FALSE;

  *value_bstr = SysAllocString(str.c_str());
  DCHECK(*value_bstr);

  return S_OK;
}

void AXPlatformNodeWin::AddAlertTarget() {
  g_alert_targets.Get().insert(this);
}

void AXPlatformNodeWin::RemoveAlertTarget() {
  if (g_alert_targets.Get().find(this) != g_alert_targets.Get().end())
    g_alert_targets.Get().erase(this);
}

base::string16 AXPlatformNodeWin::TextForIAccessibleText() {
  if (GetData().role == ui::AX_ROLE_TEXT_FIELD)
    return GetString16Attribute(ui::AX_ATTR_VALUE);
  return GetString16Attribute(ui::AX_ATTR_NAME);
}

void AXPlatformNodeWin::HandleSpecialTextOffset(LONG* offset) {
  if (*offset == IA2_TEXT_OFFSET_LENGTH) {
    *offset = static_cast<LONG>(TextForIAccessibleText().length());
  } else if (*offset == IA2_TEXT_OFFSET_CARET) {
    get_caretOffset(offset);
  }
}

ui::TextBoundaryType AXPlatformNodeWin::IA2TextBoundaryToTextBoundary(
    IA2TextBoundaryType ia2_boundary) {
  switch(ia2_boundary) {
    case IA2_TEXT_BOUNDARY_CHAR: return ui::CHAR_BOUNDARY;
    case IA2_TEXT_BOUNDARY_WORD: return ui::WORD_BOUNDARY;
    case IA2_TEXT_BOUNDARY_LINE: return ui::LINE_BOUNDARY;
    case IA2_TEXT_BOUNDARY_SENTENCE: return ui::SENTENCE_BOUNDARY;
    case IA2_TEXT_BOUNDARY_PARAGRAPH: return ui::PARAGRAPH_BOUNDARY;
    case IA2_TEXT_BOUNDARY_ALL: return ui::ALL_BOUNDARY;
    default:
      NOTREACHED();
      return ui::CHAR_BOUNDARY;
  }
}

LONG AXPlatformNodeWin::FindBoundary(
    const base::string16& text,
    IA2TextBoundaryType ia2_boundary,
    LONG start_offset,
    ui::TextBoundaryDirection direction) {
  HandleSpecialTextOffset(&start_offset);
  ui::TextBoundaryType boundary = IA2TextBoundaryToTextBoundary(ia2_boundary);
  std::vector<int32_t> line_breaks;
  return static_cast<LONG>(ui::FindAccessibleTextBoundary(
      text, line_breaks, boundary, start_offset, direction,
      AX_TEXT_AFFINITY_DOWNSTREAM));
}

AXPlatformNodeWin* AXPlatformNodeWin::GetTargetFromChildID(
    const VARIANT& var_id) {
  if (V_VT(&var_id) != VT_I4)
    return nullptr;

  LONG child_id = V_I4(&var_id);
  if (child_id == CHILDID_SELF)
    return this;

  if (child_id >= 1 && child_id <= delegate_->GetChildCount()) {
    // Positive child ids are a 1-based child index, used by clients
    // that want to enumerate all immediate children.
    AXPlatformNodeBase* base =
        FromNativeViewAccessible(delegate_->ChildAtIndex(child_id - 1));
    return static_cast<AXPlatformNodeWin*>(base);
  }

  if (child_id >= 0)
    return nullptr;

  // Negative child ids can be used to map to any descendant.
  AXPlatformNode* node = GetFromUniqueId(-child_id);
  if (!node)
    return nullptr;

  AXPlatformNodeBase* base =
      FromNativeViewAccessible(node->GetNativeViewAccessible());
  if (base && !IsDescendant(base))
    base = nullptr;

  return static_cast<AXPlatformNodeWin*>(base);
}

bool AXPlatformNodeWin::IsInTreeGrid() {
  auto* container = FromNativeViewAccessible(GetParent());

  // If parent was a rowgroup, we need to look at the grandparent
  if (container && container->GetData().role == ui::AX_ROLE_GROUP)
    container = FromNativeViewAccessible(container->GetParent());

  if (!container)
    return false;

  return container->GetData().role == ui::AX_ROLE_TREE_GRID;
}

}  // namespace ui
