

#include "FileGroup.h"
#include "crc32.h"
#include <CommCtrl.h>
#include <memory>

extern HWND g_hStatus, g_hList;
extern volatile bool bTerminateThread;

extern void UpdateStatusBar(int part, const wchar_t *text);


std::vector<DataBaseInfo> g_DataBase;
std::vector<std::vector<FileRecord>> g_DataBase2;
