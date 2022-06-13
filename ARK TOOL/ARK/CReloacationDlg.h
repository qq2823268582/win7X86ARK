#pragma once


// CReloacationDlg 对话框

class CReloacationDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CReloacationDlg)

public:
	CReloacationDlg(PIMAGE_DOS_HEADER pDosHeader, PIMAGE_OPTIONAL_HEADER32 pOptHeader, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CReloacationDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RELOCATION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()


public:
	virtual BOOL OnInitDialog();

	PIMAGE_DOS_HEADER pDosHeader;
	PIMAGE_OPTIONAL_HEADER32 pOptHeader;
	CListCtrl m_list1;
	CListCtrl m_list2;

	afx_msg DWORD RvaToFoa(IN PIMAGE_DOS_HEADER pDosHeader_Base, IN DWORD RVA);	
	afx_msg void OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg int GetRelocationCount(PIMAGE_BASE_RELOCATION pRel);
	afx_msg VOID showRelocationBlock(int pRelData_Base, int Count, int pRelocationBlockBase_RVA);
};
