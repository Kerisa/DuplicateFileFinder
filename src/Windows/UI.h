#pragma once

#include <windows.h>
#include "filegroup.h"

extern HWND		g_hList, g_hStatus, g_hDlgFilter, g_hDlg;
extern HANDLE      g_ThreadSignal, g_hThread;
extern HINSTANCE	g_hInstance;
extern HMENU		g_hMenu;							 // 全局变量
extern Filter		g_Filter;
extern const wchar_t szAppName[20];

#define WM_FINISHFIND (WM_USER + 1)

// 过滤对话框
bool Cls_OnInitDialog_filter(HWND hDlg, HWND hwndFocus, LPARAM lParam);
bool Cls_OnCommand_filter(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
void Cls_OnClose(HWND hwnd);
void Cls_OnSysCommand(HWND hwnd, UINT cmd, int x, int y);

// 主窗口
void Cls_OnSize(HWND hwnd, UINT state, int cx, int cy);
LRESULT Cls_OnListViewNotify(HWND hDlg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
bool Cls_OnCommand_main(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
bool Cls_OnCreate(HWND hDlg, LPCREATESTRUCT lpCreateStruct);
void Cls_OnSysCommand0(HWND hwnd, UINT cmd, int x, int y);
void Cls_OnFinishFind(HWND hDlg);

