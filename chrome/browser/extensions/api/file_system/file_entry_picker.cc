// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/file_system/file_entry_picker.h"

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/shell_dialogs/selected_file_info.h"

namespace extensions {

FileEntryPicker::FileEntryPicker(
    content::WebContents* web_contents,
    const base::FilePath& suggested_name,
    const ui::SelectFileDialog::FileTypeInfo& file_type_info,
    ui::SelectFileDialog::Type picker_type,
    FilesSelectedCallback files_selected_callback,
    base::OnceClosure file_selection_canceled_callback)
    : files_selected_callback_(std::move(files_selected_callback)),
      file_selection_canceled_callback_(
          std::move(file_selection_canceled_callback)) {
  CHECK(web_contents);
  gfx::NativeWindow owning_window =
      platform_util::GetTopLevel(web_contents->GetNativeView());
  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, base::MakeUnique<ChromeSelectFilePolicy>(web_contents).release());
  select_file_dialog_->SelectFile(
      picker_type, base::string16(), suggested_name, &file_type_info, 0,
      base::FilePath::StringType(), owning_window, nullptr);
}

FileEntryPicker::~FileEntryPicker() {}

void FileEntryPicker::FileSelected(const base::FilePath& path,
                                   int index,
                                   void* params) {
  MultiFilesSelected({path}, params);
}

void FileEntryPicker::FileSelectedWithExtraInfo(
    const ui::SelectedFileInfo& file,
    int index,
    void* params) {
  // Normally, file.local_path is used because it is a native path to the
  // local read-only cached file in the case of remote file system like
  // Chrome OS's Google Drive integration. Here, however, |file.file_path| is
  // necessary because we need to create a FileEntry denoting the remote file,
  // not its cache. On other platforms than Chrome OS, they are the same.
  //
  // TODO(kinaba): remove this, once after the file picker implements proper
  // switch of the path treatment depending on the |allowed_paths|.
  FileSelected(file.file_path, index, params);
}

void FileEntryPicker::MultiFilesSelected(
    const std::vector<base::FilePath>& files,
    void* params) {
  std::move(files_selected_callback_).Run(files);
  delete this;
}

void FileEntryPicker::MultiFilesSelectedWithExtraInfo(
    const std::vector<ui::SelectedFileInfo>& files,
    void* params) {
  std::vector<base::FilePath> paths;
  for (const auto& file : files)
    paths.push_back(file.file_path);
  MultiFilesSelected(paths, params);
}

void FileEntryPicker::FileSelectionCanceled(void* params) {
  std::move(file_selection_canceled_callback_).Run();
  delete this;
}

}  // namespace extensions
