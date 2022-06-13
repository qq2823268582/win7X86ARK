#pragma once


//驱动信息结构体
typedef struct _DRIVERINFO
{
	WCHAR wcDriverBasePath[MAX_PATH];	//驱动名
	WCHAR wcDriverFullPath[MAX_PATH];	//驱动路径
	ULONG DllBase;		//加载基址
	ULONG SizeOfImage;	//大小
}DRIVERINFO, *PDRIVERINFO;


// CDriverDlg 对话框
class CDriverDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDriverDlg)

public:
	CDriverDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDriverDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DRIVER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()


public:
	CListCtrl m_list;
	CMenu m_Menu;
	virtual BOOL OnInitDialog();

	afx_msg ULONG GetDriverCount();
	afx_msg VOID EnumDriver(PVOID pBuff, ULONG DriverCount);
	afx_msg VOID HideDriverInfo(WCHAR* pDriverName);
	afx_msg void OnRenew2();
	afx_msg void OnHidedriver();
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
};
