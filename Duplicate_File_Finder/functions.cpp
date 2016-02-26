#include "FileGroup.h"


extern HWND hList;

void ThreadEndRoutine(PBYTE lpVirMem, HGLOBAL hIndex)
{
	EnableWindow(GetDlgItem(GetParent(hList), IDB_BROWSE), TRUE);
	EnableWindow(GetDlgItem(GetParent(hList), IDB_SEARCH), TRUE);
	
	if (hIndex != INVALID_HANDLE_VALUE)
	{
		GlobalUnlock(hIndex);
		GlobalFree(hIndex);
	}
	if (lpVirMem && !VirtualFree(lpVirMem, 0, MEM_RELEASE))
	{
		MessageBox(0, TEXT("�ڴ�ռ��ͷŴ���"), 0, MB_ICONERROR);
		ExitProcess(1);
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////
// <DlgProc�е���>
// ����		- �������Ҫ���ҵ��ļ���
// hwnd		- �Ի��򴰿ھ��
// pReceive	- ���ջ�����
//////////////////////////////////////////////////////////////////////////////////////////////
void OpenFolder(HWND hwnd, PTSTR pReceive)
{
	BROWSEINFO		bi;
	LPITEMIDLIST	pIDList;
	TCHAR			Buffer[MAX_PATH1];

    assert(pReceive);

	RtlZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner		= hwnd;
	bi.pszDisplayName	= pReceive;
	bi.lpszTitle		= TEXT("ѡ��Ҫ�������ļ���");
	bi.ulFlags			= BIF_NONEWFOLDERBUTTON | BIF_SHAREABLE ;

	pIDList	= SHBrowseForFolder(&bi);
	if (pIDList)
	{
		SHGetPathFromIDList(pIDList, Buffer);
		lstrcpy(pReceive, Buffer);
	}
    else
        pReceive[0] = TEXT('\0');
	return;
}
//////////////////////////////////////////////////////////////////////////////////////////////
// <DlgProc�е���>
// ����		- ��ʼ��ListView����
// hList	- ListView���
//////////////////////////////////////////////////////////////////////////////////////////////
BOOL InitListViewColumns(HWND hList)
{
	int			i;
	LVCOLUMN	lvc;
	int			iWidth[6]  = {195, 195, 75, 130, 130, 340};
	TCHAR		szTitle[6][16] = {TEXT("�ļ���"), TEXT("·��"), TEXT("��С"),
							  TEXT("����ʱ��"), TEXT("�޸�ʱ��"), TEXT("��ϣֵ")};
	
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt	 = LVCFMT_LEFT;

	for (i=0; i<2; ++i)
	{
		lvc.iSubItem	= i;
		lvc.cx			= iWidth[i];
		lvc.pszText		= szTitle[i];
		lvc.cchTextMax	= sizeof(lvc.pszText) / sizeof(lvc.pszText[0]);
		if (ListView_InsertColumn(hList, i, &lvc) == -1)
			return FALSE;
	}
	lvc.fmt			= LVCFMT_RIGHT;			// ��С�Ҷ���
	lvc.iSubItem	= 2;
	lvc.cx			= iWidth[2];
	lvc.pszText		= szTitle[2];
	lvc.cchTextMax	= sizeof(lvc.pszText) / sizeof(lvc.pszText[0]);
	if (ListView_InsertColumn(hList, 2, &lvc) == -1)
		return FALSE;
	lvc.fmt	 = LVCFMT_LEFT;
	for (i=3; i<6; ++i)
	{
		lvc.iSubItem	= i;
		lvc.cx			= iWidth[i];
		lvc.pszText		= szTitle[i];
		lvc.cchTextMax	= sizeof(lvc.pszText) / sizeof(lvc.pszText[0]);
		if (ListView_InsertColumn(hList, i, &lvc) == -1)
			return FALSE;
	}
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////////
// <DlgProc�е���>
// ����		- �����µ��̲߳���ֵ
// hwnd		- �Ի��򴰿ھ��
// p		- �̲߳���ָ��
//////////////////////////////////////////////////////////////////////////////////////////////
//BOOL SetNewParam(HWND hwnd, PPARAMS p)
//{
//	p->dwHashSizeHigh	= 0;
//	p->dwHashSizeLow	= 0;
//	p->bFilter			= 50;
//	p->bOptions			= 0;
//	p->bTerminate		= FALSE;
//
//	if (IsDlgButtonChecked(hwnd, IDC_SAMENAME))
//		p->bOptions |= 0x1;
//	if (IsDlgButtonChecked(hwnd, IDC_SAMESIZE))
//		p->bOptions |= 0x2;
//	if (IsDlgButtonChecked(hwnd, IDC_SAMETIME))
//		p->bOptions |= 0x4;
//	if (IsDlgButtonChecked(hwnd, IDC_NEEDHASH))
//	{
//		p->bOptions	|= 0x8;
//		p->dwHashSizeLow = GetDlgItemInt(hwnd, IDC_FILESIZE, NULL, TRUE);
//		if (p->dwHashSizeLow > 4096)	// 4GB
//		{
//			p->dwHashSizeHigh = (p->dwHashSizeLow - 4096);
//			p->dwHashSizeLow  = (p->dwHashSizeLow - 4096) * 0x100000;		// 1024*1024
//		}
//		else
//		{
//			p->dwHashSizeHigh	= 0;
//			p->dwHashSizeLow   *= 0x100000;
//		}
//	}
//	//���ƹ�����
//	p->bFilter = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
//	if (p->bFilter == CB_ERR)	// -1, �Զ���
//	{
//		GetDlgItemText(hwnd, IDC_TYPE, szFilter, MAX_PATH);
//	}
//	return TRUE;
//}
//////////////////////////////////////////////////////////////////////////////////////////////
// <DlgProc�е���>
// ����		- ����������б����ϣ��ͬ���ļ���
// hList	- ListView���
//////////////////////////////////////////////////////////////////////////////////////////////
int DelDifferentHash(HWND hList)
{
#define BufSize 128
	TCHAR szBuffer[BufSize], szBuffer1[BufSize];
	int iItemCount, iDelCount, i, j;

	iDelCount = 0;
	iItemCount = ListView_GetItemCount(hList);

	for (i=j=0; j<iItemCount;)
	{
		j = i + 1;
		ListView_GetItemText(hList, i, 5, szBuffer, BufSize);
		if (!szBuffer[0])
		{
			++i;
			continue;
		}
		do
		{
			if (j == iItemCount)		// ɾ�����һ���������ļ�
				goto _DELETE;
			ListView_GetItemText(hList, j, 5, szBuffer1, BufSize);
		
			if (!lstrcmp(szBuffer, szBuffer1))			// ��С��ͬ��ϣ��Ȼ��ͬ����˿���ɾ��
				++j;
			else
			{
				if (j == i+1)
				{
_DELETE:			ListView_DeleteItem(hList, i);
					++iDelCount;
					--j;
					--iItemCount;
				}
				else
					i = j;
				break;
			}
		}while (j<iItemCount);
	}
	return iDelCount;
}
//////////////////////////////////////////////////////////////////////////////////////////////
// <DlgProc / Thread�е���>
// ����		- ��ѡ����б�����ͬ��ϣ���ظ��ļ�
// ����ֵ	- ��ѡ����Ŀ
// hList	- ListView���
//////////////////////////////////////////////////////////////////////////////////////////////
int SelectSameHash(HWND hList)
{
#define BufSize 128
	TCHAR szBuffer[BufSize], szBuffer1[BufSize];
	int i, iItemCount, iCheckCount;

	iItemCount = ListView_GetItemCount(hList);
	for (i=0; i<iItemCount; ++i)
		ListView_SetCheckState(hList, i, FALSE);

	for (i=0,iCheckCount=0; i<iItemCount-1;)
	{
		ListView_GetItemText(hList, i, 5, szBuffer, BufSize);
		ListView_GetItemText(hList, i+1, 5, szBuffer1, BufSize);
		if (!szBuffer[0] || !szBuffer1[0] || lstrcmp(szBuffer, szBuffer1))
		{
			++i;
			continue;
		}	
		do
		{
			ListView_SetCheckState(hList, ++i, TRUE);
			++iCheckCount;
			ListView_GetItemText(hList, i+1, 5, szBuffer1, BufSize);
		}while(i<iItemCount-1 && !lstrcmp(szBuffer, szBuffer1));
	}
	return iCheckCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// <FindFile�е���>
// ����			- ���ݹ�����ƥ���ļ�
// ����ֵ		- TRUE��ƥ�䣻 FALSE����ƥ��
// szFileName	- �ļ���
// cType		- ���������ͣ�Ϊ5ʱ��ȫ�ֱ���szFilter�л�ȡ�ļ���׺��ֻ��дһ�֣�
//////////////////////////////////////////////////////////////////////////////////////////////
//BOOL IsFileMatched1(PTSTR szFileName, char cType)
//{
//	static TCHAR szFilterMuisc[9][10] = {TEXT("mp3"), TEXT("wav"), TEXT("flac"), TEXT("ape"), TEXT("wma"),
//										 TEXT("ogg"), TEXT("m4a"), TEXT("m4p"), TEXT("aac")};
//	static TCHAR szFilterVideo[9][10] = {TEXT("mkv"), TEXT("mp4"), TEXT("avi"), TEXT("rmvb"), TEXT("wmv"),
//										 TEXT("mpeg"), TEXT("mpg"), TEXT("mov"), TEXT("rm")};
//	static TCHAR szFilterPicture[8][10] = {TEXT("jpg"), TEXT("jpeg"), TEXT("png"), TEXT("bmp"), TEXT("gif"),
//										 TEXT("ico"), TEXT("psd"), TEXT("raw")};
//	static TCHAR szFilterDocument[11][10] = {TEXT("txt"), TEXT("pdf"), TEXT("doc"), TEXT("docx"), TEXT("ppt"),
//										 TEXT("pptx"), TEXT("xls"), TEXT("xlsx"), TEXT("html"), TEXT("htm"), TEXT("mht")};
//	int  i;
//	PTSTR p;
//
//	if (cType == 0x0) return 1;		// ��������
//
//	for (i=lstrlen(szFileName); i>=0 && szFileName[i]!='.'; --i);
//	p = szFileName + (++i);
//
//	switch (cType)
//	{
//	case 0x1:			// ����
//		for (i=0; i<9; ++i)
//			if (!lstrcmp(p, szFilterMuisc[i]))
//				break;
//		if (i == 9) return 0;
//		else return 1;
//
//	case 0x2:
//		for (i=0; i<9; ++i)
//			if (!lstrcmp(p, szFilterVideo[i]))
//				break;
//		if (i == 9) return 0;
//		else return 1;
//
//	case 0x3:
//		for (i=0; i<8; ++i)
//			if (!lstrcmp(p, szFilterPicture[i]))
//				break;
//		if (i == 8) return 0;
//		else return 1;
//
//	case 0x4:
//		for (i=0; i<11; ++i)
//			if (!lstrcmp(p, szFilterDocument[i]))
//				break;
//		if (i == 11) return 0;
//		else return 1;
//
//	//case CB_ERR:	// -1
//	//	return (!lstrcmp(p, szFilter));
//	}
//	return 0;
//}

//////////////////////////////////////////////////////////////////////////////////////////////
// 4��cmp������ �Լ����ź������Ӻ���
//////////////////////////////////////////////////////////////////////////////////////////////
//typedef int (*FNCMP)(PFILEINFO, PFILEINFO);
//
//int cmphash(PFILEINFO a, PFILEINFO b)
//{
//	return lstrcmp((LPTSTR)a->SHA1, (LPTSTR)b->SHA1);
//}
//
//int cmpname(PFILEINFO a, PFILEINFO b)
//{
//	return lstrcmp(a->wfd.cFileName,b->wfd.cFileName);
//}
//
//int cmpsize(PFILEINFO a, PFILEINFO b)
//{
//	if (a->wfd.nFileSizeHigh == b->wfd.nFileSizeHigh)
//	{
//		return a->wfd.nFileSizeLow - b->wfd.nFileSizeLow;
//	}
//	else
//	{
//		return a->wfd.nFileSizeHigh - b->wfd.nFileSizeHigh;
//	}
//}
//
//int cmptime(PFILEINFO a, PFILEINFO b)
//{
//	if (a->wfd.ftLastWriteTime.dwHighDateTime == b->wfd.ftLastWriteTime.dwHighDateTime)
//	{
//		return a->wfd.ftLastWriteTime.dwLowDateTime - b->wfd.ftLastWriteTime.dwLowDateTime;
//	}
//	else
//	{
//		return a->wfd.ftLastWriteTime.dwHighDateTime - b->wfd.ftLastWriteTime.dwHighDateTime;
//	}
//}
//
//void sort(int a[],int l,int r, FNCMP fncmp)	/*ѡ������*/
//{
//	int i,j,k,t,flag=0;
//	for (i=l;i<r;i++)
//	{
//		k=i;
//		for (j=i+1;j<r;j++)
//			if ( fncmp((PFILEINFO)(a[k]),(PFILEINFO)(a[j])) > 0)
//				k=j;
//		if (k!=i)
//		{
//			t=a[i]; a[i]=a[k]; a[k]=t;
//		}
//	}
//}
//
//int partition(int a[],int l,int r, FNCMP fncmp)
//{
//	int i=l-1,j=r,v=a[r],tmp;
//	for (;;)
//	{
//		while( fncmp((PFILEINFO)(a[++i]),(PFILEINFO)v) < 0);
//		while( fncmp((PFILEINFO)(a[--j]),(PFILEINFO)v) > 0) if (j<=l) break;
//		if (i>=j) break;
//		tmp=a[i];	a[i]=a[j];	a[j]=tmp;
//	}
//	tmp=a[i];	a[i]=a[r];	a[r]=tmp;
//	return i;
//}
//
//void quicksort(int a[],int l,int r, FNCMP fncmp)
//{
//	int i;
//	if (r-l<=7) {sort(a,l,r+1, fncmp); return;}
//	i=partition(a,l,r, fncmp);
//	quicksort(a,l,i-1, fncmp);
//	quicksort(a,i+1,r, fncmp);
//}
//////////////////////////////////////////////////////////////////////////////////////////////
// <Thread�е���>
// ����		- �����������ļ��� �ļ���/·��/��С/����ʱ��/�޸�ʱ��/��ϣֵ �����ListView
// hList	- ListView�ؼ����
// pfi		- ����FILEINFO�ṹ�� �������ļ���Ϣ
// index	- Ҫ���뵽ListView����һ��
//////////////////////////////////////////////////////////////////////////////////////////////
void pInsertListViewItem(HWND hList, FileGroup::pFileInfo pfi, std::wstring* phash, DWORD index)			// ������ʾ : LVIF_GROUPID
{
	TCHAR		Buffer[128];
	LVITEM		lvI;

	RtlZeroMemory(&lvI, sizeof(LVITEM));
	lvI.iItem		= index;
	lvI.mask		= LVIF_TEXT;
	lvI.cchTextMax	= MAX_PATH1;

	lvI.iSubItem = 0;
    lvI.pszText  = const_cast<wchar_t*>(pfi->Name.c_str());
	ListView_InsertItem(hList, &lvI);			// �ȶ�subitem=0ʹ��InsertItem

	lvI.iSubItem = 1;
    lvI.pszText	 = const_cast<wchar_t*>(pfi->Path.c_str());					// ��subitem>0ʹ��SetItem
	ListView_SetItem(hList, &lvI);
	
    lvI.iSubItem = 2;
    if (pfi->Size > 966367641)		// ԼΪ0.9GB
	{
		StringCbPrintf(Buffer, 120, TEXT("%.2lf GB"), pfi->Size/(double)0x40000000);
	}
	else if (pfi->Size > 943718)	// 0.9MB
	{
		StringCbPrintf(Buffer, 120, TEXT("%.2lf MB"), pfi->Size / (double)0x100000);
	}
    else if (pfi->Size > 921)        // 0.9KB
	{
		StringCbPrintf(Buffer, 120, TEXT("%.2lf KB"), pfi->Size / (double)0x400);
	}
    else
    {
        StringCbPrintf(Buffer, 120, TEXT("%d B"), pfi->Size);
    }
	lvI.pszText  = Buffer;
	ListView_SetItem(hList, &lvI);

	//lvI.iSubItem = 3;
	//FileTimeToLocalFileTime(&pfi->wfd.ftCreationTime, &lft);
	//FileTimeToSystemTime(&lft, &st);
	//StringCbPrintf(Buffer, 120, TEXT("%d/%d/%d %d:%d:%d"),
	//				st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	//lvI.pszText  = Buffer;
	//ListView_SetItem(hList, &lvI);

	lvI.iSubItem = 4;
    FILETIME ft;
    SystemTimeToFileTime(&pfi->st, &ft);
	FileTimeToLocalFileTime(&ft, &ft);
	FileTimeToSystemTime(&ft, &pfi->st);
	StringCbPrintf(Buffer, 120, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
        pfi->st.wYear, pfi->st.wMonth, pfi->st.wDay, pfi->st.wHour, pfi->st.wMinute, pfi->st.wSecond);
	lvI.pszText  = Buffer;
	ListView_SetItem(hList, &lvI);

	lvI.iSubItem = 5;
	lvI.pszText  = phash ? (LPTSTR)phash->c_str() : 0;
	ListView_SetItem(hList, &lvI);
}

void InsertListViewItem(FileGroup::pFileInfo pfi, std::wstring* phash)
{
    pInsertListViewItem(hList, pfi, phash, 0);
}