#pragma once


// CExportDlg 对话框

class CExportDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CExportDlg)

public:
	CExportDlg(PIMAGE_DOS_HEADER pDosHeader, PIMAGE_OPTIONAL_HEADER32 pOptHeader, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CExportDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EXPORT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	CListCtrl m_list;

	virtual BOOL OnInitDialog();

	PIMAGE_DOS_HEADER pDosHeader;
	PIMAGE_OPTIONAL_HEADER32 pOptHeader;
	
	afx_msg DWORD RvaToFoa(IN PIMAGE_DOS_HEADER pDosHeader_Base, IN DWORD RVA);
};
