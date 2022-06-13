// CIotimerDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CIotimerDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"

extern HANDLE m_hDev;

// CIotimerDlg 对话框

IMPLEMENT_DYNAMIC(CIotimerDlg, CDialogEx)

CIotimerDlg::CIotimerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_IOTIMER, pParent)
{

}

CIotimerDlg::~CIotimerDlg()
{
}

void CIotimerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}


BEGIN_MESSAGE_MAP(CIotimerDlg, CDialogEx)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CIotimerDlg::OnNMRClickList1)
	ON_COMMAND(ID_RENEWIOTIMER, &CIotimerDlg::OnRenewiotimer)
END_MESSAGE_MAP()


// CIotimerDlg 消息处理程序
BOOL CIotimerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1.加载右键菜单
	m_Menu.LoadMenu(IDR_MENU12);

	//2.给列表添加列
	CRect rect;
	m_list.GetClientRect(rect);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, L"Timer地址", 0, rect.right / 7);
	m_list.InsertColumn(1, L"类型", 0, rect.right / 7);
	m_list.InsertColumn(2, L"Flag", 0, rect.right / 7);
	m_list.InsertColumn(3, L"Device", 0, rect.right / 7);
	m_list.InsertColumn(4, L"Context", 0, rect.right / 7);
	m_list.InsertColumn(5, L"函数入口地址", 0, rect.right / 7);
	m_list.InsertColumn(6, L"所属模块", 0, rect.right / 7);

	//3.申请50个回调的空间
	ULONG nCount = 50;
	PVOID IotimerInfo = malloc(sizeof(IOTIMERINFO)*nCount);
	memset(IotimerInfo, 0, sizeof(IOTIMERINFO)*nCount);

	//4.向0环发送请求遍历系统回调
	EnumIOTIMER(IotimerInfo);

	//5.获取回调的个数		
	PIOTIMERINFO buffTemp1 = (PIOTIMERINFO)IotimerInfo;
	ULONG num = 0;
	while (buffTemp1->TimerAddress)
	{
		num++;
		buffTemp1++;
	}

	//6.插入信息到List
	PIOTIMERINFO buffTemp2 = (PIOTIMERINFO)IotimerInfo;
	int j = 0;
	for (ULONG i = 0; i < num; i++, j++)
	{
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//1.Timer地址
		buffer.Format(_T("0x%08X"), buffTemp2[i].TimerAddress);
		m_list.SetItemText(j, 0, buffer);

		//2.类型
		buffer.Format(_T("0x%04X"), buffTemp2[i].Type);
		m_list.SetItemText(j, 1, buffer);

		//3.Flag
		buffer.Format(_T("0x%04X"), buffTemp2[i].Flag);
		m_list.SetItemText(j, 2, buffer);

		//4.Device
		buffer.Format(_T("0x%08X"), buffTemp2[i].Device);
		m_list.SetItemText(j, 3, buffer);

		//5.Context
		buffer.Format(_T("0x%08X"), buffTemp2[i].Context);
		m_list.SetItemText(j, 4, buffer);

		//6.回调函数入口
		buffer.Format(_T("0x%08X"), buffTemp2[i].FuncAddress);
		m_list.SetItemText(j, 5, buffer);

		//7.所属模块
		if (buffTemp2[i].ImgPath)
		{
			buffer.Format(_T("%ws"), buffTemp2[i].ImgPath);
			m_list.SetItemText(j, 6, buffer);
		}
		else
		{
			m_list.SetItemText(j, 6, L" ");
		}
	}
	//释放空间
	free(IotimerInfo);

	return TRUE;
}

//向0环发送请求遍历系统回调
VOID CIotimerDlg::EnumIOTIMER(PVOID pBuff)
{
	DWORD size = 0;
	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumIotimer,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(IOTIMERINFO) * 50,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}

void CIotimerDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
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

void CIotimerDlg::OnRenewiotimer()
{
	//1.删除所有的列表内容
	m_list.DeleteAllItems();

	//3.申请50个回调的空间
	ULONG nCount = 50;
	PVOID IotimerInfo = malloc(sizeof(IOTIMERINFO)*nCount);
	memset(IotimerInfo, 0, sizeof(IOTIMERINFO)*nCount);

	//4.向0环发送请求遍历系统回调
	EnumIOTIMER(IotimerInfo);

	//5.获取回调的个数		
	PIOTIMERINFO buffTemp1 = (PIOTIMERINFO)IotimerInfo;
	ULONG num = 0;
	while (buffTemp1->TimerAddress)
	{
		num++;
		buffTemp1++;
	}

	//6.插入信息到List
	PIOTIMERINFO buffTemp2 = (PIOTIMERINFO)IotimerInfo;
	int j = 0;
	for (ULONG i = 0; i < num; i++, j++)
	{
		CString buffer;
		m_list.InsertItem(j, _T(""));

		//1.Timer地址
		buffer.Format(_T("0x%08X"), buffTemp2[i].TimerAddress);
		m_list.SetItemText(j, 0, buffer);

		//2.类型
		buffer.Format(_T("0x%04X"), buffTemp2[i].Type);
		m_list.SetItemText(j, 1, buffer);

		//3.Flag
		buffer.Format(_T("0x%04X"), buffTemp2[i].Flag);
		m_list.SetItemText(j, 2, buffer);

		//4.Device
		buffer.Format(_T("0x%08X"), buffTemp2[i].Device);
		m_list.SetItemText(j, 3, buffer);

		//5.Context
		buffer.Format(_T("0x%08X"), buffTemp2[i].Context);
		m_list.SetItemText(j, 4, buffer);

		//6.回调函数入口
		buffer.Format(_T("0x%08X"), buffTemp2[i].FuncAddress);
		m_list.SetItemText(j, 5, buffer);

		//7.所属模块
		if (buffTemp2[i].ImgPath)
		{
			buffer.Format(_T("%ws"), buffTemp2[i].ImgPath);
			m_list.SetItemText(j, 6, buffer);
		}
		else
		{
			m_list.SetItemText(j, 6, L" ");
		}
	}
	//释放空间
	free(IotimerInfo);
}
