#pragma once


// CVadDlg 对话框

class CVadDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CVadDlg)

public:
	CVadDlg(ULONG PID, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CVadDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VAD };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()


public:
	ULONG m_PID;	//进程PID
	CListCtrl m_list;

	virtual BOOL OnInitDialog();
	ULONG  GetVADCount(PULONG pPID);
	VOID  EnumVAD(PULONG pPID, PVOID pBuff, ULONG VadCount);	
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRenewvad();
};
