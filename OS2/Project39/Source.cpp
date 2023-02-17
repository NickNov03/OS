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
	int NetWidth; // ширина поля
	int NetHeight; // высота поля
	int PaintField; // длина стороны квадрата (находящегося внутри поля), внутри которого происходи рисование
	int OffsetX; // отступ от края поля до квадрата по Х
	int OffsetY; //отступ от края поля до квадрата по Y
	NetInf() // конструктор
	{
		NetWidth = 0;
		NetHeight = 0;
		PaintField = 0;
		OffsetX = 0;
		OffsetY = 0;
	};
};



UINT WM_RESTART, // освобожает поля
	WM_CROSS, // вызывается из обработчика WM_FILLMATRIX для инициализации элемента матрицы еденицей (крестиком)
	WM_CIRCLE, // вызывается из обработчика WM_FILLMATRIX для инициализации элемента матрицы минус еденицей (кругом)
	WM_FILLMATRIX; // вызывается для инциализации всей матрицы



int width = 320; // ширина окна
int height = 240; // высота окна
int N = 3; // кол-во полей


DWORD lines = RGB(255, 0, 0); // Цвет линий
DWORD bg = RGB(0, 0, 255); // Цвет квадратов
int color = 0;
vector<vector<short>> matrix(N, vector<short>(N, 0)); // матрица	
const wchar_t class_name[] = L"Sample Window Class"; // имя окна
const TCHAR szTitle[] = L"Title"; //

HPEN COLOR_NET;
POINT mouse;

HBRUSH COLOR_BG = CreateSolidBrush(bg);

void NetInfFill(NetInf* PNI, RECT rect) // заполнение структуры NetInf при помощи rect
{
	PNI->NetWidth = rect.right / N;
	PNI->NetHeight = rect.bottom / N;
	PNI->PaintField = min(PNI->NetHeight, PNI->NetWidth) * 0.8;
	PNI->OffsetX = (PNI->NetWidth - PNI->PaintField) / 2;
	PNI->OffsetY = (PNI->NetHeight - PNI->PaintField) / 2;
}

// сохранение полей структуры в файл
void saving() // вызывается при закрытии окна
{
	ofstream out("config.txt");

	if (out.is_open())
	{
		out.clear();
		out << N << endl << width << endl << height << endl << bg << endl << lines;
		out.close();
	}
}


void RunNotepad() // запуск блокнота
{
	STARTUPINFO sInfo; // хранит	различные	параметры	запуска
	PROCESS_INFORMATION pInfo; // после	создания	процесса	будет	хранить	его	дескриптор	и	т.п.
	ZeroMemory(&sInfo, sizeof(STARTUPINFO)); // Fills a block of memory with zeros
	CreateProcess(_T("C:\\Windows\\Notepad.exe"), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &sInfo, &pInfo); // создание процеса (и МТ)
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

void NetPaint(PAINTSTRUCT ps, RECT rect, HDC hdc) // рисование сетки
{
	for (int i = 1; i < N; i++)
	{
		MoveToEx // устанавливаем новую позицию
		(
			hdc, //	где мы глобально (в нашем контексте)
			i * rect.right / N, // где мы в нашем контексте по x (правая координата i-го столбца)
			0, // по y (вверху)
			NULL // куда сохраняем предыдущую позицию (никуда)
		);
		LineTo // проводим линию текущей ручкой (PEN)
		(
			hdc, // понятно
			i * rect.right / N, // куда проводим по x (туда же)
			rect.bottom // по y (вниз)
		); // таким образом, линия проводится вертикально вниз
		MoveToEx(hdc, 0, i * rect.bottom / N, NULL); // аналогично проводим
		LineTo(hdc, rect.right, i * rect.bottom / N); // горизонтальные линии
	}
}




void Circles(HDC hdc, RECT rect, NetInf NI) // рисует круги
{
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			if (matrix[i][j] == 1) // если тут должен быть круг
			{
				Ellipse // то рисуем его)
				(
					hdc,
					NI.NetWidth * i + NI.OffsetX, // тут всё логично
					NI.NetHeight * j + NI.OffsetY,
					NI.NetWidth * (i + 1) - NI.OffsetX,
					NI.NetHeight * (j + 1) - NI.OffsetY
				);
			}
		}
	}
}


void Crosses(HDC hdc, RECT rect, NetInf NI) // рисует крестики
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


