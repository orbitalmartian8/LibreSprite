// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/win/native_dialogs.h"

#include "base/string.h"
#include "she/error.h"

#include <windows.h>

#include <string>
#include <vector>

namespace she {

// 32k is the limit for Win95/98/Me/NT4/2000/XP with ANSI version
#define FILENAME_BUFSIZE (1024*32)

class FileDialogWin32 : public FileDialog {
public:
  FileDialogWin32()
    : m_filename(FILENAME_BUFSIZE)
    , m_save(false) {
  }

  void dispose() override {
    delete this;
  }

  void toOpenFile() override {
    m_save = false;
  }

  void toSaveFile() override {
    m_save = true;
  }

  void setTitle(const std::string& title) override {
    m_title = base::from_utf8(title);
  }

  void setDefaultExtension(const std::string& extension) override {
    m_defExtension = base::from_utf8(extension);
  }

  void addFilter(const std::string& extension, const std::string& description) override {
    if (m_defExtension.empty()) {
      m_defExtension = base::from_utf8(extension);
      m_defFilter = 0;
    }
    m_filters.push_back(std::make_pair(description, extension));
  }

  std::string getFileName() override {
    return base::to_utf8(&m_filename[0]);
  }

  void setFileName(const std::string& filename) override {
    wcscpy(&m_filename[0], base::from_utf8(filename).c_str());
  }

  bool show(DisplayHandle parent) override {
    std::wstring filtersWStr = getFiltersForGetOpenFileName();

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = (HWND)parent;
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpstrFilter = filtersWStr.c_str();
    ofn.nFilterIndex = m_defFilter;
    ofn.lpstrFile = &m_filename[0];
    ofn.nMaxFile = FILENAME_BUFSIZE;
    ofn.lpstrTitle = m_title.c_str();
    ofn.lpstrDefExt = m_defExtension.c_str();
    ofn.Flags =
      OFN_ENABLESIZING |
      OFN_EXPLORER |
      OFN_LONGNAMES |
      OFN_NOCHANGEDIR |
      OFN_PATHMUSTEXIST;

    if (!m_save)
      ofn.Flags |= OFN_FILEMUSTEXIST;
#if 0                           // This is asked in custom UI
    else
      ofn.Flags |= OFN_OVERWRITEPROMPT;
#endif

    BOOL res;
    if (m_save)
      res = GetSaveFileName(&ofn);
    else
      res = GetOpenFileName(&ofn);

    if (!res) {
      DWORD err = CommDlgExtendedError();
      if (err) {
        std::vector<char> buf(1024);
        sprintf(&buf[0], "Error using GetOpen/SaveFileName Win32 API. Code: %d", err);
        she::error_message(&buf[0]);
      }
    }

    return res != FALSE;
  }

private:

  std::wstring getFiltersForGetOpenFileName() const {
    std::wstring filters;
    for (const auto& filter : m_filters) {
      filters.append(base::from_utf8(filter.first));
      filters.push_back('\0');
      filters.append(base::from_utf8(filter.second));
      filters.push_back('\0');
    }
    filters.push_back('\0');
    return filters;
  }

  std::vector<std::pair<std::string, std::string>> m_filters;
  std::wstring m_defExtension;
  int m_defFilter;
  std::vector<WCHAR> m_filename;
  std::wstring m_title;
  bool m_save;
};

NativeDialogsWin32::NativeDialogsWin32()
{
}

FileDialog* NativeDialogsWin32::createFileDialog()
{
  return new FileDialogWin32();
}

} // namespace she