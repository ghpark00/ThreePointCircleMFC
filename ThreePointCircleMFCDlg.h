
// ThreePointCircleMFCDlg.h: 헤더 파일
//

#pragma once

#include "CanvasWnd.h"

// CThreePointCircleMFCDlg 대화 상자
class CThreePointCircleMFCDlg : public CDialogEx
{
// 생성입니다.
public:
	CThreePointCircleMFCDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_THREEPOINTCIRCLEMFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
	virtual void OnOK();


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedApplySettings();
	afx_msg void OnBnClickedReset();
	afx_msg LRESULT OnCanvasEvent(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	static constexpr int kMinimumPointRadius = 1;
	static constexpr int kMaximumPointRadius = 100;
	static constexpr int kMinimumOutlineThickness = 1;
	static constexpr int kMaximumOutlineThickness = 50;

	static bool ParsePositiveInteger(const CString& text, int minimum,
		int maximum, int& value) noexcept;
	void UpdateCoordinateLabels();
	void UpdateSettingEdits();
	void UpdateCanvasStatus();
	void SetStatusText(const CString& text);

	CCanvasWnd m_canvas;
	int m_pointRadius = 10;
	int m_outlineThickness = 2;
};