void RandBrush(HWND hwnd) // изменение цвета фона на случайный
{
	bg = RGB(rand() % 256, rand() % 256, rand() % 256); // генерация нового цвета фона
	COLOR_BG = CreateSolidBrush(bg); // меняем дескриптор созданной кисти 
	SetClassLongPtr // Заменяет маркер фоновой кисти, связанной с классом
	(
		hwnd, // дескриптор окна
		GCL_HBRBACKGROUND, // Заменяет дескриптор фоновой кисти, связанной с классом
		(LONG)COLOR_BG // новая кисть в LONG
	);
	InvalidateRect // добавляет прямоугольник к обновляемому региону заданного окна
	(
		hwnd, // Дескриптор основного окна
		NULL, // указывает, что вся область перерисуется
		TRUE // Фон также будет перерисован
	);
}


void RestartGame() // matrix заполняется нулями
{
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			matrix[i][j] = 0;
		}
	}
}


// hwnd — это дескриптор окна
// Msg — это код сообщения
// wParam и lParam содержат дополнительные данные, относящиеся к сообщению. Точное значение зависит от кода сообщения
// LRESULT — это целочисленное значение, которое программа возвращает к Windows
// Значение этого значения зависит от кода сообщения
// CALLBACK — это соглашение о вызовах функции
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps; //Создаётся ссылка на структуру, содержащую информацию для окрашивания рабочей области
	RECT rect; //Созадётся ссылка на структуру, содержащую информацию о прямоугольнике

	if (Msg == WM_CROSS) // ставим крест в поле
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
	if (Msg == WM_CIRCLE) // ставим круг в поле
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
	if (Msg == WM_FILLMATRIX) // инициализируем матрицу актуальной инфой
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

	switch (Msg) // обрабатываем сообщения
	{
	case WM_LBUTTONDOWN:
	{
		GetClientRect(hwnd, &rect); // получаем инф-ию о клиентской области
		int x = GET_X_LPARAM(lParam) * N / rect.right; // номер квадрата, на который мы нажали, по x
		int y = GET_Y_LPARAM(lParam) * N / rect.bottom; // номер квадрата, на который мы нажали, по у
		SendMessage(HWND_BROADCAST, WM_CIRCLE, wParam, MAKELPARAM(x, y)); // отпралвяем сообщение о нажатии лкм
		return 0;
	}

	case WM_RBUTTONDOWN:
	{
		GetClientRect(hwnd, &rect); // получаем инф-ию о клиентской области
		int x = GET_X_LPARAM(lParam) * N / rect.right; // номер квадрата, на который мы нажали, по x
		int y = GET_Y_LPARAM(lParam) * N / rect.bottom; // номер квадрата, на который мы нажали, по у
		SendMessage(HWND_BROADCAST, WM_CROSS, wParam, MAKELPARAM(x, y)); // отпралвяем сообщение о нажатии пкм
		return 0;
	}

	case WM_DESTROY: // сообщение о закрытии окна
		saving();
		// функция просто указывает системе, что потоку требуется прекратить свою работу
		// помещает	в	очередь	сообщений	новое	типа	WM_QUIT, предназначенное для текущего потока
		// Когда поток извлекает сообщение WM_QUIT из своей очереди сообщений,
		// он должен выйти из своего цикла обработки сообщений и возвратить управление системе
		// параметр 0 указывает на то, что приложение закончило работу без ошибки
		PostQuitMessage(0);
		return 0; // сообщение успешно обработано

	case WM_KEYDOWN: // сообщение о нажатой клавише на клаве
	{
		switch (wParam) // обрабатываем доп данные о нажатии
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
		SendMessage // отпраляем сообщение о заполнении матриц
		(
			HWND_BROADCAST, // Дескриптор окна, оконная процедура которого получит сообщение (всем top-level окнам)
			WM_FILLMATRIX, // Сообщение (UINT)
			wParam, // Дополнительная информация
			lParam // Дополнительная информация
		);
		return 0;

	case WM_SIZE: // изменение размера окна
		InvalidateRect(hwnd, NULL, true); //Вся рабочая область перекрашивается (вызываем WM_PAINT)
		return 0;

	case WM_PAINT:
	{
		HDC hdc = BeginPaint // Заполняет ps и устанавливает контекст рисования (хэндл DC)
		(
			hwnd, // Handle to the window to be repainted
			&ps // Pointer to the PAINTSTRUCT structure that will receive painting information
		);
		GetClientRect(hwnd, &rect); // rect заполняется координатами клиентской области
		HPEN hPen = CreatePen // return value is a handle that identifies a logical pen
		(
			PS_SOLID, // сплошная
			3, lines // ширина 3 пиксель, цвет lines
		);
		SelectObject(hdc, hPen); // Созданная ручка устанавливается в созданный контекст окрашивания главного окна

		NetPaint(ps, rect, hdc); // создаем сетку

		NetInf NI; // структура с инф-ей о полях
		NetInf* PNI = &NI; // указатель на нее
		NetInfFill(PNI, rect); // заполняем ее актуальными данными

		Circles(hdc, rect, NI); // рисуем круги

		Crosses(hdc, rect, NI); // рисуем крестики

		return 0;
	}

	default:
		return DefWindowProc(hwnd, Msg, wParam, lParam); // по умолчанию дефолтная обработка
	}
}


