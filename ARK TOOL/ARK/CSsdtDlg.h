#pragma once

//SSDT信息结构体
typedef struct _SSDTINFO
{
	ULONG uIndex;	    //回调号
	ULONG uFuntionAddr;	//函数地址
}SSDTINFO, *PSSDTINFO;

// CSsdtDlg 对话框
class CSsdtDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSsdtDlg)

public:
	CSsdtDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSsdtDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SSDT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_list;
	CMenu m_Menu;
	virtual BOOL OnInitDialog();
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRenewssdt();
	ULONG GetSsdtCount();
	VOID  EnumSsdt(PVOID pBuff, ULONG SsdtCount);
};
