//      Простая компьютерная игра Thread War
//      Используйте клавиши "влево" и "вправо", чтобы перемещать пушку
//      клавиша "пробел" производит выстрел
//      Если 30 врагов уйдут с экрана неуничтоженными,  вы проиграли
//      Очки даются за каждого убитого противника
#include <windows.h>
#include <process.h>
#include <tchar.h>

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

#define BULLET '*' // Символ пули
#define BARREL '|' // Символ пушки
#define STACK_SIZE (64*1024) // Размер стека для нового потока

// Объекты синхронизации
HANDLE screenlock;              // изменением экрана занимается только один поток
HANDLE bulletsem;               // можно выстрелить только три раза подряд
HANDLE startevt;                // игра начинается с нажатием клавиши "влево" или "вправо"
HANDLE conin, conout;           // дескрипторы консоли
HANDLE mainthread;              // Основной поток main
CRITICAL_SECTION gameover;

// Ширина и высота консоли
int columns = 0;
int rows = 0;

// количество попаданий и промахов
LONG hit = 0;
LONG miss = 0;
LONG delayfactor = 7;           // фактор задержки для генерации противников

char badchar[] = "-\\|/";		// Анимация противника

// Создание случайного числа от n0 до n1
int random(int n0, int n1)
{
	if (n0 == 0 && n1 == 1)
		return rand() % 2;

	return rand() % (n1 - n0) + n0;
}

// Очистка экрана консоли 
void cls()
{
	COORD org = { 0, 0 };
	DWORD res;
	FillConsoleOutputCharacter(conout, ' ', columns*rows,
		org, &res);
}

// вывести на экран символ в позицию х и у 
void writeat(int x, int y, char c)
{
	// Блокировать вывод на экран при помощи мьютекса
	WaitForSingleObject(screenlock, INFINITE);
	COORD pos = { x, y };
	DWORD res;
	WriteConsoleOutputCharacterA(conout, &c, 1, pos, &res);
	ReleaseMutex(screenlock);
}

// Получить нажатие на клавишу (счетчик повторейний в ct) 
int getakey(int &ct)
{
	INPUT_RECORD input;
	DWORD res;
	while (true) {
		ReadConsoleInput(conin, &input, 1, &res);

		// игнорировать другие события
		if (input.EventType != KEY_EVENT)
			continue;

		// игнорировать события отпускания клавиш 
		// нас интересуют только нажатия
		if (!input.Event.KeyEvent.bKeyDown)
			continue;

		ct = input.Event.KeyEvent.wRepeatCount;
		return input.Event.KeyEvent.wVirtualKeyCode;
	}
}

// Обработка комбинаций ^С, ^Break, и т.п. 
BOOL WINAPI ctrl(DWORD type)
{
	ExitProcess(0);
	return TRUE;                // недостижимый участок кода
}

// Определить символ в заданной позиции экрана
int getat(int x, int y)
{
	char c;
	DWORD res;
	COORD org = { x, y };

	// Блокировать доступ к консоли до тех пор, пока процедура не будет выполнена
	WaitForSingleObject(screenlock, INFINITE);
	ReadConsoleOutputCharacterA(conout, &c, 1, org, &res);
	ReleaseMutex(screenlock);   // unlock
	return c;
}

// Отобразить очки в заголовке окна и проверить условие завершения игры 
void score(void)
{
	TCHAR s[128];
	wsprintf(s, _T("Thread War! Попаданий: %3d, Промахов: %3d"),
		hit, miss);
	SetConsoleTitle(s);

	if (miss >= 30) {
		EnterCriticalSection(&gameover);
		SuspendThread(mainthread);      // остановить главный поток
		MessageBox(NULL, _T("Game Over!"), _T("Thread War"),
			MB_OK | MB_SETFOREGROUND);
		ExitProcess(0);                 // не выходит из критической секции
	}

	if ((hit + miss) % 20 == 0)
		InterlockedDecrement(&delayfactor);     // должен быть ilock
}

