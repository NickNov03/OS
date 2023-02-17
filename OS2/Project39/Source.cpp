#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <tchar.h>
#include <fstream>
#include <windowsx.h>

using namespace std;

struct NetInf
{
	int NetWidth; // ������ ����
	int NetHeight; // ������ ����
	int PaintField; // ����� ������� �������� (������������ ������ ����), ������ �������� ��������� ���������
	int OffsetX; // ������ �� ���� ���� �� �������� �� �
	int OffsetY; //������ �� ���� ���� �� �������� �� Y
	NetInf() // �����������
	{
		NetWidth = 0;
		NetHeight = 0;
		PaintField = 0;
		OffsetX = 0;
		OffsetY = 0;
	};
};



UINT WM_RESTART, // ���������� ����
	WM_CROSS, // ���������� �� ����������� WM_FILLMATRIX ��� ������������� �������� ������� �������� (���������)
	WM_CIRCLE, // ���������� �� ����������� WM_FILLMATRIX ��� ������������� �������� ������� ����� �������� (������)
	WM_FILLMATRIX; // ���������� ��� ������������ ���� �������



int width = 320; // ������ ����
int height = 240; // ������ ����
int N = 3; // ���-�� �����


DWORD lines = RGB(255, 0, 0); // ���� �����
DWORD bg = RGB(0, 0, 255); // ���� ���������
int color = 0;
vector<vector<short>> matrix(N, vector<short>(N, 0)); // �������	
const wchar_t class_name[] = L"Sample Window Class"; // ��� ����
const TCHAR szTitle[] = L"Title"; //

HPEN COLOR_NET;
POINT mouse;

HBRUSH COLOR_BG = CreateSolidBrush(bg);

void NetInfFill(NetInf* PNI, RECT rect) // ���������� ��������� NetInf ��� ������ rect
{
	PNI->NetWidth = rect.right / N;
	PNI->NetHeight = rect.bottom / N;
	PNI->PaintField = min(PNI->NetHeight, PNI->NetWidth) * 0.8;
	PNI->OffsetX = (PNI->NetWidth - PNI->PaintField) / 2;
	PNI->OffsetY = (PNI->NetHeight - PNI->PaintField) / 2;
}

// ���������� ����� ��������� � ����
void saving() // ���������� ��� �������� ����
{
	ofstream out("config.txt");

	if (out.is_open())
	{
		out.clear();
		out << N << endl << width << endl << height << endl << bg << endl << lines;
		out.close();
	}
}


void RunNotepad() // ������ ��������
{
	STARTUPINFO sInfo; // ������	���������	���������	�������
	PROCESS_INFORMATION pInfo; // �����	��������	��������	�����	�������	���	����������	�	�.�.
	ZeroMemory(&sInfo, sizeof(STARTUPINFO)); // Fills a block of memory with zeros
	CreateProcess(_T("C:\\Windows\\Notepad.exe"), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &sInfo, &pInfo); // �������� ������� (� ��)
}



void read()
{
	ifstream in("config.txt");
	if (in.is_open()) {
		string str;
		if (!getline(in, str)) return;
		N = stoi(str);
		if (!getline(in, str)) return;
		width = stoi(str);
		if (!getline(in, str)) return;
		height = stoi(str);
		if (!getline(in, str)) return;
		bg = stoi(str);
		if (!getline(in, str)) return;
		lines = stoi(str);
		in.close();
	}
}

void NetPaint(PAINTSTRUCT ps, RECT rect, HDC hdc) // ��������� �����
{
	for (int i = 1; i < N; i++)
	{
		MoveToEx // ������������� ����� �������
		(
			hdc, //	��� �� ��������� (� ����� ���������)
			i * rect.right / N, // ��� �� � ����� ��������� �� x (������ ���������� i-�� �������)
			0, // �� y (������)
			NULL // ���� ��������� ���������� ������� (������)
		);
		LineTo // �������� ����� ������� ������ (PEN)
		(
			hdc, // �������
			i * rect.right / N, // ���� �������� �� x (���� ��)
			rect.bottom // �� y (����)
		); // ����� �������, ����� ���������� ����������� ����
		MoveToEx(hdc, 0, i * rect.bottom / N, NULL); // ���������� ��������
		LineTo(hdc, rect.right, i * rect.bottom / N); // �������������� �����
	}
}




