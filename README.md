# ThreePointCircleMFC

## 프로젝트 소개

ThreePointCircleMFC는 Visual Studio 2022와 C++17로 작성한 MFC Dialog 기반 프로그램입니다. 사용자가 캔버스에 입력한 세 점의 외접원을 계산하고, 점 Drag와 Random Move에 맞추어 결과를 실시간으로 다시 그립니다.

원 렌더링에는 `Ellipse`, GDI+ 같은 도형 API를 사용하지 않습니다. 8-bit `CImage`의 bitmap memory를 `GetBits()`와 `GetPitch()`로 얻어 픽셀을 직접 수정합니다.

## 주요 기능

- 캔버스 클릭으로 P1, P2, P3 입력
- 클릭점 반지름 설정(1~100 px)
- P1/P2/P3의 캔버스 기준 중심 좌표 표시
- 세 점을 지나는 외접원 계산 및 테두리 렌더링
- 외접원 선 두께 설정(1~50 px)
- 클릭점 Drag 중 좌표와 외접원 실시간 갱신
- 점, 외접원, Drag 및 Random Move 상태 초기화
- 별도 `std::thread`에서 수행하는 Random Move
- 약 500 ms 간격으로 총 10회 이동
- 실행 중 최소화, 복원, repaint 및 초기화 지원

## 실행 화면 흐름

1. 캔버스에서 P1을 클릭합니다.
2. P2를 클릭합니다.
3. P3를 클릭합니다.
4. 유효한 세 점이면 세 점을 지나는 외접원이 생성됩니다.
5. 클릭점 하나를 눌러 Drag합니다.
6. Mouse Move마다 좌표와 외접원이 갱신됩니다.
7. 외접원이 생성된 상태에서 **랜덤 이동**을 누르면 세 점이 10회 이동합니다.

## 프로젝트 구조

| 구성 요소 | 책임 |
| --- | --- |
| `CThreePointCircleMFCDlg` | 설정 입력 검증, 좌표·상태 표시, 버튼 활성화 정책, Random Move worker의 시작·취소·종료 및 UI message 처리 |
| `CCanvasWnd` | `CImage` 수명 관리, 클릭점 보관, 입력·Hit-test·Mouse Capture·Drag 처리, 외접원 재계산 및 화면 갱신 |
| `PixelRenderer` | 8-bit bitmap memory를 직접 수정하여 작은 채움 원과 큰 원의 테두리 렌더링 |
| `CircleGeometry` | 중복점·일직선·수치 오류를 구분하고 세 점의 외접원 중심과 반지름 계산 |

주요 파일은 다음과 같습니다.

```text
ThreePointCircleMFCDlg.h/.cpp  Dialog와 Random Move 제어
CanvasWnd.h/.cpp               캔버스 입력, 상태, 렌더링 연결
PixelRenderer.h/.cpp           직접 픽셀 렌더링
CircleGeometry.h/.cpp          외접원 계산
ThreePointCircleMFC.rc         Dialog와 컨트롤 리소스
```

## 외접원 계산

`CircleGeometry::CalculateCircumcircle()`은 첫 번째 점을 원점으로 평행 이동한 뒤 두 수직이등분선의 연립방정식을 풉니다. 계산한 중심에서 첫 번째 점까지의 거리를 반지름으로 사용하고, 세 점에서 중심까지의 거리가 허용 오차 안에서 같은지도 확인합니다.

- 두 점 이상이 같은 좌표이면 `DuplicatePoints`를 반환합니다.
- 행렬식이 0에 가까우면 세 점이 일직선인 것으로 보고 `CollinearPoints`를 반환합니다.
- 중심이나 반지름이 유한하지 않거나 거리 검증에 실패하면 `NumericalFailure`를 반환합니다.

이 경우 클릭점은 유지하지만 외접원은 표시하지 않습니다. 점을 다시 유효한 위치로 이동하면 즉시 외접원이 복원됩니다.

## 직접 픽셀 렌더링

`CCanvasWnd`는 8-bit grayscale `CImage`를 캔버스 크기로 유지합니다. `PixelRenderer`는 다음 정보로 각 scanline에 직접 접근합니다.

