
#pragma once

#include <memory>
#include <string>
#include <Windows.h>
#include <ShlObj.h>
#include <process.h>
#include <vector>
#include "FileGroup.h"
#include "resource.h"


extern std::pair<wchar_t*, uint64_t> szUnit[4];


extern std::vector<DataBaseInfo> g_DataBase;


std::wstring OpenFolder			(HWND hwnd);
int  GetListViewCheckedCount    (HWND g_hList);
BOOL InitListViewColumns        (HWND g_hList);
void SplitString                (const wchar_t *src,
                                 std::vector<std::wstring>& v,
                                 const wchar_t *key,
                                 const wchar_t  begin,
                                 const wchar_t  end,
                                 bool ignoreSpace
                                 );
bool HasParentDirectoryInList   (HWND g_hList, const std::wstring& dir);
void EnableControls             (HWND hDlg, int id, bool enable);
void LoadFilterConfigToDialog   (HWND hDlg, Filter& fg);
void UpdateFilterConfig         (HWND hDlg, Filter& fg);

void UpdateStatusBar            (int part, const wchar_t *text);