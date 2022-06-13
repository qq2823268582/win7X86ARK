#pragma once


// CThreadDlg 对话框

class CThreadDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CThreadDlg)

public:
	CThreadDlg(ULONG PID, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CThreadDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_THREAD };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()


public:
	CListCtrl m_list;
	ULONG m_PID;	//进程PID
	
	virtual BOOL OnInitDialog();
	ULONG GetThreadCount(PULONG pPID);
	VOID  EnumThread(PULONG pPID, PVOID pBuff, ULONG ThreadCount);
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);	
	afx_msg void OnRenewthread();
	
	//CMenu m_menu6;
	//INT pEthread;
	//afx_msg void OnHidethread();
	//VOID Hidethread(IN int PETHREAD);
};