void Circles(HDC hdc, RECT rect, NetInf NI) // ������ �����
{
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			if (matrix[i][j] == 1) // ���� ��� ������ ���� ����
			{
				Ellipse // �� ������ ���)
				(
					hdc,
					NI.NetWidth * i + NI.OffsetX, // ��� �� �������
					NI.NetHeight * j + NI.OffsetY,
					NI.NetWidth * (i + 1) - NI.OffsetX,
					NI.NetHeight * (j + 1) - NI.OffsetY
				);
			}
		}
	}
}


void Crosses(HDC hdc, RECT rect, NetInf NI) // ������ ��������
{
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			if (matrix[i][j] == -1)
			{
				MoveToEx(hdc, i * NI.NetWidth + NI.OffsetX, j * NI.NetHeight + NI.OffsetY, NULL);
				LineTo(hdc, i * NI.NetWidth + NI.PaintField + NI.OffsetX, j * NI.NetHeight + NI.PaintField + NI.OffsetY);
				MoveToEx(hdc, i * NI.NetWidth + NI.OffsetX, j * NI.NetHeight + NI.PaintField + NI.OffsetY, NULL);
				LineTo(hdc, i * NI.NetWidth + NI.PaintField + NI.OffsetX, j * NI.NetHeight + NI.OffsetY);
			}
		}
	}
}


void RandBrush(HWND hwnd) // ��������� ����� ���� �� ���������
{
	bg = RGB(rand() % 256, rand() % 256, rand() % 256); // ��������� ������ ����� ����
	COLOR_BG = CreateSolidBrush(bg); // ������ ���������� ��������� ����� 
	SetClassLongPtr // �������� ������ ������� �����, ��������� � �������
	(
		hwnd, // ���������� ����
		GCL_HBRBACKGROUND, // �������� ���������� ������� �����, ��������� � �������
		(LONG)COLOR_BG // ����� ����� � LONG
	);
	InvalidateRect // ��������� ������������� � ������������ ������� ��������� ����
	(
		hwnd, // ���������� ��������� ����
		NULL, // ���������, ��� ��� ������� ������������
		TRUE // ��� ����� ����� �����������
	);
}


void RestartGame() // matrix ����������� ������
{
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			matrix[i][j] = 0;
		}
	}
}


