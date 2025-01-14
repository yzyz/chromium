// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/chrome_cleaner_dialog_win.h"

#include <memory>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_controller_win.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_dialog_controller_win.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

class MockChromeCleanerDialogController
    : public safe_browsing::ChromeCleanerDialogController {
 public:
  MOCK_METHOD0(DialogShown, void());
  MOCK_METHOD1(Accept, void(bool));
  MOCK_METHOD0(Cancel, void());
  MOCK_METHOD0(Close, void());
  MOCK_METHOD1(DetailsButtonClicked, void(bool));
  MOCK_METHOD1(SetLogsEnabled, void(bool));
  MOCK_METHOD0(LogsEnabled, bool());
};

class ChromeCleanerDialogTest : public DialogBrowserTest {
 public:
  ChromeCleanerDialogTest()
      : mock_dialog_controller_(
            base::MakeUnique<NiceMock<MockChromeCleanerDialogController>>()) {
    ON_CALL(*mock_dialog_controller_, LogsEnabled())
        .WillByDefault(Return(true));
  }

  void ShowDialog(const std::string& name) override {
    chrome::ShowChromeCleanerPrompt(browser(), mock_dialog_controller_.get());
  }

 protected:
  // Since the DialogBrowserTest can be run interactively, we use NiceMock here
  // to suppress warnings about uninteresting calls.
  std::unique_ptr<NiceMock<MockChromeCleanerDialogController>>
      mock_dialog_controller_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeCleanerDialogTest);
};

IN_PROC_BROWSER_TEST_F(ChromeCleanerDialogTest, InvokeDialog_default) {
  RunDialog();
}

}  // namespace
