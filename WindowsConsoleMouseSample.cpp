// WindowsConsoleMouseSample.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <Windows.h>
#include <string>
#include <iostream>

class ConsoleGUIBase
{
	HANDLE hStdin, hStdout;
	DWORD fdwSaveOldMode;
	bool stop;

	struct State
	{
		int button_1_left = 0, button_1_right = 0, button_2_left = 0, button_2_right = 0;
		bool button_1_hover = false, button_2_hover = false;

		bool operator==(const State& other) const
		{
			return button_1_left == other.button_1_left
				&& button_1_right == other.button_1_right
				&& button_2_left == other.button_2_left
				&& button_2_right == other.button_2_right
				&& button_1_hover == other.button_1_hover
				&& button_2_hover == other.button_2_hover;
		}

		bool operator!=(const State& other) const
		{
			return !(*this == other);
		}
	};
	State state;

public:
	ConsoleGUIBase() : stop(false)
	{
		// 入力処理サンプル https://learn.microsoft.com/ja-jp/windows/console/reading-input-buffer-events
		hStdin = GetStdHandle(STD_INPUT_HANDLE);
		if (hStdin == INVALID_HANDLE_VALUE)
			error_exit("GetStdHandle");
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hStdout == INVALID_HANDLE_VALUE)
			error_exit("GetStdHandle");

		DWORD fdwMode;

		if (!GetConsoleMode(hStdin, &fdwSaveOldMode))
			error_exit("GetConsoleMode");

		// Windows10で必要（これがないと、マウスクリックでテキストコピー機能が動作し、イベントは発生しない）
		fdwMode = ENABLE_EXTENDED_FLAGS;
		if (!SetConsoleMode(hStdin, fdwMode))
			error_exit("SetConsoleMode");

		// マウス入力を有効にする
		fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
		if (!SetConsoleMode(hStdin, fdwMode))
			error_exit("SetConsoleMode");

		// カーソルを表示しない
		CONSOLE_CURSOR_INFO cursor_info = { 1, FALSE };
		SetConsoleCursorInfo(hStdout, &cursor_info); // 終了時に戻す必要あり？
	}

	~ConsoleGUIBase()
	{
		SetConsoleMode(hStdin, fdwSaveOldMode);
	}

	// CHAR_INFO配列に文字列を表示属性とともに書き込む。
	void put_str(CHAR_INFO* buffer, COORD buffer_size, COORD& cur_pos, const wchar_t* text, WORD attributes)
	{
		size_t len = wcslen(text);
		for (size_t i = 0; i < len; i++)
		{
			CHAR_INFO& c = buffer[cur_pos.X + cur_pos.Y * buffer_size.X];
			c.Char.UnicodeChar = text[i];
			c.Attributes = attributes;
			cur_pos.X++;
			// TODO: 全角文字の場合、2文字として処理する必要がある
		}
	}

	// 現在のstateをもとにレンダリングする。
	void render()
	{
		const SHORT buffer_size_x = 80, buffer_size_y = 20;
		const COORD buffer_size = { buffer_size_x,buffer_size_y };
		CHAR_INFO buffer[buffer_size_x * buffer_size_y];
		memset(buffer, 0, sizeof(buffer));
		COORD cur_pos = { 0,0 };
		wchar_t text_buffer[100];

		swprintf_s(text_buffer, sizeof(text_buffer) / sizeof(text_buffer[0]), L"[BUTTON 1]");
		put_str(buffer, buffer_size, cur_pos, text_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY | (state.button_1_hover ? BACKGROUND_INTENSITY : 0));
		swprintf_s(text_buffer, sizeof(text_buffer) / sizeof(text_buffer[0]), L" Left click: %d Right click: %d", state.button_1_left, state.button_1_right);
		put_str(buffer, buffer_size, cur_pos, text_buffer, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

		cur_pos.X = 0;
		cur_pos.Y = 1;

		swprintf_s(text_buffer, sizeof(text_buffer) / sizeof(text_buffer[0]), L"[BUTTON 2]");
		put_str(buffer, buffer_size, cur_pos, text_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY | (state.button_2_hover ? BACKGROUND_INTENSITY : 0));
		swprintf_s(text_buffer, sizeof(text_buffer) / sizeof(text_buffer[0]), L" Left click: %d Right click: %d", state.button_2_left, state.button_2_right);
		put_str(buffer, buffer_size, cur_pos, text_buffer, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

		COORD coord_zero = { 0,0 };
		SMALL_RECT rect_full = { 0, 0, buffer_size.X - 1, buffer_size.Y - 1 };
		if (WriteConsoleOutput(hStdout, buffer, buffer_size, coord_zero, &rect_full) == 0)
		{
			error_exit("WriteConsoleOutput");
		};
	}

	// マウスカーソルがボタンの上にあるかどうか判定する。
	bool hit_button(COORD mouse_coord, int button_id)
	{
		// mouse_event.dwMousePosition.X: 最も左の文字で0。全角は2文字分。
		switch (button_id)
		{
		case 1:
			return mouse_coord.X >= 0 && mouse_coord.X < 9 && mouse_coord.Y == 0;
		case 2:
			return mouse_coord.X >= 0 && mouse_coord.X < 9 && mouse_coord.Y == 1;
		}
		return false;
	}

	// マウスイベントを処理する。
	void mouse_event(MOUSE_EVENT_RECORD mouse_event)
	{
		const State last_state = state;
		switch (mouse_event.dwEventFlags)
		{
		case 0:
			if (mouse_event.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
			{
				// 左クリック
				if (hit_button(mouse_event.dwMousePosition, 1))
				{
					state.button_1_left++;
				}
				if (hit_button(mouse_event.dwMousePosition, 2))
				{
					state.button_2_left++;
				}
			}
			if (mouse_event.dwButtonState == RIGHTMOST_BUTTON_PRESSED)
			{
				// 右クリック
				if (hit_button(mouse_event.dwMousePosition, 1))
				{
					state.button_1_right++;
				}
				if (hit_button(mouse_event.dwMousePosition, 2))
				{
					state.button_2_right++;
				}
			}
		case MOUSE_MOVED:
			state.button_1_hover = hit_button(mouse_event.dwMousePosition, 1);
			state.button_2_hover = hit_button(mouse_event.dwMousePosition, 2);
			break;
		}

		if (last_state != state)
		{
			render();
		}
	}

	void error_exit(const char* msg)
	{
		std::cerr << msg << std::endl;
		exit(1);
	}

	void main()
	{
		DWORD cNumRead;
		INPUT_RECORD irInBuf[128];

		render();

		while (!stop)
		{
			if (!ReadConsoleInput(
				hStdin,      // input buffer handle
				irInBuf,     // buffer to read into
				128,         // size of read buffer
				&cNumRead)) // number of records read
				error_exit("ReadConsoleInput");

			for (DWORD i = 0; i < cNumRead; i++)
			{
				switch (irInBuf[i].EventType)
				{
				case KEY_EVENT: // keyboard input
					// KeyEventProc(irInBuf[i].Event.KeyEvent);
					break;

				case MOUSE_EVENT: // mouse input
					mouse_event(irInBuf[i].Event.MouseEvent);
					break;

				case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
					// ResizeEventProc(irInBuf[i].Event.WindowBufferSizeEvent);
					break;

				case FOCUS_EVENT:  // disregard focus events

				case MENU_EVENT:   // disregard menu events
					break;

				default:
					error_exit("Unknown event type");
					break;
				}
			}
		}
	}
};

int main()
{
	ConsoleGUIBase myGui;
	myGui.main();
}
