#pragma once


// CImportDlg 对话框

class CImportDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CImportDlg)

public:
	CImportDlg(PIMAGE_DOS_HEADER pDosHeader, PIMAGE_OPTIONAL_HEADER32 pOptHeader, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CImportDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IMPORT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	CListCtrl m_list1;
	CListCtrl m_list2;
	virtual BOOL OnInitDialog();

	PIMAGE_DOS_HEADER pDosHeader;
	PIMAGE_OPTIONAL_HEADER32 pOptHeader;
	
	afx_msg VOID showDLL(int Original);

	afx_msg DWORD RvaToFoa(IN PIMAGE_DOS_HEADER pDosHeader_Base, IN DWORD RVA);
	afx_msg int GetImportCount();
	afx_msg int GetINTCount(PIMAGE_THUNK_DATA pINT);
	afx_msg void OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult);
};
