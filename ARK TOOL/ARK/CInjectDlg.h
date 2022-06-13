#pragma once

typedef struct _INJECT_INFO
{
	ULONG ProcessId;
	wchar_t DllName[1024];
}INJECT_INFO, *PINJECT_INFO;

// CInjectDlg 对话框
class CInjectDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CInjectDlg)

public:
	CInjectDlg(ULONG PID, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CInjectDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_INJECT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_1;
	ULONG m_PID;	//进程PID

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg VOID InjectProcess();
};