// это поток противника 
DWORD WINAPI badguy(LPVOID _y)
{
	int y = (long)_y;          // случайная координата у
	int dir;
	int dly;
	int x;
	BOOL hitme = FALSE;

	// нечетные у появляются слева, четные у появляются справа
	x = y % 2 ? 0 : columns;

	// установить направление в зависимости от начальной позиции
	dir = x ? -1 : 1;

	// пока противник находится в пределах экрана
	while (((dir == 1 && x != columns) ||
		(dir == -1 && x != 0)) && !hitme) {

		// проверка на попадание (пуля?) 
		if (getat(x, y) == BULLET)
			hitme = TRUE;

		// вывод символа на экран
		writeat(x, y, badchar[x % 4]);

		// еще одна проверка на попадание
		if (getat(x, y) == BULLET)
			hitme = TRUE;

		// проверка на попадание через небольшие
		// промежутки времени
		if (delayfactor < 3)
			dly = 3;
		else
			dly = delayfactor + 3;

		for (int i = 0; i < dly; i++) {
			Sleep(40);
			if (getat(x, y) == BULLET) {
				hitme = TRUE;
				break;
			}
		}

		writeat(x, y, ' ');

		// еще одна проверка на попадание
		if (getat(x, y) == BULLET)
			hitme = TRUE;

		x += dir;
	}

	if (hitme)
	{
		// в противника попали!
		MessageBeep(-1);
		InterlockedIncrement(&hit);
	}
	else {
		// противник убежал!
		InterlockedIncrement(&miss);
	}

	score();

	return 0;
}

// этот поток занимается созданием потоков противников
DWORD WINAPI badguys(LPVOID)
{
	// ждем сигнала к началу игры в течение 15 секунд
	WaitForSingleObject(startevt, 15000);

	// создаем случайного врага
	// каждые 5 секунд появляется шанс создать
	// противника с координатами от 1 до 10
	while (true) {
		if (random(0, 100) < (hit + miss) / 25 + 20)
			// со временем вероятность увеличивается
			CreateThread(NULL, STACK_SIZE, badguy,
			(void *)(random(1, 10)), 0, NULL);
		Sleep(1000);            // каждую секунду
	}
}

// Это поток пули
// каждая пуля - это отдельный поток
DWORD WINAPI bullet(LPVOID _xy_)
{
	int x = LOWORD(_xy_);
	int y = HIWORD(_xy_);

	if (getat(x, y) == '*')
		return 0;               // здесь уже есть пуля

								// надо подождать 
								// Проверить семафор
								// если семафор равен 0, выстрела не происходит 
	if (WaitForSingleObject(bulletsem, 0) == WAIT_TIMEOUT)
		return 0;

	while (--y >= 0) {
		writeat(x, y, BULLET);       // отобразить пулю
		Sleep(100);
		writeat(x, y, ' ');       // стереть пулю
	}

	// выстрел сделан - добавить 1 к семафору
	ReleaseSemaphore(bulletsem, 1, NULL);

	return 0;
}

// Основная программа
int main()
{
	// Настройка глобальных переменных
	conin = GetStdHandle(STD_INPUT_HANDLE);
	conout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCtrlHandler(ctrl, TRUE);
	SetConsoleMode(conin, ENABLE_WINDOW_INPUT);

	// Уберем курсор
	CONSOLE_CURSOR_INFO cinfo;
	cinfo.bVisible = false;
	SetConsoleCursorInfo(conout, &cinfo);

	// получим реальный дескриптор текущего потока
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
		&mainthread, 0, FALSE, DUPLICATE_SAME_ACCESS);

	startevt = CreateEvent(NULL, TRUE, FALSE, NULL);
	screenlock = CreateMutex(NULL, FALSE, NULL);
	InitializeCriticalSection(&gameover);
	bulletsem = CreateSemaphore(NULL, 3, 3, NULL);

	CONSOLE_SCREEN_BUFFER_INFO info;        // информация о консоли
	GetConsoleScreenBufferInfo(conout, &info);

	// Инициализировать отображение информации об очках
	score();

	// Настроить генератор случайных чисел
	srand((unsigned)time(NULL));
	cls();                      // на самом деле не нужно 

	// установка начальной позиции пушки
	columns = info.srWindow.Right - info.srWindow.Left + 1;
	rows = info.srWindow.Bottom - info.srWindow.Top + 1;
	int y = rows - 1;
	int x = columns / 2;

	// запустить поток badguys; ничего не делать до тех пор, 
	// пока не произойдет событие или истекут 15 секунд
	CreateThread(NULL, STACK_SIZE, badguys, NULL, 0, NULL);

	// основной цикл игры
	while (true) {
		int c, ct;

		writeat(x, y, BARREL);     // нарисовать пушку 
		c = getakey(ct);        // получить символ

		switch (c) {
		case VK_SPACE:
			CreateThread(NULL, 64 * 1024, bullet, (void *)MAKELONG(x, y), 0, NULL);
			Sleep(100);     // дать пуле время улететь на некоторое расстояние
			break;

		case VK_LEFT:          // команда "влево!"
			SetEvent(startevt); // поток badguys работает 
			writeat(x, y, ' '); // убрать с экрана пушку
			x = (x > ct) ? (x - ct) : 0;
			break;

		case VK_RIGHT:         // команда "вправо!"; логика та же
			SetEvent(startevt);
			writeat(x, y, ' ');
			x = (x + ct < columns - 1) ? (x + ct) : (columns - 1);
			break;
		}
	}
}
