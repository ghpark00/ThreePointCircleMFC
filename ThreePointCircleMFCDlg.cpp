
// ThreePointCircleMFCDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "ThreePointCircleMFC.h"
#include "ThreePointCircleMFCDlg.h"
#include "afxdialogex.h"

#include <cstdint>

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

void CThreePointCircleMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CThreePointCircleMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_APPLY_SETTINGS, &CThreePointCircleMFCDlg::OnBnClickedApplySettings)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CThreePointCircleMFCDlg::OnBnClickedReset)
	ON_MESSAGE(WM_APP_CANVAS_EVENT, &CThreePointCircleMFCDlg::OnCanvasEvent)
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

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CThreePointCircleMFCDlg::OnOK()
{
	// Enter applies the settings but never closes the dialog unexpectedly.
	OnBnClickedApplySettings();
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
		return;
	}

	CString status;
	status.Format(_T("클릭점 %d/3\r\n설정 적용: 반지름 %d, 두께 %d"),
		m_canvas.GetPointCount(), m_pointRadius, m_outlineThickness);
	SetStatusText(status);
}

void CThreePointCircleMFCDlg::OnBnClickedReset()
{
	m_canvas.ResetPoints();
	UpdateCoordinateLabels();
	UpdateSettingEdits();
	SetStatusText(_T("클릭점 0/3\r\n캔버스를 클릭하세요."));
}

LRESULT CThreePointCircleMFCDlg::OnCanvasEvent(WPARAM wParam, LPARAM /*lParam*/)
{
	const CanvasEvent event = static_cast<CanvasEvent>(wParam);
	if (event == CanvasEvent::PointsChanged)
	{
		UpdateCoordinateLabels();
		UpdateCanvasStatus();
	}
	else if (event == CanvasEvent::PointLimitReached)
	{
		SetStatusText(_T("클릭점 3/3\r\n더 이상 추가할 수 없습니다."));
	}
	else if (event == CanvasEvent::ExistingPointPressed)
	{
		SetStatusText(_T("클릭점 3/3\r\n드래그는 다음 단계에서 지원합니다."));
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

void CThreePointCircleMFCDlg::SetStatusText(const CString& text)
{
	SetDlgItemText(IDC_STATIC_STATUS, text);
}

