
#pragma once

#include <Windows.h>
#include <ShlObj.h>
#include <process.h>
#include "FileGroup.h"
#include "resource.h"

void OpenFolder              (HWND hwnd, PTSTR pReceive);
int  GetListViewCheckedCount (HWND g_hList);
BOOL InitListViewColumns     (HWND g_hList);
int  DelDifferentHash        (HWND g_hList);
int  SelectSameHash          (HWND g_hList);
void InsertListViewItem(FileGroup::pFileInfo pfi, int groupid, std::wstring* phash = 0);
void SplitString             (const wchar_t *src,
                                 std::vector<std::wstring>& v,
                                 const wchar_t *key,
                                 const wchar_t  begin,
                                 const wchar_t  end,
                                 bool ignoreSpace
                                 );
bool IsSubString             (const wchar_t* child, const wchar_t* parent, int pos = 0);
bool HasParentDirectoryInList(HWND g_hList, const wchar_t *dir);
void EnableControls          (HWND hDlg, int id, bool enable);
void LoadFilterConfigToDialog(HWND hDlg, FileGroup& fg);
void UpdateFilterConfig      (HWND hDlg, FileGroup& fg);

void UpdateStatusBar(int part, const wchar_t *text);