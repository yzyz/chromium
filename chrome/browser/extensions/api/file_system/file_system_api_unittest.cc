// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/file_system/file_system_api.h"

#include <stddef.h>

#include <vector>

#include "base/files/file_path.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/shell_dialogs/select_file_dialog.h"

using extensions::FileSystemChooseEntryFunction;
using extensions::api::file_system::AcceptOption;

namespace extensions {
namespace {

void CheckExtensions(const std::vector<base::FilePath::StringType>& expected,
                     const std::vector<base::FilePath::StringType>& actual) {
  EXPECT_EQ(expected.size(), actual.size());
  if (expected.size() != actual.size())
    return;

  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(expected[i], actual[i]);
  }
}

AcceptOption BuildAcceptOption(const std::string& description,
                               const std::string& mime_types,
                               const std::string& extensions) {
  AcceptOption option;

  if (!description.empty())
    option.description.reset(new std::string(description));

  if (!mime_types.empty()) {
    option.mime_types.reset(new std::vector<std::string>(base::SplitString(
        mime_types, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)));
  }

  if (!extensions.empty()) {
    option.extensions.reset(new std::vector<std::string>(base::SplitString(
        extensions, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)));
  }

  return option;
}

#if defined(OS_WIN)
#define ToStringType base::UTF8ToWide
#else
#define ToStringType
#endif

}  // namespace

TEST(FileSystemApiUnitTest, FileSystemChooseEntryFunctionFileTypeInfoTest) {
  // AcceptsAllTypes is ignored when no other extensions are available.
  ui::SelectFileDialog::FileTypeInfo file_type_info;
  bool acceptsAllTypes = false;
  FileSystemChooseEntryFunction::BuildFileTypeInfo(&file_type_info,
      base::FilePath::StringType(), NULL, &acceptsAllTypes);
  EXPECT_TRUE(file_type_info.include_all_files);
  EXPECT_TRUE(file_type_info.extensions.empty());

  // Test grouping of multiple types.
  file_type_info = ui::SelectFileDialog::FileTypeInfo();
  std::vector<AcceptOption> options;
  options.push_back(BuildAcceptOption(std::string(),
                                      "application/x-chrome-extension", "jso"));
  acceptsAllTypes = false;
  FileSystemChooseEntryFunction::BuildFileTypeInfo(&file_type_info,
      base::FilePath::StringType(), &options, &acceptsAllTypes);
  EXPECT_FALSE(file_type_info.include_all_files);
  ASSERT_EQ(file_type_info.extensions.size(), (size_t) 1);
  EXPECT_TRUE(file_type_info.extension_description_overrides[0].empty()) <<
      "No override must be specified for boring accept types";
  // Note here (and below) that the expectedTypes are sorted, because we use a
  // set internally to generate the output: thus, the output is sorted.
  std::vector<base::FilePath::StringType> expectedTypes;
  expectedTypes.push_back(ToStringType("crx"));
  expectedTypes.push_back(ToStringType("jso"));
  CheckExtensions(expectedTypes, file_type_info.extensions[0]);

  // Test that not satisfying the extension will force all types.
  file_type_info = ui::SelectFileDialog::FileTypeInfo();
  options.clear();
  options.push_back(
      BuildAcceptOption(std::string(), std::string(), "unrelated"));
  acceptsAllTypes = false;
  FileSystemChooseEntryFunction::BuildFileTypeInfo(&file_type_info,
      ToStringType(".jso"), &options, &acceptsAllTypes);
  EXPECT_TRUE(file_type_info.include_all_files);

  // Test multiple list entries, all containing their own types.
  file_type_info = ui::SelectFileDialog::FileTypeInfo();
  options.clear();
  options.push_back(BuildAcceptOption(std::string(), std::string(), "jso,js"));
  options.push_back(BuildAcceptOption(std::string(), std::string(), "cpp,cc"));
  acceptsAllTypes = false;
  FileSystemChooseEntryFunction::BuildFileTypeInfo(&file_type_info,
      base::FilePath::StringType(), &options, &acceptsAllTypes);
  ASSERT_EQ(file_type_info.extensions.size(), options.size());

  expectedTypes.clear();
  expectedTypes.push_back(ToStringType("js"));
  expectedTypes.push_back(ToStringType("jso"));
  CheckExtensions(expectedTypes, file_type_info.extensions[0]);

  expectedTypes.clear();
  expectedTypes.push_back(ToStringType("cc"));
  expectedTypes.push_back(ToStringType("cpp"));
  CheckExtensions(expectedTypes, file_type_info.extensions[1]);

  // Test accept type that causes description override.
  file_type_info = ui::SelectFileDialog::FileTypeInfo();
  options.clear();
  options.push_back(BuildAcceptOption(std::string(), "image/*", "html"));
  acceptsAllTypes = false;
  FileSystemChooseEntryFunction::BuildFileTypeInfo(&file_type_info,
      base::FilePath::StringType(), &options, &acceptsAllTypes);
  ASSERT_EQ(file_type_info.extension_description_overrides.size(), (size_t) 1);
  EXPECT_FALSE(file_type_info.extension_description_overrides[0].empty()) <<
      "Accept type \"image/*\" must generate description override";

  // Test multiple accept types that cause description override causes us to
  // still present the default.
  file_type_info = ui::SelectFileDialog::FileTypeInfo();
  options.clear();
  options.push_back(BuildAcceptOption(std::string(), "image/*,audio/*,video/*",
                                      std::string()));
  acceptsAllTypes = false;
  FileSystemChooseEntryFunction::BuildFileTypeInfo(&file_type_info,
      base::FilePath::StringType(), &options, &acceptsAllTypes);
  ASSERT_EQ(file_type_info.extension_description_overrides.size(), (size_t) 1);
  EXPECT_TRUE(file_type_info.extension_description_overrides[0].empty());

  // Test explicit description override.
  file_type_info = ui::SelectFileDialog::FileTypeInfo();
  options.clear();
  options.push_back(
      BuildAcceptOption("File Types 101", "image/jpeg", std::string()));
  acceptsAllTypes = false;
  FileSystemChooseEntryFunction::BuildFileTypeInfo(&file_type_info,
      base::FilePath::StringType(), &options, &acceptsAllTypes);
  EXPECT_EQ(file_type_info.extension_description_overrides[0],
      base::UTF8ToUTF16("File Types 101"));
}

