
// ThreePointCircleMFCDlg.h: 헤더 파일
//

#pragma once

#include "CanvasWnd.h"

#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

// CThreePointCircleMFCDlg 대화 상자
class CThreePointCircleMFCDlg : public CDialogEx
{
// 생성입니다.
public:
	CThreePointCircleMFCDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	~CThreePointCircleMFCDlg() override;

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
	afx_msg void OnBnClickedRandomMove();
	afx_msg void OnDestroy();
	afx_msg LRESULT OnCanvasEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRandomMoveUpdate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRandomMoveFinished(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	static constexpr int kMinimumPointRadius = 1;
	static constexpr int kMaximumPointRadius = 100;
	static constexpr int kMinimumOutlineThickness = 1;
	static constexpr int kMaximumOutlineThickness = 50;
	static constexpr int kRandomMoveIterationCount = 10;

	static bool ParsePositiveInteger(const CString& text, int minimum,
		int maximum, int& value) noexcept;
	void UpdateCoordinateLabels();
	void UpdateSettingEdits();
	void UpdateCanvasStatus();
	void UpdateControlStates();
	void UpdateRandomMoveStatus(int iteration);
	void RandomMoveWorker(HWND targetWindow, std::uint64_t runId,
		int canvasWidth, int canvasHeight) noexcept;
	void CancelAndJoinRandomMove(bool invalidateRunId);
	void DrainRandomMoveMessages() noexcept;
	void SetStatusText(const CString& text);

	CCanvasWnd m_canvas;
	int m_pointRadius = 10;
	int m_outlineThickness = 2;
	std::thread m_randomMoveThread;
	std::mutex m_randomMoveWaitMutex;
	std::condition_variable m_randomMoveCondition;
	std::atomic_bool m_randomMoveCancelRequested{ false };
	std::uint64_t m_activeRandomMoveRunId = 0;
	int m_lastRandomMoveIteration = 0;
	bool m_randomMoveRunning = false;
	bool m_isClosing = false;
#ifdef _DEBUG
	std::array<std::chrono::steady_clock::time_point,
		kRandomMoveIterationCount> m_randomMoveAppliedTimes{};
	int m_randomMoveTimingSampleCount = 0;
#endif
};