// hwnd � ��� ���������� ����
// Msg � ��� ��� ���������
// wParam � lParam �������� �������������� ������, ����������� � ���������. ������ �������� ������� �� ���� ���������
// LRESULT � ��� ������������� ��������, ������� ��������� ���������� � Windows
// �������� ����� �������� ������� �� ���� ���������
// CALLBACK � ��� ���������� � ������� �������
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps; //�������� ������ �� ���������, ���������� ���������� ��� ����������� ������� �������
	RECT rect; //�������� ������ �� ���������, ���������� ���������� � ��������������

	if (Msg == WM_CROSS) // ������ ����� � ����
	{
		int i = LOWORD(lParam);
		int j = HIWORD(lParam);
		if (matrix[i][j] == 0)
		{
			matrix[i][j] = -1;
			InvalidateRect(hwnd, NULL, TRUE);
		}
		return 0;
	}
	if (Msg == WM_CIRCLE) // ������ ���� � ����
	{
		int i = LOWORD(lParam);
		int j = HIWORD(lParam);
		if (matrix[i][j] == 0)
		{
			matrix[i][j] = 1;
			InvalidateRect(hwnd, NULL, TRUE);
		}
		return 0;
	}
	if (Msg == WM_FILLMATRIX) // �������������� ������� ���������� �����
	{
		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
			{
				if (matrix[i][j] == -1)
				{
					SendMessage(HWND_BROADCAST, WM_CIRCLE, 0, MAKELPARAM(i, j));
				}
				if (matrix[i][j] == 1)
				{
					SendMessage(HWND_BROADCAST, WM_CROSS, 0, MAKELPARAM(i, j));
				}
			}
		}
		return 0;
	}
	if (Msg == WM_RESTART)
	{
		RestartGame();
		InvalidateRect(hwnd, NULL, true);
		return 0;
	}

	switch (Msg) // ������������ ���������
	{
	case WM_LBUTTONDOWN:
	{
		GetClientRect(hwnd, &rect); // �������� ���-�� � ���������� �������
		int x = GET_X_LPARAM(lParam) * N / rect.right; // ����� ��������, �� ������� �� ������, �� x
		int y = GET_Y_LPARAM(lParam) * N / rect.bottom; // ����� ��������, �� ������� �� ������, �� �
		SendMessage(HWND_BROADCAST, WM_CIRCLE, wParam, MAKELPARAM(x, y)); // ���������� ��������� � ������� ���
		return 0;
	}

	case WM_RBUTTONDOWN:
	{
		GetClientRect(hwnd, &rect); // �������� ���-�� � ���������� �������
		int x = GET_X_LPARAM(lParam) * N / rect.right; // ����� ��������, �� ������� �� ������, �� x
		int y = GET_Y_LPARAM(lParam) * N / rect.bottom; // ����� ��������, �� ������� �� ������, �� �
		SendMessage(HWND_BROADCAST, WM_CROSS, wParam, MAKELPARAM(x, y)); // ���������� ��������� � ������� ���
		return 0;
	}

	case WM_DESTROY: // ��������� � �������� ����
		saving();
		// ������� ������ ��������� �������, ��� ������ ��������� ���������� ���� ������
		// ��������	�	�������	���������	�����	����	WM_QUIT, ��������������� ��� �������� ������
		// ����� ����� ��������� ��������� WM_QUIT �� ����� ������� ���������,
		// �� ������ ����� �� ������ ����� ��������� ��������� � ���������� ���������� �������
		// �������� 0 ��������� �� ��, ��� ���������� ��������� ������ ��� ������
		PostQuitMessage(0);
		return 0; // ��������� ������� ����������

	case WM_KEYDOWN: // ��������� � ������� ������� �� �����
	{
		switch (wParam) // ������������ ��� ������ � �������
		{
		case VK_ESCAPE: // Esc
			PostQuitMessage(0);
			break;
		case 'Q': // Q
			if (GetKeyState(VK_CONTROL)) PostQuitMessage(0); // + Ctrl
			break;
		case 'C': // C
			if (GetKeyState(VK_SHIFT)) RunNotepad(); // + Shift
			break;
		case VK_RETURN: // Enter
			RandBrush(hwnd);
			break;
		case 'R': // R
			SendMessage(HWND_BROADCAST, WM_RESTART, wParam, lParam);
			return 0;
		default:
			break;
		}

		return 0;
	}
		
	case WM_CREATE:
		SendMessage // ��������� ��������� � ���������� ������
		(
			HWND_BROADCAST, // ���������� ����, ������� ��������� �������� ������� ��������� (���� top-level �����)
			WM_FILLMATRIX, // ��������� (UINT)
			wParam, // �������������� ����������
			lParam // �������������� ����������
		);
		return 0;

	case WM_SIZE: // ��������� ������� ����
		InvalidateRect(hwnd, NULL, true); //��� ������� ������� ��������������� (�������� WM_PAINT)
		return 0;

	case WM_PAINT:
	{
		HDC hdc = BeginPaint // ��������� ps � ������������� �������� ��������� (����� DC)
		(
			hwnd, // Handle to the window to be repainted
			&ps // Pointer to the PAINTSTRUCT structure that will receive painting information
		);
		GetClientRect(hwnd, &rect); // rect ����������� ������������ ���������� �������
		HPEN hPen = CreatePen // return value is a handle that identifies a logical pen
		(
			PS_SOLID, // ��������
			3, lines // ������ 3 �������, ���� lines
		);
		SelectObject(hdc, hPen); // ��������� ����� ��������������� � ��������� �������� ����������� �������� ����

		NetPaint(ps, rect, hdc); // ������� �����

		NetInf NI; // ��������� � ���-�� � �����
		NetInf* PNI = &NI; // ��������� �� ���
		NetInfFill(PNI, rect); // ��������� �� ����������� �������

		Circles(hdc, rect, NI); // ������ �����

		Crosses(hdc, rect, NI); // ������ ��������

		return 0;
	}

	default:
		return DefWindowProc(hwnd, Msg, wParam, lParam); // �� ��������� ��������� ���������
	}
}


