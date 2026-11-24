
// ThreePointCircleMFCDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "ThreePointCircleMFC.h"
#include "ThreePointCircleMFCDlg.h"
#include "afxdialogex.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <system_error>

namespace
{
	constexpr UINT WM_APP_RANDOM_MOVE_UPDATE = WM_APP + 2;
	constexpr UINT WM_APP_RANDOM_MOVE_FINISHED = WM_APP + 3;
	constexpr int kRandomPointGenerationAttempts = 100;
	constexpr std::chrono::milliseconds kRandomMoveInterval(500);

	struct RandomMovePayload
	{
		std::uint64_t runId = 0;
		int iteration = 0;
		std::array<CPoint, CCanvasWnd::kMaxPointCount> points{};
	};

	bool GenerateValidRandomPoints(int width, int height,
		std::mt19937& generator,
		std::array<CPoint, CCanvasWnd::kMaxPointCount>& points)
	{
		if (width <= 0 || height <= 0)
		{
			return false;
		}

		std::uniform_int_distribution<int> xDistribution(0, width - 1);
		std::uniform_int_distribution<int> yDistribution(0, height - 1);
		for (int attempt = 0; attempt < kRandomPointGenerationAttempts; ++attempt)
		{
			for (CPoint& point : points)
			{
				point.x = xDistribution(generator);
				point.y = yDistribution(generator);
			}

			const CircleGeometry::CircumcircleResult result =
				CircleGeometry::CalculateCircumcircle(
					{ static_cast<double>(points[0].x), static_cast<double>(points[0].y) },
					{ static_cast<double>(points[1].x), static_cast<double>(points[1].y) },
					{ static_cast<double>(points[2].x), static_cast<double>(points[2].y) });
			if (result.IsValid())
			{
				return true;
			}
		}
		return false;
	}

	bool PostRandomMoveUpdate(HWND targetWindow,
		std::unique_ptr<RandomMovePayload> payload) noexcept
	{
		RandomMovePayload* rawPayload = payload.release();
		if (::PostMessage(targetWindow, WM_APP_RANDOM_MOVE_UPDATE, 0,
			reinterpret_cast<LPARAM>(rawPayload)))
		{
			return true;
		}

		delete rawPayload;
		return false;
	}

	void PostRandomMoveFinished(HWND targetWindow, std::uint64_t runId,
		bool succeeded, int completedIterations) noexcept
	{
		const LPARAM result = MAKELPARAM(
			static_cast<WORD>(completedIterations),
			static_cast<WORD>(succeeded ? 1 : 0));
		::PostMessage(targetWindow, WM_APP_RANDOM_MOVE_FINISHED,
			static_cast<WPARAM>(runId), result);
	}
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CThreePointCircleMFCDlg 대화 상자



CThreePointCircleMFCDlg::CThreePointCircleMFCDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_THREEPOINTCIRCLEMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CThreePointCircleMFCDlg::~CThreePointCircleMFCDlg()
{
	m_randomMoveCancelRequested.store(true);
	m_randomMoveCondition.notify_all();
	if (m_randomMoveThread.joinable())
	{
		m_randomMoveThread.join();
	}
}

void CThreePointCircleMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CThreePointCircleMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_APPLY_SETTINGS, &CThreePointCircleMFCDlg::OnBnClickedApplySettings)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CThreePointCircleMFCDlg::OnBnClickedReset)
	ON_BN_CLICKED(IDC_BUTTON_RANDOM_MOVE, &CThreePointCircleMFCDlg::OnBnClickedRandomMove)
	ON_MESSAGE(WM_APP_CANVAS_EVENT, &CThreePointCircleMFCDlg::OnCanvasEvent)
	ON_MESSAGE(WM_APP_RANDOM_MOVE_UPDATE, &CThreePointCircleMFCDlg::OnRandomMoveUpdate)
	ON_MESSAGE(WM_APP_RANDOM_MOVE_FINISHED, &CThreePointCircleMFCDlg::OnRandomMoveFinished)
END_MESSAGE_MAP()


// CThreePointCircleMFCDlg 메시지 처리기

BOOL CThreePointCircleMFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

#ifdef _DEBUG
	ASSERT(CircleGeometry::RunDebugVerification());