TEST(FileSystemApiUnitTest, FileSystemChooseEntryFunctionSuggestionTest) {
  std::string opt_name;
  base::FilePath suggested_name;
  base::FilePath::StringType suggested_extension;

  opt_name = std::string("normal_path.txt");
  FileSystemChooseEntryFunction::BuildSuggestion(&opt_name, &suggested_name,
      &suggested_extension);
  EXPECT_FALSE(suggested_name.IsAbsolute());
  EXPECT_EQ(suggested_name.MaybeAsASCII(), "normal_path.txt");
  EXPECT_EQ(suggested_extension, ToStringType("txt"));

  // We should provide just the basename, i.e., "path".
  opt_name = std::string("/a/bad/path");
  FileSystemChooseEntryFunction::BuildSuggestion(&opt_name, &suggested_name,
      &suggested_extension);
  EXPECT_FALSE(suggested_name.IsAbsolute());
  EXPECT_EQ(suggested_name.MaybeAsASCII(), "path");
  EXPECT_TRUE(suggested_extension.empty());

#if !defined(OS_WIN)
  // TODO(thorogood): Fix this test on Windows.
  // Filter out absolute paths with no basename.
  opt_name = std::string("/");
  FileSystemChooseEntryFunction::BuildSuggestion(&opt_name, &suggested_name,
      &suggested_extension);
  EXPECT_FALSE(suggested_name.IsAbsolute());
  EXPECT_TRUE(suggested_name.MaybeAsASCII().empty());
  EXPECT_TRUE(suggested_extension.empty());
#endif
}

}  // namespace extensions