// �-��, ���������� ������ ��� ������ � �����
// ���������� ����� ��������, ��� �� ������������ ������������ ��������, �� ������������ �������� ����� ������������
//  ��� �������� ���� ��������� � �����-�� ������ ���������� ���������
// ���������:
// 1. ���������� ����������, ������������ ������� ���������� ��� �������� ��� ������������� ������������ ����� (EXE) ��� �������� � ������.
// 2. ������������� � 16-��������� Windows, �� ������ ������ ����� ����
// 3. �������� ��������� ��������� ������ � ���� ������ �������
// 4. ����, �����������, ����� �� ������� ���� ���������� ��������, ���������� ��� �������� ������� �������
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR szCmdLine, int nCmdShow)
{

	WM_RESTART = RegisterWindowMessage(TEXT("YOURAPP_FIND_MSG1"));
	WM_CROSS = RegisterWindowMessage(TEXT("YOURAPP_FIND_MSG3"));
	WM_CIRCLE = RegisterWindowMessage(TEXT("YOURAPP_FIND_MSG4"));
	WM_FILLMATRIX = RegisterWindowMessage(TEXT("YOURAPP_FIND_MSG5"));

	LPWSTR* argv;
	int argc;
	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	read();
	if (argc > 1)
	{
		N = stoi(argv[1]);
		matrix.assign(N, vector<short>(N, 0));
	}
	else
	{
		matrix.assign(N, vector<short>(N, 0));
	}

	MSG msg{}; // ���������, ������� �������� � ���� ���������� � ����������� (����� Windows � ����� ��� ����� ������)
	HWND hwnd{}; // ���������� ���� (HANDLE ����. �� ������ ���� � ������� �������� ���������� � ����� ����)

	// ����� ���� ���������� ����� ���������, ������� ����� ���� ������ ��� ���������� ����
	// ������, ���������� ��� ������� ����, ���������� ������� ���������� ����
	// ������ ���� ������ ���� ������� � ������� ����, ���� ���� ��������� ������� ������ ���� ��������� ����� ������
	// ����� ��������, ��� ����� ���� �� �������� "�������" � ������ C++. ������, ��� ��������� ������, ������������ ��������� ������������ ��������
	// ������ ���� �������������� � ������� �� ����� ����������
	// ��������� WNDCLASSEX �������� ���������� � ������ ����
	// ������ �� ��������� WNDCLASS, ���������� ��������� �����
	// cbSize, ������� ������������� ������ ���������, hIconSm, ������� �������� ���������� ���������� ������, ���������� � ������� ����
	WNDCLASSEX wc
	{

	};
	// ������������� ������ ���� ���������, � ������
	wc.cbSize = sizeof(wc);
	// ������������� ����� �������������� ����, ������� ����������� ����� �� ���������� ������ ����. ������� �������������� ��� ����� ������.
	wc.cbClsExtra = 0;
	// ������������� ����� �������������� ������, ������� ����������� ����� �� ����������� ����
	wc.cbWndExtra = 0;
	// ���������� ����� ���� ������.���� ���� ��������� ����� ���� ������������ ���������� �����,
	// ������� ������������, ����� ������� ������ ����, ��� ��� ����� ���� ����� �����
	wc.hbrBackground = COLOR_BG;
	// ���������� ������� ������. ���� ���� ��������� ������ ���� ������������ ������� �������
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	// ���������� ������ ������
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	// ���������� ���������� ������, ������� ������ � ������� ����
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	// ���������� ����������, ������� �������� ������� ��������� ��� ������
	wc.hInstance = hInstance;
	// ��������� �� ������� ���������
	// ��� ������� �������� ���� � ��������� ���� ������� �� ������ ��������� ���� �� ��� ���������
	wc.lpfnWndProc = WindowProcedure;
	// 
	wc.lpszClassName = class_name;
	//
	wc.lpszMenuName = nullptr;
	// ������������� �����(�) ������
	wc.style = CS_VREDRAW | CS_HREDRAW;

	// ������������ ����� ���� ��� ������������ ������������� � ������� ������� CreateWindow ��� CreateWindowEx; �����-����� � ����� ������
	if (!RegisterClassEx(&wc))
	{
		return EXIT_FAILURE;
	}

	// �������� ����
	// ���������� ���������� ����������	����
	hwnd = CreateWindow(wc.lpszClassName, class_name, WS_OVERLAPPEDWINDOW, 0, 0, 320, 240, nullptr, nullptr, wc.hInstance, nullptr);
	if (hwnd == INVALID_HANDLE_VALUE) // ���� ���-�� ����� �� ���
	{
		return EXIT_FAILURE;
	}

	// ���������� ����
	ShowWindow(hwnd, nCmdShow);
	// ��������� (��������������)
	UpdateWindow(hwnd);

	// ������ ���������� GetMessage �������� ����� ��������� MSG 
	// ���� ������� ����������� �������, ��� ��������� ��������� MSG ���������� � ���������
	// ������ ��� ��������� ��������� ����������� ���������, ���������� �� �������
	while (GetMessage(&msg, nullptr, 0, 0)) // ���� ��������� ���������
	{
		TranslateMessage(&msg); // ����������� ��������� ������������ ����� � ���������� ���������
		DispatchMessage(&msg); // ���������� ��������� (���������� GetMessage'�) ������� ��������� (lpfnWndProc'�)
	}

	DeleteObject(COLOR_BG);
	DeleteObject(COLOR_NET);


	return 0;

}