#endif

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	if (!m_canvas.AttachCanvas(this, IDC_CANVAS_PLACEHOLDER))
	{
		AfxMessageBox(_T("8-bit CImage 캔버스를 만들지 못했습니다."), MB_OK | MB_ICONERROR);
		EndDialog(IDCANCEL);
		return FALSE;
	}

	SendDlgItemMessage(IDC_EDIT_POINT_RADIUS, EM_SETLIMITTEXT, 10, 0);
	SendDlgItemMessage(IDC_EDIT_OUTLINE_THICKNESS, EM_SETLIMITTEXT, 10, 0);

	UpdateSettingEdits();
	UpdateCoordinateLabels();
	SetStatusText(_T("클릭점 0/3\r\n캔버스를 클릭하세요."));
	UpdateControlStates();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CThreePointCircleMFCDlg::OnOK()
{
	// Enter applies the settings but never closes the dialog unexpectedly.
	if (!m_randomMoveRunning)
	{
		OnBnClickedApplySettings();
	}
}

void CThreePointCircleMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CThreePointCircleMFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CThreePointCircleMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CThreePointCircleMFCDlg::OnBnClickedApplySettings()
{
	if (m_randomMoveRunning || m_isClosing)
	{
		return;
	}

	CString radiusText;
	CString thicknessText;
	GetDlgItemText(IDC_EDIT_POINT_RADIUS, radiusText);
	GetDlgItemText(IDC_EDIT_OUTLINE_THICKNESS, thicknessText);

	int radius = 0;
	int thickness = 0;
	const bool radiusIsValid = ParsePositiveInteger(
		radiusText, kMinimumPointRadius, kMaximumPointRadius, radius);
	const bool thicknessIsValid = ParsePositiveInteger(
		thicknessText, kMinimumOutlineThickness, kMaximumOutlineThickness, thickness);

	if (!radiusIsValid || !thicknessIsValid)
	{
		CString message(_T("설정을 적용하지 않았습니다. 기존 유효한 설정을 유지합니다.\r\n\r\n"));
		if (!radiusIsValid)
		{
			message += _T("- 클릭점 반지름: 1~100 사이의 정수를 입력하세요.\r\n");
		}
		if (!thicknessIsValid)
		{
			message += _T("- 외접원 선 두께: 1~50 사이의 정수를 입력하세요.");
		}
		AfxMessageBox(message, MB_OK | MB_ICONWARNING);
		SetStatusText(_T("설정 오류\r\n기존 설정을 유지합니다."));
		::SetFocus(::GetDlgItem(GetSafeHwnd(), !radiusIsValid ?
			IDC_EDIT_POINT_RADIUS : IDC_EDIT_OUTLINE_THICKNESS));
		return;
	}

	m_pointRadius = radius;
	m_outlineThickness = thickness;
	m_canvas.SetRenderSettings(m_pointRadius, m_outlineThickness);
	UpdateSettingEdits();

	if (m_canvas.GetPointCount() == CCanvasWnd::kMaxPointCount)
	{
		UpdateCanvasStatus();
		UpdateControlStates();
		return;
	}

	CString status;
	status.Format(_T("클릭점 %d/3\r\n설정 적용: 반지름 %d, 두께 %d"),
		m_canvas.GetPointCount(), m_pointRadius, m_outlineThickness);
	SetStatusText(status);
	UpdateControlStates();
}

void CThreePointCircleMFCDlg::OnBnClickedReset()
{
	CancelAndJoinRandomMove(true);
	m_canvas.ResetPoints();
	UpdateCoordinateLabels();
	UpdateSettingEdits();
	SetStatusText(_T("클릭점 0/3\r\n캔버스를 클릭하세요."));
	UpdateControlStates();
}

void CThreePointCircleMFCDlg::OnBnClickedRandomMove()
{
	if (m_randomMoveRunning || m_isClosing)
	{
		return;
	}

	CircleGeometry::Circumcircle circle;
	if (m_canvas.GetPointCount() != CCanvasWnd::kMaxPointCount ||
		!m_canvas.GetCircumcircle(circle))
	{
		UpdateControlStates();
		return;
	}

	CRect canvasRectangle;
	m_canvas.GetClientRect(&canvasRectangle);
	const int canvasWidth = static_cast<int>(canvasRectangle.Width());
	const int canvasHeight = static_cast<int>(canvasRectangle.Height());
	if (canvasWidth <= 0 || canvasHeight <= 0)
	{
		SetStatusText(_T("랜덤 이동을 시작할 수 없습니다.\r\n캔버스 크기가 올바르지 않습니다."));
		UpdateControlStates();
		return;
	}

	if (m_randomMoveThread.joinable())
	{
		m_randomMoveThread.join();
	}

	++m_activeRandomMoveRunId;
	m_lastRandomMoveIteration = 0;
	m_randomMoveCancelRequested.store(false);
	m_randomMoveRunning = true;
#ifdef _DEBUG
	m_randomMoveTimingSampleCount = 0;
#endif
	UpdateControlStates();
	SetStatusText(_T("랜덤 이동 준비 중... 0 / 10"));

	const std::uint64_t runId = m_activeRandomMoveRunId;
	const HWND targetWindow = GetSafeHwnd();
	try
	{
		m_randomMoveThread = std::thread(
			&CThreePointCircleMFCDlg::RandomMoveWorker, this,
			targetWindow, runId, canvasWidth, canvasHeight);
	}
	catch (const std::system_error&)
	{
		m_randomMoveRunning = false;
		m_randomMoveCancelRequested.store(true);
		UpdateControlStates();
		SetStatusText(_T("랜덤 이동 worker를 시작하지 못했습니다."));
	}
}

