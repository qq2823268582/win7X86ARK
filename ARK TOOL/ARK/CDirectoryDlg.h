#pragma once


// CDirectoryDlg 对话框

class CDirectoryDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDirectoryDlg)

public:
	CDirectoryDlg(PIMAGE_OPTIONAL_HEADER32 pOptHeader,CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDirectoryDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIRECTORY };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	PIMAGE_OPTIONAL_HEADER32 pOptHeader;
	virtual BOOL OnInitDialog();

};
