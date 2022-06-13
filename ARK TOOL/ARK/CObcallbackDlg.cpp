// CObcallbackDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CObcallbackDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"

extern HANDLE m_hDev;

// CObcallbackDlg 对话框
IMPLEMENT_DYNAMIC(CObcallbackDlg, CDialogEx)
CObcallbackDlg::CObcallbackDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_OBCALLBACK, pParent)
{

}

CObcallbackDlg::~CObcallbackDlg()
{
}

void CObcallbackDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CObcallbackDlg, CDialogEx)
	ON_COMMAND(ID_RENEWOBCALLBACK, &CObcallbackDlg::OnRenewobcallback)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CObcallbackDlg::OnNMRClickList1)
END_MESSAGE_MAP()

// CObcallbackDlg 消息处理程序
BOOL CObcallbackDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	//1.加载右键菜单
	m_Menu.LoadMenu(IDR_MENU10);

	//2.给列表添加列
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, L"回调对象地址", 0, rect.right / 6);
	m_list.InsertColumn(1, L"回调类型", 0, rect.right / 6);
	m_list.InsertColumn(2, L"Pre/Post", 0, rect.right / 6);
	m_list.InsertColumn(3, L"Altitude", 0, rect.right / 6);
	m_list.InsertColumn(4, L"函数入口地址", 0, rect.right / 6);
	m_list.InsertColumn(5, L"所属模块", 0, rect.right / 6);

	//3.申请50个回调的空间
	ULONG nCount = 50;
	PVOID ObcallbackInfo = malloc(sizeof(OBCALLBACKINFO)*nCount);
	memset(ObcallbackInfo, 0, sizeof(OBCALLBACKINFO)*nCount);

	//4.向0环发送请求遍历系统回调
	EnumOBCallBack(ObcallbackInfo);

	//5.获取回调的个数		
	POBCALLBACKINFO buffTemp1 = (POBCALLBACKINFO)ObcallbackInfo;
	ULONG num = 0;
	while (buffTemp1->ObjectAddress)
	{
		num++;
		buffTemp1++;
	}

	//6.插入信息到List
	POBCALLBACKINFO buffTemp2 = (POBCALLBACKINFO)ObcallbackInfo;
	int j = 0;
	for (ULONG i = 0; i < num; i++, j++)
	{
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//1.回调对象地址
		buffer.Format(_T("0x%08X"), buffTemp2[i].ObjectAddress);
		m_list.SetItemText(j, 0, buffer);

		//2.回调类型
		switch (buffTemp2[i].ObjectType)
		{
		case 1:
		{
			m_list.SetItemText(j, 1, L"进程");
			break;
		}
		case 2:
		{
			m_list.SetItemText(j, 1, L"线程");
			break;
		}		
		}

		//3.前/后回调
		switch (buffTemp2[i].CallType)
		{
		case 1:
		{
			m_list.SetItemText(j, 2, L"precall");
			break;
		}
		case 2:
		{
			m_list.SetItemText(j, 2, L"postcall");
			break;
		}
		}

		//4.Altitude
		buffer.Format(_T("%ws"), buffTemp2[i].Altitude);
		m_list.SetItemText(j, 3, buffer);

		//5.回调函数入口
		buffer.Format(_T("0x%08X"), buffTemp2[i].FuncAddress);
		m_list.SetItemText(j, 4, buffer);

		//6.所属模块
		if (buffTemp2[i].ImgPath )
		{
			buffer.Format(_T("%ws"), buffTemp2[i].ImgPath);
			m_list.SetItemText(j, 5, buffer);
		}
		else
		{
			m_list.SetItemText(j, 5, L" ");
		}
	}
	//释放空间
	free(ObcallbackInfo);

	return TRUE;
}


//向0环发送请求遍历系统回调
VOID CObcallbackDlg::EnumOBCallBack(PVOID pBuff)
{
	DWORD size = 0;
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumObcallback,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(OBCALLBACKINFO) * 50,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}


//右键菜单1：刷新
void CObcallbackDlg::OnRenewobcallback()
{
	//1.删除所有的列表内容
	m_list.DeleteAllItems();

	//3.申请50个回调的空间
	ULONG nCount = 50;
	PVOID ObcallbackInfo = malloc(sizeof(OBCALLBACKINFO)*nCount);
	memset(ObcallbackInfo, 0, sizeof(OBCALLBACKINFO)*nCount);

	//4.向0环发送请求遍历系统回调
	EnumOBCallBack(ObcallbackInfo);

	//5.获取回调的个数		
	POBCALLBACKINFO buffTemp1 = (POBCALLBACKINFO)ObcallbackInfo;
	ULONG num = 0;
	while (buffTemp1->ObjectAddress)
	{
		num++;
		buffTemp1++;
	}

	//6.插入信息到List
	POBCALLBACKINFO buffTemp2 = (POBCALLBACKINFO)ObcallbackInfo;
	int j = 0;
	for (ULONG i = 0; i < num; i++, j++)
	{
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//1.回调对象地址
		buffer.Format(_T("0x%08X"), buffTemp2[i].ObjectAddress);
		m_list.SetItemText(j, 0, buffer);

		//2.回调类型
		switch (buffTemp2[i].ObjectType)
		{
		case 1:
		{
			m_list.SetItemText(j, 1, L"进程");
			break;
		}
		case 2:
		{
			m_list.SetItemText(j, 1, L"线程");
			break;
		}
		}

		//3.前/后回调
		switch (buffTemp2[i].CallType)
		{
		case 1:
		{
			m_list.SetItemText(j, 2, L"precall");
			break;
		}
		case 2:
		{
			m_list.SetItemText(j, 2, L"postcall");
			break;
		}
		}

		//4.Altitude
		buffer.Format(_T("%ws"), buffTemp2[i].Altitude);
		m_list.SetItemText(j, 3, buffer);

		//5.回调函数入口
		buffer.Format(_T("0x%08X"), buffTemp2[i].FuncAddress);
		m_list.SetItemText(j, 4, buffer);

		//6.所属模块
		if (buffTemp2[i].ImgPath)
		{
			buffer.Format(_T("%ws"), buffTemp2[i].ImgPath);
			m_list.SetItemText(j, 5, buffer);
		}
		else
		{
			m_list.SetItemText(j, 5, L" ");
		}
	}
	//释放空间
	free(ObcallbackInfo);
}

//右键点击
void CObcallbackDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (m_list.GetSelectionMark() != -1)
	{
		CMenu *nMenu = m_Menu.GetSubMenu(0);
		POINT pos;
		GetCursorPos(&pos);
		nMenu->TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, this);
	}
	*pResult = 0;
}