void CThreePointCircleMFCDlg::OnDestroy()
{
	m_isClosing = true;
	CancelAndJoinRandomMove(true);
	m_canvas.SetInteractionEnabled(false);
	CDialogEx::OnDestroy();
}

LRESULT CThreePointCircleMFCDlg::OnCanvasEvent(WPARAM wParam, LPARAM /*lParam*/)
{
	const CanvasEvent event = static_cast<CanvasEvent>(wParam);
	if (event == CanvasEvent::PointsChanged)
	{
		UpdateCoordinateLabels();
		UpdateCanvasStatus();
		UpdateControlStates();
	}
	return 0;
}

LRESULT CThreePointCircleMFCDlg::OnRandomMoveUpdate(
	WPARAM /*wParam*/, LPARAM lParam)
{
	std::unique_ptr<RandomMovePayload> payload(
		reinterpret_cast<RandomMovePayload*>(lParam));
	if (!payload || !m_randomMoveRunning ||
		payload->runId != m_activeRandomMoveRunId ||
		payload->iteration <= m_lastRandomMoveIteration)
	{
		return 0;
	}

	if (!m_canvas.ApplyPoints(payload->points))
	{
		CancelAndJoinRandomMove(true);
		UpdateControlStates();
		SetStatusText(_T("랜덤 좌표의 외접원을 계산하지 못했습니다."));
		return 0;
	}

	m_lastRandomMoveIteration = payload->iteration;
#ifdef _DEBUG
	if (payload->iteration >= 1 &&
		payload->iteration <= kRandomMoveIterationCount)
	{
		m_randomMoveAppliedTimes[
			static_cast<std::size_t>(payload->iteration - 1)] =
			std::chrono::steady_clock::now();
		m_randomMoveTimingSampleCount = payload->iteration;
	}
#endif
	UpdateCoordinateLabels();
	UpdateRandomMoveStatus(payload->iteration);
	return 0;
}

LRESULT CThreePointCircleMFCDlg::OnRandomMoveFinished(
	WPARAM wParam, LPARAM lParam)
{
	const std::uint64_t runId = static_cast<std::uint64_t>(wParam);
	const int completedIterations = static_cast<int>(LOWORD(lParam));
	const bool succeeded = HIWORD(lParam) != 0;
	if (!m_randomMoveRunning || runId != m_activeRandomMoveRunId)
	{
		return 0;
	}

	m_randomMoveCancelRequested.store(true);
	m_randomMoveCondition.notify_all();
	if (m_randomMoveThread.joinable())
	{
		m_randomMoveThread.join();
	}
	m_randomMoveRunning = false;
	UpdateControlStates();

	if (succeeded && completedIterations == kRandomMoveIterationCount &&
		m_lastRandomMoveIteration == kRandomMoveIterationCount)
	{
		CString status(_T("랜덤 이동 완료 (10 / 10)"));
#ifdef _DEBUG
		if (m_randomMoveTimingSampleCount == kRandomMoveIterationCount)
		{
			long long minimumInterval = (std::numeric_limits<long long>::max)();
			long long maximumInterval = 0;
			for (int index = 1; index < kRandomMoveIterationCount; ++index)
			{
				const long long interval = std::chrono::duration_cast<
					std::chrono::milliseconds>(
						m_randomMoveAppliedTimes[static_cast<std::size_t>(index)] -
						m_randomMoveAppliedTimes[static_cast<std::size_t>(index - 1)]).count();
				minimumInterval = (std::min)(minimumInterval, interval);
				maximumInterval = (std::max)(maximumInterval, interval);
			}
			CString timing;
			timing.Format(_T("\r\n주기 %lld~%lld ms"),
				minimumInterval, maximumInterval);
			status += timing;
		}
#endif
		SetStatusText(status);
	}
	else
	{
		CString status;
		status.Format(_T("랜덤 이동 중단 (%d / %d)\r\n유효한 좌표 생성에 실패했습니다."),
			completedIterations, kRandomMoveIterationCount);
		SetStatusText(status);
	}
	return 0;
}