- `CImage::GetBits()`로 bitmap 시작 주소 획득
- `CImage::GetPitch()`로 부호가 포함된 scanline 간격 획득
- 캔버스 범위로 반복 영역을 clipping한 뒤 픽셀 값 기록

작은 클릭점은 픽셀 중심과 점 중심 사이의 거리 제곱이 `radius²` 이하인지 검사하여 채웁니다. 외접원은 거리 제곱이 안쪽 반지름 제곱과 바깥쪽 반지름 제곱 사이인 픽셀만 기록하므로 내부를 채우지 않습니다. 외접원이 캔버스 밖으로 나가도 원을 축소하거나 이동하지 않고 bitmap과 겹치는 영역만 그립니다.

`CImage::BitBlt()`는 직접 생성한 완성 bitmap을 `OnPaint()`에서 화면으로 전달하는 용도로만 사용합니다.

## Drag 구조

1. `HitTestPoint()`가 클릭 위치에서 가장 가까운 기존 점을 찾습니다.
2. 점을 찾으면 `SetCapture()`로 Mouse Capture를 시작합니다.
3. `OnMouseMove()`가 좌표를 캔버스 범위로 제한하고 점을 이동합니다.
4. 이동할 때마다 외접원을 다시 계산하고 bitmap과 좌표 UI를 즉시 갱신합니다.
5. `OnLButtonUp()`, `WM_CAPTURECHANGED`, `WM_CANCELMODE`, Esc, 초기화 및 창 파괴 시 Drag index와 Capture를 정리합니다.

## Random Move 스레드 구조

- Dialog는 실행마다 새 `std::thread`를 시작합니다.
- worker는 실행당 하나의 `std::mt19937`을 사용해 세 점의 후보 좌표를 만듭니다.
- 생성한 세 점이 유효한 외접원을 만드는지 `CircleGeometry`로 확인합니다.
- `std::condition_variable::wait_until()`과 `std::chrono::steady_clock`으로 약 500 ms 간격을 유지하며 총 10회 수행합니다.
- worker는 MFC UI나 `CImage`에 접근하지 않고 `PostMessage()`로 좌표 payload만 전달합니다.
- UI thread가 `WM_APP` message를 받아 점, 외접원, 좌표와 상태를 갱신합니다.
- Run ID(generation)가 이전 실행의 늦은 message를 차단합니다.
- 초기화와 종료는 cancel flag 설정, `condition_variable` 통지, `join()` 및 대기 message 회수 순서로 worker를 정리합니다.

## 빌드 방법

### 준비 사항

- Visual Studio 2022
- **Desktop development with C++** 워크로드
- 사용 중인 toolset(v143)에 맞는 **C++ MFC for latest v143 build tools** 구성요소
- Windows 10/11 SDK

### 빌드 및 실행

1. `ThreePointCircleMFC.sln`을 Visual Studio 2022에서 엽니다.
2. Solution Configuration을 `Debug` 또는 `Release`로 선택합니다.
3. Solution Platform을 `x64`로 선택합니다.
4. **Build > Build Solution**을 실행합니다.
5. `F5`로 디버깅 실행하거나 `Ctrl+F5`로 실행합니다.

## 사용 방법

1. 클릭점 반지름과 외접원 두께를 입력하고 **설정 적용**을 누릅니다.
2. 캔버스를 세 번 클릭해 P1, P2, P3를 만듭니다.
3. 점을 Drag하여 외접원이 실시간으로 바뀌는지 확인합니다.
4. **랜덤 이동**을 눌러 약 500 ms 간격의 10회 이동을 실행합니다.
5. 모든 상태를 지우려면 **초기화**를 누릅니다.

## 제한사항

- 중복된 점이나 일직선상의 세 점으로는 유일한 외접원을 만들 수 없습니다.
- 큰 외접원의 중심과 일부 또는 대부분이 캔버스 밖에 있을 수 있으며, 보이는 영역만 렌더링됩니다.
- Random Move의 500 ms 간격은 Windows thread scheduling과 UI message 처리 시점에 따라 약간 달라질 수 있습니다.
- Random Move 중에는 설정 변경과 수동 Drag를 잠그며, 초기화·최소화·복원·창 닫기는 계속 사용할 수 있습니다.
