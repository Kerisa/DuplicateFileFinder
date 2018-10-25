

#include "FileGroup.h"

extern HWND g_hStatus, g_hList;
extern volatile bool bTerminateThread;

extern void UpdateStatusBar(int part, const wchar_t *text);


std::vector<DataBaseInfo> g_DataBase;
std::vector<std::vector<FileRecord>> g_DataBase2;

std::wstring Filter::MakeCompareCommand()
{
	std::wstring cmd(L"DoubleChecker.exe ");
	if (Switch[Filter::Compare_FileName] != Filter::Type_Off)
	{
		if (Switch[Filter::Compare_FileName] == Filter::Type_Whole)
			cmd += L"-n ";
		else
			cmd += L"-ns " + mKeyWordOfFileName;
	}

	if (Switch[Filter::Compare_FileSize] != Filter::Type_Off)
		cmd += L"-s ";

	if (Switch[Filter::Compare_FileDate] != Filter::Type_Off)
		cmd += L"-t ";

	if (Switch[Filter::Compare_FileHash] != Filter::Type_Off)
		cmd += L"-c ";

	return cmd;
}

std::vector<std::wstring> Filter::MakeSearchCommand()
{
	if (mSearchDirectory.empty())
		return std::vector<std::wstring>();

	std::wstring cmd;
	if (Switch[Filter::Search_Suffix] != Filter::Type_Off && !mSuffix.empty())
	{
		if (Switch[Filter::Search_Suffix] == Filter::Type_Include)
			cmd += L"-type +(";
		else
			cmd += L"-type -(";
		for (auto& s : mSuffix)
		{
			cmd += s;
			cmd += L",";
		}
		cmd.pop_back();
		cmd += L") ";
	}

	if (Switch[Filter::Search_FileSize] != Filter::Type_Off)
	{
		TCHAR buf[128];
		swprintf_s(buf, _countof(buf), L"-size (%llu:%llu) ", FileSizeRangeLow, FileSizeRangeHigh);
		cmd += buf;
	}

	if (Switch[Filter::Search_FileAttribute] != Filter::Type_Off && SelectedAttributes != 0)
	{
		std::wstring tmp;
		if (SelectedAttributes & Filter::Attrib_Archive)
			tmp += L'a';
		if (SelectedAttributes & Filter::Attrib_ReadOnly)
			tmp += L'r';
		if (SelectedAttributes & Filter::Attrib_Hide)
			tmp += L'h';
		if (SelectedAttributes & Filter::Attrib_System)
			tmp += L's';
		if (SelectedAttributes & Filter::Attrib_Normal)
			tmp += L'n';

		if (Switch[Filter::Search_FileAttribute] == Filter::Type_Include)
			cmd += L"-attr +(";
		else
			cmd += L"-attr -(";
		cmd += tmp;
		cmd += L") ";
	}

	std::vector<std::wstring> cmds;
	for (auto& it : mSearchDirectory)
	{
		cmds.push_back(std::wstring(L"Finder.exe ") + it.first + L" " + cmd);
	}
	return cmds;
}
