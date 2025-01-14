// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_FILE_SYSTEM_FILE_ENTRY_PICKER_H_
#define CHROME_BROWSER_EXTENSIONS_API_FILE_SYSTEM_FILE_ENTRY_PICKER_H_

#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace base {
class FilePath;
}  // namespace base

namespace content {
class WebContents;
}  // namespace content

namespace extensions {

// Shows a dialog to the user to ask for the filename for a file to save or
// open. Deletes itself once the dialog is closed.
class FileEntryPicker : public ui::SelectFileDialog::Listener {
 public:
  using FilesSelectedCallback =
      base::OnceCallback<void(const std::vector<base::FilePath>& paths)>;

  // Creates a file picker. After the user picks file(s) or cancels, the
  // relevant callback is called and this object deletes itself.
  // The dialog is modal to the |web_contents|'s window.
  // See SelectFileDialog::SelectFile for the other parameters.
  FileEntryPicker(content::WebContents* web_contents,
                  const base::FilePath& suggested_name,
                  const ui::SelectFileDialog::FileTypeInfo& file_type_info,
                  ui::SelectFileDialog::Type picker_type,
                  FilesSelectedCallback files_selected_callback,
                  base::OnceClosure file_selection_canceled_callback);

 private:
  ~FileEntryPicker() override;  // FileEntryPicker deletes itself.

  // ui::SelectFileDialog::Listener implementation.
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;
  void FileSelectedWithExtraInfo(const ui::SelectedFileInfo& file,
                                 int index,
                                 void* params) override;
  void MultiFilesSelected(const std::vector<base::FilePath>& files,
                          void* params) override;
  void MultiFilesSelectedWithExtraInfo(
      const std::vector<ui::SelectedFileInfo>& files,
      void* params) override;
  void FileSelectionCanceled(void* params) override;

  FilesSelectedCallback files_selected_callback_;
  base::OnceClosure file_selection_canceled_callback_;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;

  DISALLOW_COPY_AND_ASSIGN(FileEntryPicker);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_FILE_SYSTEM_FILE_ENTRY_PICKER_H_
