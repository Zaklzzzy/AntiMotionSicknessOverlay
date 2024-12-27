#include <windows.h>
#include <math.h>
#include <stdlib.h>

#define POINT_COUNT 20 // Количество точек по бокам экрана
#define SPEED 2        // Скорость движения точек

// Координаты точек
POINT points[POINT_COUNT];
POINT oldPoints[POINT_COUNT]; // Старые позиции точек для очистки
RECT pointBounds[POINT_COUNT]; // Границы движения каждой точки
POINT startPositions[POINT_COUNT]; // Стартовые позиции точек

// Смещение камеры
float cameraOffsetX = 0.0f;
float cameraOffsetY = 0.0f;

// Регистрация Raw Input для отслеживания движения мыши
void RegisterRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01; // Генерация ввода от мыши
    rid.usUsage = 0x02;     // Устройство мыши
    rid.dwFlags = RIDEV_INPUTSINK; // Направление ввода в окно
    rid.hwndTarget = hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        MessageBox(hwnd, L"Failed to register raw input", L"Error", MB_OK | MB_ICONERROR);
    }
}

// Функция для инициализации точек
void InitializePoints(int width, int height) {
    int margin = 50; // Отступ от краёв
    int spacing = height / (POINT_COUNT / 2); // Расстояние между точками
    int boundarySize = 100; // Размер границ

    // Левая сторона
    for (int i = 0; i < POINT_COUNT / 2; i++) {
        points[i].x = margin;
        points[i].y = i * spacing + margin;
        startPositions[i] = points[i]; // Сохраняем стартовые позиции
        oldPoints[i] = points[i];

        // Задаём границы
        pointBounds[i].left = points[i].x - boundarySize / 2;
        pointBounds[i].right = points[i].x + boundarySize / 2;
        pointBounds[i].top = points[i].y - boundarySize / 2;
        pointBounds[i].bottom = points[i].y + boundarySize / 2;
    }

    // Правая сторона
    for (int i = POINT_COUNT / 2; i < POINT_COUNT; i++) {
        points[i].x = width - margin;
        points[i].y = (i - POINT_COUNT / 2) * spacing + margin;
        startPositions[i] = points[i]; // Сохраняем стартовые позиции
        oldPoints[i] = points[i];

        // Задаём границы
        pointBounds[i].left = points[i].x - boundarySize / 2;
        pointBounds[i].right = points[i].x + boundarySize / 2;
        pointBounds[i].top = points[i].y - boundarySize / 2;
        pointBounds[i].bottom = points[i].y + boundarySize / 2;
    }
}

// Обработчик окна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INPUT: { // Обработка данных Raw Input
        UINT dwSize;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        LPBYTE lpb = new BYTE[dwSize];
        if (lpb == NULL) return 0;

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
            RAWINPUT* raw = (RAWINPUT*)lpb;
            if (raw->header.dwType == RIM_TYPEMOUSE && raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
                // Относительное движение мыши
                cameraOffsetX = (float)raw->data.mouse.lLastX * 0.5f;
                cameraOffsetY = (float)raw->data.mouse.lLastY * 0.5f;
            }
        }
        delete[] lpb;
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Рисуем точки
        HBRUSH pointBrush = CreateSolidBrush(RGB(255, 255, 255)); // Белые точки
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, pointBrush);

        for (int i = 0; i < POINT_COUNT; i++) {
            // Стираем старую точку
            RECT eraseRect = { oldPoints[i].x - 10, oldPoints[i].y - 10, oldPoints[i].x + 10, oldPoints[i].y + 10 };
            FillRect(hdc, &eraseRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

            // Рисуем новую точку
            Ellipse(hdc, points[i].x - 10, points[i].y - 10, points[i].x + 10, points[i].y + 10);

            // Обновляем старую позицию
            oldPoints[i] = points[i];
        }

        SelectObject(hdc, oldBrush);
        DeleteObject(pointBrush);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_TIMER: {
        for (int i = 0; i < POINT_COUNT; i++) {
            // Применяем смещение мыши
            points[i].x -= (int)cameraOffsetX;
            points[i].y -= (int)cameraOffsetY;

            // Возврат точки к стартовой позиции
            float returnSpeed = 0.1f; // Коэффициент скорости возврата
            points[i].x += (int)((startPositions[i].x - points[i].x) * returnSpeed);
            points[i].y += (int)((startPositions[i].y - points[i].y) * returnSpeed);

            // Проверяем границы экрана
            if (points[i].x < 0) points[i].x = 0;
            if (points[i].x > GetSystemMetrics(SM_CXSCREEN)) points[i].x = GetSystemMetrics(SM_CXSCREEN);
            if (points[i].y < 0) points[i].y = 0;
            if (points[i].y > GetSystemMetrics(SM_CYSCREEN)) points[i].y = GetSystemMetrics(SM_CYSCREEN);
        }

        // Плавное затухание смещения
        cameraOffsetX *= 0.8f;
        cameraOffsetY *= 0.8f;

        InvalidateRect(hwnd, NULL, FALSE); // Перерисовать окно
        return 0;
    }
    case WM_SIZE: {
        // Обновляем точки при изменении размера окна
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        InitializePoints(width, height);
        InvalidateRect(hwnd, NULL, TRUE); // Перерисовать окно
        return 0;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Главная функция
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OverlayWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Failed to register window class", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        wc.lpszClassName,
        L"AntiMotionSicknessOverlay",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, L"Failed to create overlay window", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Регистрация Raw Input
    RegisterRawInput(hwnd);

    // Устанавливаем прозрачный фон
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_COLORKEY);

    // Инициализация точек
    RECT rect;
    GetClientRect(hwnd, &rect);
    InitializePoints(rect.right, rect.bottom);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Запускаем таймер для обновления
    SetTimer(hwnd, 1, 16, NULL); // 60 обновлений в секунду

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
