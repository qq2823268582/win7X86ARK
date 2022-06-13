// CIdtDlg.cpp: 实现文件
//

#include "pch.h"
#include "ARK.h"
#include "CIdtDlg.h"
#include "afxdialogex.h"
#include "ARKDlg.h"


extern HANDLE m_hDev;  //引用其它文件的全局变量


// CIdtDlg 对话框
IMPLEMENT_DYNAMIC(CIdtDlg, CDialogEx)
CIdtDlg::CIdtDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_IDT, pParent)
{

}

CIdtDlg::~CIdtDlg()
{
}

void CIdtDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}


BEGIN_MESSAGE_MAP(CIdtDlg, CDialogEx)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CIdtDlg::OnNMRClickList1)
	ON_COMMAND(ID_RENEWIDT, &CIdtDlg::OnRenewidt)
END_MESSAGE_MAP()


//CIdtDlg 消息处理程序
BOOL CIdtDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	//1.加载右键菜单
	m_Menu.LoadMenu(IDR_MENU4);

	//2.//给列表添加列
	CRect rect;
	m_list.GetClientRect(rect);	
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);	
	m_list.InsertColumn(0, L"中断号", 0, rect.right / 6);
	m_list.InsertColumn(1, L"段选择子", 0, rect.right / 6);
	m_list.InsertColumn(2, L"处理函数的地址", 0, rect.right / 6);
	m_list.InsertColumn(3, L"参数个数", 0, rect.right / 6);
	m_list.InsertColumn(4, L"特权级", 0, rect.right / 6);
	m_list.InsertColumn(5, L"类型", 0, rect.right / 6);

	//3.申请空间 //不用查询数量了，直接申请0x100个的空间
	PVOID pIdtInfo = malloc(sizeof(IDTINFO) * 0x100);
	
	//4.向0环发送请求遍历IDT
	EnumIdt(pIdtInfo);

	//5.强制转换类型
	PIDTINFO pbuffTemp = (PIDTINFO)pIdtInfo;

	//6.遍历IDT表
	for (int i = 0; i < 0x100; i++)
	{
		CString buffer;
		m_list.InsertItem(i, _T(""));

		//中断号
		buffer.Format(_T("%d"), i);
		m_list.SetItemText(i, 0, buffer);
		//段选择子
		buffer.Format(_T("%d"), pbuffTemp[i].Selector);
		m_list.SetItemText(i, 1, buffer);
		//处理函数的地址
		buffer.Format(_T("%08X"), pbuffTemp[i].pFunction);
		m_list.SetItemText(i, 2, buffer);
		//参数个数
		buffer.Format(_T("%d"), pbuffTemp[i].ParamCount);
		m_list.SetItemText(i, 3, buffer);
		//特权级
		buffer.Format(_T("%d"), pbuffTemp[i].Dpl);
		m_list.SetItemText(i, 4, buffer);
		//类型
		buffer.Format(_T("%d"), pbuffTemp[i].GateType);
		m_list.SetItemText(i, 5, buffer);
	}

	//7.释放空间
	free(pIdtInfo);

	return TRUE; 
}

//向0环发送请求遍历IDT表
VOID CIdtDlg::EnumIdt(PVOID pBuff)
{
	DWORD size = 0;

	DeviceIoControl(
		m_hDev,		//打开的设备句柄
		enumIdt,	//控制码
		NULL,		//输入缓冲区
		0,			//输入缓冲区大小
		pBuff,		//输出缓冲区
		sizeof(IDTINFO) * 0x100,	//输出缓冲区的大小
		&size,		//实际传输的字节数
		NULL		//是否是OVERLAPPED操作
	);

	return VOID();
}

//右键点击事件
void CIdtDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
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

//刷新IDT表
void CIdtDlg::OnRenewidt()
{
	//1.清空列表
	m_list.DeleteAllItems();

	//2.申请空间 //不用查询数量了，直接申请0x100个的空间
	PVOID pIdtInfo = malloc(sizeof(IDTINFO) * 0x100);
	memset(pIdtInfo, 0, sizeof(IDTINFO) * 0x100);

	//3.向0环发送请求遍历IDT
	EnumIdt(pIdtInfo);

	//4.强制转换类型
	PIDTINFO pbuffTemp = (PIDTINFO)pIdtInfo;

	//5.遍历IDT表
	for (int i = 0; i < 0x100; i++)
	{
		CString buffer;
		m_list.InsertItem(i, _T(""));
		//中断号
		buffer.Format(_T("%d"), i);
		m_list.SetItemText(i, 0, buffer);
		//段选择子
		buffer.Format(_T("%d"), pbuffTemp[i].Selector);
		m_list.SetItemText(i, 1, buffer);
		//处理函数的地址
		buffer.Format(_T("0x%08X"), pbuffTemp[i].pFunction);
		m_list.SetItemText(i, 2, buffer);
		//参数个数
		buffer.Format(_T("%d"), pbuffTemp[i].ParamCount);
		m_list.SetItemText(i, 3, buffer);
		//特权级
		buffer.Format(_T("%d"), pbuffTemp[i].Dpl);
		m_list.SetItemText(i, 4, buffer);
		//类型
		buffer.Format(_T("%d"), pbuffTemp[i].GateType);
		m_list.SetItemText(i, 5, buffer);
	}

	//6.释放空间
	free(pIdtInfo);
}