// ф-ия, вызываемая виндой для работы с окном
// возвращает целое значение, оно не используется операционной системой, но возвращаемое значение можно использовать
//  для передачи кода состояния в какую-то другую написанную программу
// параметры:
// 1. дескриптор экземпляра, операционная система использует это значение для идентификации исполняемого файла (EXE) при загрузке в память.
// 2. использовался в 16-разрядной Windows, но теперь всегда равен нулю
// 3. содержит аргументы командной строки в виде строки Юникода
// 4. флаг, указывающий, будет ли главное окно приложения свернуто, развернуто или показано обычным образом
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

	MSG msg{}; // структура, которая содержит в себе информацию о соообщениях (между Windows и окном или между окнами)
	HWND hwnd{}; // дескриптор окна (HANDLE указ. на объект ядра в котором хранится информация о нашем окне)

	// Класс окна определяет набор поведений, которые могут быть общими для нескольких окон
	// Данные, уникальные для каждого окна, называются данными экземпляра окна
	// Каждое окно должно быть связано с классом окна, даже если программа создает только один экземпляр этого класса
	// Важно понимать, что класс окна не является "классом" в смысле C++. Скорее, это структура данных, используемая внутренне операционной системой
	// Классы окон регистрируются в системе во время выполнения
	// Структура WNDCLASSEX содержит информацию о классе окна
	// похожа на структуру WNDCLASS, отличается наличиями полей
	// cbSize, который устанавливает размер структуры, hIconSm, который содержит дескриптор маленького значка, связанного с классом окна
	WNDCLASSEX wc
	{

	};
	// Устанавливает размер этой структуры, в байтах
	wc.cbSize = sizeof(wc);
	// Устанавливает число дополнительных байт, которые размещаются вслед за структурой класса окна. Система инициализирует эти байты нулями.
	wc.cbClsExtra = 0;
	// Устанавливает число дополнительных байтов, которые размещаются вслед за экземпляром окна
	wc.cbWndExtra = 0;
	// Дескриптор кисти фона класса.Этот член структуры может быть дескриптором физической кисти,
	// которая используется, чтобы красить цветом фона, или это может быть кодом цвета
	wc.hbrBackground = COLOR_BG;
	// Дескриптор курсора класса. Этот член структуры должен быть дескриптором ресурса курсора
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	// Дескриптор значка класса
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	// Дескриптор маленького значка, который связан с классом окна
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	// Дескриптор экземпляра, который содержит оконную процедуру для класса
	wc.hInstance = hInstance;
	// Указатель на оконную процедуру
	// Все аспекты внешнего вида и поведения окна зависят от ответа процедуры окна на эти сообщения
	wc.lpfnWndProc = WindowProcedure;
	// 
	wc.lpszClassName = class_name;
	//
	wc.lpszMenuName = nullptr;
	// Устанавливает стиль(и) класса
	wc.style = CS_VREDRAW | CS_HREDRAW;

	// Регистрирует класс окна для последующего использования в вызовах функции CreateWindow или CreateWindowEx; иначе-выход с кодои ошибки
	if (!RegisterClassEx(&wc))
	{
		return EXIT_FAILURE;
	}

	// создание окна
	// Возвращает дескриптор созданного	окна
	hwnd = CreateWindow(wc.lpszClassName, class_name, WS_OVERLAPPEDWINDOW, 0, 0, 320, 240, nullptr, nullptr, wc.hInstance, nullptr);
	if (hwnd == INVALID_HANDLE_VALUE) // если что-то пошло не так
	{
		return EXIT_FAILURE;
	}

	// показываем окно
	ShowWindow(hwnd, nCmdShow);
	// обновляем (перерисовываем)
	UpdateWindow(hwnd);

	// Первым параметром GetMessage является адрес структуры MSG 
	// Если функция завершается успешно, она заполняет структуру MSG сведениями о сообщении
	// Другие три параметра позволяют фильтровать сообщения, полученные из очереди
	while (GetMessage(&msg, nullptr, 0, 0)) // цикл обработки сообщений
	{
		TranslateMessage(&msg); // Преобразует сообщения виртуального ключа в символьные сообщения
		DispatchMessage(&msg); // Отправляет сообщение (полученное GetMessage'м) оконной процедуре (lpfnWndProc'у)
	}

	DeleteObject(COLOR_BG);
	DeleteObject(COLOR_NET);


	return 0;

}
