/*
The MIT License (MIT)

Copyright (c) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <optional>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

using byte = uint8_t;
#include <aviutl/filter.hpp>

using namespace AviUtl;

////////////////////////////////
// ウィンドウフックのラッパー．
////////////////////////////////
class HookTarget
{
public:
	using HookProc = std::optional<LRESULT>(*)(/*HWND hwnd, */UINT message, WPARAM wparam, LPARAM lparam/*, HookTarget* that*/);

private:
	HookProc hookProc = nullptr;
	static LRESULT CALLBACK callback(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, uintptr_t uid, auto)
	{
		if (auto that = reinterpret_cast<HookTarget*>(uid);
			that->hookProc != nullptr) {
			if (auto ret = that->hookProc(/*hwnd, */message, wparam, lparam/*, that*/))
				return *ret;
		}
		return ::DefSubclassProc(hwnd, message, wparam, lparam);
	}
	auto hook_uid() { return reinterpret_cast<uintptr_t>(this); }

public:
	// hooking window procedures.
	bool setup_hook(HookProc func)
	{
		if (auto hwnd = get_hwnd(); hwnd != nullptr) {
			hookProc = func;
			return ::SetWindowSubclass(hwnd, callback, hook_uid(), 0) != FALSE;
		}
		return false;
	}
	bool remove_hook()
	{
		hookProc = nullptr;
		if (auto hwnd = get_hwnd(); hwnd != nullptr)
			return ::RemoveWindowSubclass(hwnd, callback, hook_uid()) != FALSE;
		return false;
	}

	void send_message(auto... args)
	{
		if (auto hwnd = get_hwnd(); hwnd != nullptr && ::IsWindowVisible(hwnd) != FALSE) {
			// 注意: 非表示の設定ダイアログに特定のメッセージを送ると例外が飛ぶ．
			auto t = hookProc; hookProc = nullptr;
			::SendMessageW(hwnd, args...);
			hookProc = t;
		}
	}

	virtual HWND get_hwnd() = 0;
};

// 拡張編集ウィンドウ．
inline class : public HookTarget {
	FilterPlugin* exeditfp = nullptr;
	static FilterPlugin* get_exeditfp(FilterPlugin* fp)
	{
		auto h_exedit = ::GetModuleHandleW(L"exedit.auf");
		SysInfo si; fp->exfunc->get_sys_info(nullptr, &si);
		for (int i = 0; i < si.filter_n; i++) {
			auto fp_other = fp->exfunc->get_filterp(i);
			if (fp_other->dll_hinst == h_exedit) return fp_other;
		}
		return nullptr;
	}

public:
	HWND get_hwnd() override { return exeditfp != nullptr ? exeditfp->hwnd : nullptr; }
	bool init(FilterPlugin* fp)
	{
		exeditfp = get_exeditfp(fp);
		return exeditfp != nullptr;
	}
} ExEditWindow;

// 設定ダイアログ．
inline class : public HookTarget {
	HWND hwnd = nullptr;
	static auto get_settingdlg()
	{
		return ::FindWindowW(L"ExtendedFilterClass", nullptr);
	}

public:
	HWND get_hwnd() override { return hwnd; }
	bool init()
	{
		hwnd = get_settingdlg();
		return hwnd != nullptr;
	}
} SettingDlg;

// メインウィンドウ．
inline struct {
	HWND hwnd = nullptr;
	auto send_message(auto... args)
	{
		return ::SendMessageW(hwnd, args...);
	}
} MainWindow;


////////////////////////////////
// 設定項目．
////////////////////////////////
inline constinit struct Settings {
	struct {
		bool Left = false, Right = false, Up = false, Down = false;
		constexpr bool is_effective() const { return Left || Right || Up || Down; }
	} Timeline;

	struct {
		bool Left = false, Right = false;
		constexpr bool is_effective() const { return Left || Right; }
	} SettingDlg;

	void load(const char* ini_file)
	{
#define load_val(wnd, key)	wnd.key = 0 != ::GetPrivateProfileIntA(#wnd, "hook" #key, 0, ini_file)
		load_val(Timeline, Left);
		load_val(Timeline, Right);
		load_val(Timeline, Up);
		load_val(Timeline, Down);
		load_val(SettingDlg, Left);
		load_val(SettingDlg, Right);
#undef load
	}
} settings;


////////////////////////////////
// 設定ロードセーブ．
////////////////////////////////

// replacing a file extension when it's known.
template<class T, size_t len_max, size_t len_old, size_t len_new>
static void replace_tail(T(&dst)[len_max], size_t len, const T(&tail_old)[len_old], const T(&tail_new)[len_new])
{
	if (len < len_old || len - len_old + len_new > len_max) return;
	std::memcpy(dst + len - len_old, tail_new, len_new * sizeof(T));
}
inline void load_settings(HMODULE h_dll)
{
	char path[MAX_PATH];
	replace_tail(path, ::GetModuleFileNameA(h_dll, path, std::size(path)) + 1, "auf", "ini");

	settings.load(path);
}


////////////////////////////////
// 各種コマンド実行の実装．
////////////////////////////////
struct Menu {
	enum ID : int32_t {
		Invalid = -1,

		ExEditLeft = 1,
		ExEditRight,
		ExEditUp,
		ExEditDown,

		SettingDlgLeft,
		SettingDlgRight,
	};
	static constexpr bool is_invalid(ID id) { return id <= Invalid; }
	static constexpr bool is_timeline(ID id) { return id <= ExEditDown; }
	static constexpr bool is_settingdlg(ID id) { return id <= SettingDlgRight; }