bool CThreePointCircleMFCDlg::ParsePositiveInteger(const CString& text,
	int minimum, int maximum, int& value) noexcept
{
	if (text.IsEmpty())
	{
		return false;
	}

	std::uint64_t parsed = 0;
	for (int index = 0; index < text.GetLength(); ++index)
	{
		const wchar_t character = text[index];
		if (character < L'0' || character > L'9')
		{
			return false;
		}

		parsed = parsed * 10 + static_cast<unsigned int>(character - L'0');
		if (parsed > static_cast<std::uint64_t>(maximum))
		{
			return false;
		}
	}

	if (parsed < static_cast<std::uint64_t>(minimum))
	{
		return false;
	}

	value = static_cast<int>(parsed);
	return true;
}

void CThreePointCircleMFCDlg::UpdateCoordinateLabels()
{
	const UINT labelIds[CCanvasWnd::kMaxPointCount] =
	{
		IDC_STATIC_P1,
		IDC_STATIC_P2,
		IDC_STATIC_P3
	};

	for (int index = 0; index < CCanvasWnd::kMaxPointCount; ++index)
	{
		CString text;
		CPoint point;
		if (m_canvas.GetPoint(index, point))
		{
			text.Format(_T("P%d: X = %d, Y = %d"), index + 1, point.x, point.y);
		}
		else
		{
			text.Format(_T("P%d: 미입력"), index + 1);
		}
		SetDlgItemText(labelIds[index], text);
	}
}

void CThreePointCircleMFCDlg::UpdateSettingEdits()
{
	SetDlgItemInt(IDC_EDIT_POINT_RADIUS, static_cast<UINT>(m_pointRadius), FALSE);
	SetDlgItemInt(IDC_EDIT_OUTLINE_THICKNESS,
		static_cast<UINT>(m_outlineThickness), FALSE);
}

void CThreePointCircleMFCDlg::UpdateCanvasStatus()
{
	const int pointCount = m_canvas.GetPointCount();
	if (pointCount < CCanvasWnd::kMaxPointCount)
	{
		CString status;
		status.Format(_T("클릭점 %d/3\r\n다음 점을 입력하세요."), pointCount);
		SetStatusText(status);
		return;
	}

	CircleGeometry::Circumcircle circle;
	if (m_canvas.GetCircumcircle(circle))
	{
		CString status;
		status.Format(_T("외접원 생성 완료\r\n중심 (%.2f, %.2f), 반지름 %.2f"),
			circle.centerX, circle.centerY, circle.radius);
		SetStatusText(status);
		return;
	}

	switch (m_canvas.GetCircumcircleError())
	{
	case CircleGeometry::CircumcircleError::DuplicatePoints:
		SetStatusText(_T("중복된 클릭점이 있어\r\n외접원을 계산할 수 없습니다."));
		break;
	case CircleGeometry::CircumcircleError::CollinearPoints:
		SetStatusText(_T("세 점이 일직선이어서\r\n외접원을 계산할 수 없습니다."));
		break;
	default:
		SetStatusText(_T("수치적으로 불안정하여\r\n외접원을 계산할 수 없습니다."));
		break;
	}
}

void CThreePointCircleMFCDlg::UpdateControlStates()
{
	const bool idle = !m_randomMoveRunning && !m_isClosing;
	const HWND dialogWindow = GetSafeHwnd();
	if (!::IsWindow(dialogWindow))
	{
		return;
	}

	::EnableWindow(::GetDlgItem(dialogWindow, IDC_EDIT_POINT_RADIUS), idle);
	::EnableWindow(::GetDlgItem(dialogWindow, IDC_EDIT_OUTLINE_THICKNESS), idle);
	::EnableWindow(::GetDlgItem(dialogWindow, IDC_BUTTON_APPLY_SETTINGS), idle);
	::EnableWindow(::GetDlgItem(dialogWindow, IDC_BUTTON_RESET), !m_isClosing);

	CircleGeometry::Circumcircle circle;
	const bool canStartRandomMove = idle &&
		m_canvas.GetPointCount() == CCanvasWnd::kMaxPointCount &&
		m_canvas.GetCircumcircle(circle);
	::EnableWindow(::GetDlgItem(dialogWindow, IDC_BUTTON_RANDOM_MOVE),
		canStartRandomMove);
	m_canvas.SetInteractionEnabled(idle);
}

