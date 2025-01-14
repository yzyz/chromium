// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PASSWORDS_AUTO_SIGNIN_FIRST_RUN_DIALOG_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_PASSWORDS_AUTO_SIGNIN_FIRST_RUN_DIALOG_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/ui/passwords/password_dialog_prompts.h"
#include "ui/views/controls/styled_label_listener.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class StyledLabel;
}

class AutoSigninFirstRunDialogView : public views::DialogDelegateView,
                                     public views::StyledLabelListener,
                                     public AutoSigninFirstRunPrompt {
 public:
  AutoSigninFirstRunDialogView(PasswordDialogController* controller,
                               content::WebContents* web_contents);
  ~AutoSigninFirstRunDialogView() override;

  // AutoSigninFirstRunPrompt:
  void ShowAutoSigninPrompt() override;
  void ControllerGone() override;

 private:
  // views::DialogDelegateView:
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;
  bool ShouldShowCloseButton() const override;
  void WindowClosing() override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;
  bool Cancel() override;
  bool Accept() override;
  bool Close() override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;

  // views::StyledLabelListener:
  void StyledLabelLinkClicked(views::StyledLabel* label,
                              const gfx::Range& range,
                              int event_flags) override;

  // Sets up the child views.
  void InitWindow();

  // A weak pointer to the controller.
  PasswordDialogController* controller_;
  content::WebContents* const web_contents_;

  views::StyledLabel* text_;

  DISALLOW_COPY_AND_ASSIGN(AutoSigninFirstRunDialogView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PASSWORDS_AUTO_SIGNIN_FIRST_RUN_DIALOG_VIEW_H_
