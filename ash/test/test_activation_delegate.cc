// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/test_activation_delegate.h"

#include "ash/wm/window_util.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/events/event.h"

namespace ash {
namespace test {

////////////////////////////////////////////////////////////////////////////////
// TestActivationDelegate

TestActivationDelegate::TestActivationDelegate()
    : window_(NULL),
      window_was_active_(false),
      activate_(true),
      activated_count_(0),
      lost_active_count_(0),
      should_activate_count_(0) {}

TestActivationDelegate::TestActivationDelegate(bool activate)
    : window_(NULL),
      window_was_active_(false),
      activate_(activate),
      activated_count_(0),
      lost_active_count_(0),
      should_activate_count_(0) {}

void TestActivationDelegate::SetWindow(aura::Window* window) {
  window_ = window;
  ::wm::SetActivationDelegate(window, this);
  ::wm::SetActivationChangeObserver(window, this);
}

bool TestActivationDelegate::ShouldActivate() const {
  should_activate_count_++;
  return activate_;
}

void TestActivationDelegate::OnWindowActivated(
    ::wm::ActivationChangeObserver::ActivationReason reason,
    aura::Window* gained_active,
    aura::Window* lost_active) {
  DCHECK(window_ == gained_active || window_ == lost_active);
  if (window_ == gained_active) {
    activated_count_++;
  } else if (window_ == lost_active) {
    if (lost_active_count_++ == 0)
      window_was_active_ = wm::IsActiveWindow(window_);
  }
}

}  // namespace test
}  // namespace ash