void CThreePointCircleMFCDlg::UpdateRandomMoveStatus(int iteration)
{
	CircleGeometry::Circumcircle circle;
	CString status;
	if (m_canvas.GetCircumcircle(circle))
	{
		status.Format(_T("랜덤 이동 중... %d / %d\r\n중심 (%.1f, %.1f)"),
			iteration, kRandomMoveIterationCount,
			circle.centerX, circle.centerY);
	}
	else
	{
		status.Format(_T("랜덤 이동 중... %d / %d\r\n외접원 계산 불가"),
			iteration, kRandomMoveIterationCount);
	}
	SetStatusText(status);
}

void CThreePointCircleMFCDlg::RandomMoveWorker(HWND targetWindow,
	std::uint64_t runId, int canvasWidth, int canvasHeight) noexcept
{
	int completedIterations = 0;
	try
	{
		std::random_device randomDevice;
		std::seed_seq seed
		{
			randomDevice(), randomDevice(), randomDevice(), randomDevice(),
			static_cast<unsigned int>(runId),
			static_cast<unsigned int>(runId >> 32)
		};
		std::mt19937 generator(seed);
		auto nextUpdate = std::chrono::steady_clock::now();

		for (int iteration = 1;
			iteration <= kRandomMoveIterationCount; ++iteration)
		{
			nextUpdate += kRandomMoveInterval;
			std::unique_lock<std::mutex> lock(m_randomMoveWaitMutex);
			const bool cancelled = m_randomMoveCondition.wait_until(
				lock, nextUpdate,
				[this]() { return m_randomMoveCancelRequested.load(); });
			lock.unlock();
			if (cancelled || m_randomMoveCancelRequested.load())
			{
				return;
			}

			std::array<CPoint, CCanvasWnd::kMaxPointCount> points{};
			if (!GenerateValidRandomPoints(
				canvasWidth, canvasHeight, generator, points))
			{
				PostRandomMoveFinished(targetWindow, runId, false,
					completedIterations);
				return;
			}
			if (m_randomMoveCancelRequested.load())
			{
				return;
			}

			auto payload = std::make_unique<RandomMovePayload>();
			payload->runId = runId;
			payload->iteration = iteration;
			payload->points = points;
			if (!PostRandomMoveUpdate(targetWindow, std::move(payload)))
			{
				PostRandomMoveFinished(targetWindow, runId, false,
					completedIterations);
				return;
			}
			completedIterations = iteration;
		}

		if (!m_randomMoveCancelRequested.load())
		{
			PostRandomMoveFinished(targetWindow, runId, true,
				completedIterations);
		}
	}
	catch (...)
	{
		if (!m_randomMoveCancelRequested.load())
		{
			PostRandomMoveFinished(targetWindow, runId, false,
				completedIterations);
		}
	}
}

void CThreePointCircleMFCDlg::CancelAndJoinRandomMove(bool invalidateRunId)
{
	if (invalidateRunId)
	{
		++m_activeRandomMoveRunId;
	}

	m_randomMoveCancelRequested.store(true);
	m_randomMoveCondition.notify_all();
	if (m_randomMoveThread.joinable())
	{
		m_randomMoveThread.join();
	}
	m_randomMoveRunning = false;
	m_lastRandomMoveIteration = 0;
	DrainRandomMoveMessages();
}

void CThreePointCircleMFCDlg::DrainRandomMoveMessages() noexcept
{
	const HWND dialogWindow = GetSafeHwnd();
	if (!::IsWindow(dialogWindow))
	{
		return;
	}

	MSG message{};
	while (::PeekMessage(&message, dialogWindow,
		WM_APP_RANDOM_MOVE_UPDATE, WM_APP_RANDOM_MOVE_UPDATE, PM_REMOVE))
	{
		delete reinterpret_cast<RandomMovePayload*>(message.lParam);
	}
	while (::PeekMessage(&message, dialogWindow,
		WM_APP_RANDOM_MOVE_FINISHED, WM_APP_RANDOM_MOVE_FINISHED, PM_REMOVE))
	{
	}
}

void CThreePointCircleMFCDlg::SetStatusText(const CString& text)
{
	SetDlgItemText(IDC_STATIC_STATUS, text);
}

