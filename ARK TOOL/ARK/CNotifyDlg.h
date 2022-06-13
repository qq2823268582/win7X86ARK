#pragma once


typedef struct _NOTIFYINFO
{
	ULONG	CallbackType;
	ULONG	CallbacksAddr;
	ULONGLONG	Cookie;           
	WCHAR	ImgPath[MAX_PATH];
}NOTIFYINFO,*PNOTIFYINFO;

// CNotifyDlg 对话框
class CNotifyDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CNotifyDlg)

public:
	CNotifyDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CNotifyDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NOTIFY };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	CListCtrl m_list;
	CMenu m_Menu;
	
	virtual BOOL OnInitDialog();	
	VOID EnumCallBack(PVOID pBuff);
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRenewnotify();
};