	inline constexpr static struct { ID id; const char* name; } Items[] = {
		{ ExEditLeft,		"タイムライン左入力" },
		{ ExEditRight,		"タイムライン右入力" },
		{ ExEditUp,			"タイムライン上入力" },
		{ ExEditDown,		"タイムライン下入力" },
		{ SettingDlgLeft,	"設定ダイアログ左入力" },
		{ SettingDlgRight,	"設定ダイアログ右入力" },
	};
};

bool menu_timeline_handler(Menu::ID id)
{
	WPARAM vk = 0;
	switch (id) {
	case Menu::ExEditLeft:	vk = VK_LEFT;	break;
	case Menu::ExEditRight:	vk = VK_RIGHT;	break;
	case Menu::ExEditUp:	vk = VK_UP;		break;
	case Menu::ExEditDown:	vk = VK_DOWN;	break;
	}
	if (vk != 0) ExEditWindow.send_message(WM_KEYDOWN, vk, 0);
	return false;
}

bool menu_setting_dlg_handler(Menu::ID id)
{
	WPARAM vk = 0;
	switch (id) {
	case Menu::SettingDlgLeft:	vk = VK_LEFT;	break;
	case Menu::SettingDlgRight:	vk = VK_RIGHT;	break;
	}
	if (vk != 0) SettingDlg.send_message(WM_KEYDOWN, vk, 0);
	return false;
}


////////////////////////////////
// フックのコールバック関数．
////////////////////////////////
std::optional<LRESULT> exedit_hook(/*HWND hwnd, */UINT message, WPARAM wparam, LPARAM lparam/*, HookTarget* that*/)
{
	switch (message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if ([=] {
			switch (wparam) {
			case VK_LEFT:	return settings.Timeline.Left;
			case VK_RIGHT:	return settings.Timeline.Right;
			case VK_UP:		return settings.Timeline.Up;
			case VK_DOWN:	return settings.Timeline.Down;
			default: return false;
			}
		}()) return MainWindow.send_message(message, wparam, lparam);
		break;
	}
	return std::nullopt;
}

std::optional<LRESULT> setting_dlg_hook(/*HWND hwnd, */UINT message, WPARAM wparam, LPARAM lparam/*, HookTarget* that*/)
{
	switch (message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if ([=] {
			switch (wparam) {
			case VK_LEFT:	return settings.SettingDlg.Left;
			case VK_RIGHT:	return settings.SettingDlg.Right;
			default: return false;
			}
		}()) return MainWindow.send_message(message, wparam, lparam);
		break;
	}
	return std::nullopt;
}


////////////////////////////////
// AviUtlに渡す関数の定義．
////////////////////////////////
BOOL func_init(FilterPlugin* fp)
{
	// 拡張編集の確認．
	if (!ExEditWindow.init(fp)) {
		::MessageBoxA(fp->hwnd, "拡張編集が見つかりませんでした．",
			fp->name, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	// メインウィンドウのハンドルを取得，保存．
	MainWindow.hwnd = fp->hwnd_parent;

	// message-only window を作成，登録．これで NoConfig でも AviUtl からメッセージを受け取れる.
	fp->hwnd = ::CreateWindowExW(0, L"AviUtl", L"", 0, 0, 0, 0, 0,
		HWND_MESSAGE, nullptr, fp->hinst_parent, nullptr);

	// メニュー登録．
	for (auto [id, name] : Menu::Items)
		fp->exfunc->add_menu_item(fp, name, fp->hwnd, id, 0, ExFunc::AddMenuItemFlag::None);

	// 設定ロード．
	load_settings(fp->dll_hinst);

	return TRUE;
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, EditHandle* editp, FilterPlugin* fp)
{
	using Message = FilterPlugin::WindowMessage;
	switch (message) {
		// 拡張編集タイムライン/設定ダイアログのフック/アンフック．
	case Message::ChangeWindow:
		SettingDlg.init();
		if (settings.Timeline.is_effective()) ExEditWindow.setup_hook(exedit_hook);
		if (settings.SettingDlg.is_effective()) SettingDlg.setup_hook(setting_dlg_hook);
		break;
	case Message::Exit:
		if (settings.Timeline.is_effective()) ExEditWindow.remove_hook();
		if (settings.SettingDlg.is_effective()) SettingDlg.remove_hook();

		// message-only window を削除．必要ないかもしれないけど．
		fp->hwnd = nullptr; ::DestroyWindow(hwnd);
		break;

	case Message::Command:
		// コマンドハンドラ．
		if (auto id = static_cast<Menu::ID>(wparam); !Menu::is_invalid(id) &&
			fp->exfunc->is_editing(editp) && !fp->exfunc->is_saving(editp)) {
			return (
				Menu::is_timeline(id)   ? menu_timeline_handler(id) :
				Menu::is_settingdlg(id) ? menu_setting_dlg_handler(id) :
				false) ? TRUE : FALSE;
		}
		break;
	}
	return FALSE;
}


////////////////////////////////
// 看板．
////////////////////////////////
#define PLUGIN_NAME		"Allow Arrow"
#define PLUGIN_VERSION	"v1.00"
#define PLUGIN_AUTHOR	"sigma-axis"
#define PLUGIN_INFO_FMT(name, ver, author)	(name##" "##ver##" by "##author)

extern "C" __declspec(dllexport) FilterPluginDLL* __stdcall GetFilterTable(void)
{
	// （フィルタとは名ばかりの）看板．
	using Flag = FilterPlugin::Flag;
	static constinit FilterPluginDLL filter {
		.flag = Flag::AlwaysActive | Flag::ExInformation | Flag::NoConfig,
		.name = PLUGIN_NAME,
		.func_init = func_init,
		.func_WndProc = func_WndProc,
		.information = PLUGIN_INFO_FMT(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR),
	};
	return &filter;
}
