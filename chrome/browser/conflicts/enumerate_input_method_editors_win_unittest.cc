// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/enumerate_input_method_editors_win.h"

#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_reg_util_win.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class EnumerateInputMethodEditorsTest : public testing::Test {
 protected:
  EnumerateInputMethodEditorsTest() = default;
  ~EnumerateInputMethodEditorsTest() override = default;

  void SetUp() override {
    // Override all registry hives so that real IMEs don't mess up the unit
    // tests.
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_CLASSES_ROOT));
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_CURRENT_USER));
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_LOCAL_MACHINE));
  }

  void RunUntilIdle() { scoped_task_environment_.RunUntilIdle(); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  registry_util::RegistryOverrideManager registry_override_manager_;

  DISALLOW_COPY_AND_ASSIGN(EnumerateInputMethodEditorsTest);
};

// Adds a fake IME entry to the registry that should be found by the
// enumeration. The call must be wrapped inside an ASSERT_NO_FATAL_FAILURE.
void RegisterFakeIme(const wchar_t* guid, const wchar_t* path) {
  base::win::RegKey class_id(
      HKEY_CLASSES_ROOT,
      base::StringPrintf(kClassIdRegistryKeyFormat, guid).c_str(), KEY_WRITE);
  ASSERT_TRUE(class_id.Valid());

  ASSERT_EQ(ERROR_SUCCESS, class_id.WriteValue(nullptr, path));

  base::win::RegKey registration(HKEY_LOCAL_MACHINE, kImeRegistryKey,
                                 KEY_WRITE);
  ASSERT_EQ(ERROR_SUCCESS, registration.CreateKey(guid, KEY_WRITE));
}

void OnImeEnumerated(std::vector<base::FilePath>* imes,
                     const base::FilePath& ime_path,
                     uint32_t size_of_image,
                     uint32_t time_date_stamp) {
  imes->push_back(ime_path);
}

}  // namespace

// Registers a few fake IMEs then see if the enumeration finds them.
TEST_F(EnumerateInputMethodEditorsTest, EnumerateImes2) {
  // Use the current exe file as an arbitrary module that exists.
  base::FilePath file_exe;
  ASSERT_TRUE(base::PathService::Get(base::FILE_EXE, &file_exe));
  ASSERT_NO_FATAL_FAILURE(
      RegisterFakeIme(L"{FAKE_GUID}", file_exe.value().c_str()));

  // Do the asynchronous enumeration.
  std::vector<base::FilePath> imes;
  EnumerateInputMethodEditors(
      base::Bind(&OnImeEnumerated, base::Unretained(&imes)));

  RunUntilIdle();

  EXPECT_EQ(1u, imes.size());
  EXPECT_EQ(file_exe, imes[0]);
}

TEST_F(EnumerateInputMethodEditorsTest, SkipMicrosoftImes) {
  static constexpr wchar_t kMicrosoftImeExample[] =
      L"{6a498709-e00b-4c45-a018-8f9e4081ae40}";

  // Register a fake IME using the Microsoft IME guid.
  ASSERT_NO_FATAL_FAILURE(
      RegisterFakeIme(kMicrosoftImeExample, L"c:\\path\\to\\ime.dll"));

  // Do the asynchronous enumeration.
  std::vector<base::FilePath> imes;
  EnumerateInputMethodEditors(
      base::Bind(&OnImeEnumerated, base::Unretained(&imes)));

  RunUntilIdle();

  EXPECT_TRUE(imes.empty());
}
