// CNotifyDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CNotifyDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"

extern HANDLE m_hDev;

// CNotifyDlg 对话框
IMPLEMENT_DYNAMIC(CNotifyDlg, CDialogEx)
CNotifyDlg::CNotifyDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NOTIFY, pParent)
{

}

CNotifyDlg::~CNotifyDlg()
{
}

void CNotifyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}


BEGIN_MESSAGE_MAP(CNotifyDlg, CDialogEx)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CNotifyDlg::OnNMRClickList1)
	ON_COMMAND(ID_RENEWNOTIFY, &CNotifyDlg::OnRenewnotify)
END_MESSAGE_MAP()


// CNotifyDlg 消息处理程序
BOOL CNotifyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1.加载右键菜单
	m_Menu.LoadMenu(IDR_MENU9);

	//2.给列表添加列
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, L"回调入口", 0, rect.right / 4);
	m_list.InsertColumn(1, L"回调类型", 0, rect.right / 4);
	m_list.InsertColumn(2, L"Cookie", 0, rect.right / 4);
	m_list.InsertColumn(2, L"回调路径", 0, rect.right / 4);
		
	//3.申请50个回调的空间
	ULONG nCount = 50;
	PVOID NotifyInfo = malloc(sizeof(NOTIFYINFO)*nCount);
	memset(NotifyInfo, 0, sizeof(NOTIFYINFO)*nCount);

	//4.向0环发送请求遍历系统回调
	EnumCallBack(NotifyInfo);
	
	//5.获取回调的个数		
	PNOTIFYINFO buffTemp1 = (PNOTIFYINFO)NotifyInfo;
	ULONG num = 0;
	while (buffTemp1->CallbackType)
	{
		num++;
		buffTemp1++;
	}
	
	//6.插入信息到List
	PNOTIFYINFO buffTemp2 = (PNOTIFYINFO)NotifyInfo;	
	int j = 0;
	for (ULONG i = 0; i < num; i++, j++)
	{
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//回调入口
		buffer.Format(_T("0x%08X"), buffTemp2[i].CallbacksAddr);
		m_list.SetItemText(j, 0, buffer);

		//回调类型
		switch (buffTemp2[i].CallbackType)
		{
		    case 1:
		    {
			m_list.SetItemText(j, 1, L"CreateProcess");
			break;
		     }
		    case 2:
		    {
			m_list.SetItemText(j, 1, L"CreateThread");
			break;
		    }
		    case 3:
		   {
			m_list.SetItemText(j, 1, L"LoadImage");
			break;
		   }
		   case 4:
		   {
			m_list.SetItemText(j, 1, L"CmpRegister");
			break;
		   }
		}
			
		//回调路径
		buffer.Format(_T("%ws"), buffTemp2[i].ImgPath);
		m_list.SetItemText(j, 2, buffer);

		//Cookie
		if (buffTemp2[i].CallbackType==4)
		{
			buffer.Format(_T("0x%08X"), buffTemp2[i].Cookie);
			m_list.SetItemText(j, 3, buffer);
		}
		else
		{
			m_list.SetItemText(j, 3, L" ");
		}			
	}
	//释放空间
	free(NotifyInfo);
	return TRUE;
}

//向0环发送请求遍历系统回调
VOID CNotifyDlg::EnumCallBack(PVOID pBuff)
{
	DWORD size = 0;
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumNotify,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(NOTIFYINFO) * 50,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}

//右键菜单
void CNotifyDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
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

//右键菜单1：刷新系统回调
void CNotifyDlg::OnRenewnotify()
{
	//1.删除所有的列表内容
	m_list.DeleteAllItems();

	//3.申请50个回调的空间
	ULONG nCount = 50;
	PVOID NotifyInfo = malloc(sizeof(NOTIFYINFO)*nCount);
	memset(NotifyInfo, 0, sizeof(NOTIFYINFO)*nCount);

	//4.向0环发送请求遍历系统回调
	EnumCallBack(NotifyInfo);

	//5.获取回调的个数		
	PNOTIFYINFO buffTemp1 = (PNOTIFYINFO)NotifyInfo;
	ULONG num = 0;
	while (buffTemp1->CallbackType)
	{
		num++;
		buffTemp1++;
	}

	//6.插入信息到List
	PNOTIFYINFO buffTemp2 = (PNOTIFYINFO)NotifyInfo;
	int j = 0;
	for (ULONG i = 0; i < num; i++, j++)
	{
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//回调入口
		buffer.Format(_T("0x%08X"), buffTemp2[i].CallbacksAddr);
		m_list.SetItemText(j, 0, buffer);

		//回调类型
		switch (buffTemp2[i].CallbackType)
		{
		case 1:
		{
			m_list.SetItemText(j, 1, L"CreateProcess");
			break;
		}
		case 2:
		{
			m_list.SetItemText(j, 1, L"CreateThread");
			break;
		}
		case 3:
		{
			m_list.SetItemText(j, 1, L"LoadImage");
			break;
		}
		case 4:
		{
			m_list.SetItemText(j, 1, L"CmpRegister");
			break;
		}
		}

		//回调路径
		buffer.Format(_T("%ws"), buffTemp2[i].ImgPath);
		m_list.SetItemText(j, 2, buffer);

		//Cookie
		if (buffTemp2[i].CallbackType == 4)
		{
			buffer.Format(_T("0x%08X"), buffTemp2[i].Cookie);
			m_list.SetItemText(j, 3, buffer);
		}
		else
		{
			m_list.SetItemText(j, 3, L" ");
		}
	}
	//释放空间
	free(NotifyInfo);	
}
