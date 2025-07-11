#pragma once

#include "Keyboard.h"

#define UNUSED_OPT -2

// 使わないキーはUNUSED_OPTに指定してください。
// 1つのキーだけを割り当てる場合はKeyboard.hの定義を優先してください。
// また、できるだけBIGの方を使用してください。(通常の文字キーの場合はBIGとSMALLの両方が必要になります。)
// 特殊なキーに割り当てると、正常に動作しない可能性があります。検証を行うことを推奨します。

#define KEYMAP_LEFT_BIG				KEY_INPUT_LEFT
#define KEYMAP_LEFT_SMALL			UNUSED_OPT

#define KEYMAP_RIGHT_BIG			KEY_INPUT_RIGHT
#define KEYMAP_RIGHT_SMALL			UNUSED_OPT

#define KEYMAP_DOWN_BIG				KEY_INPUT_DOWN
#define KEYMAP_DOWN_SMALL			UNUSED_OPT

#define KEYMAP_DROP_BIG				' '
#define KEYMAP_DROP_SMALL			UNUSED_OPT

#define KEYMAP_RIGHT_SPIN_BIG		'X'
#define KEYMAP_RIGHT_SPIN_SMALL		'x'

#define KEYMAP_LEFT_SPIN_BIG		'Z'
#define KEYMAP_LEFT_SPIN_SMALL		'z'

#define KEYMAP_HOLD_BIG				'C'
#define KEYMAP_HOLD_SMALL			'c'

// キーマップを変更した場合は下のM_KEYMAP_CUSTOMIZEDのコメントを解除してください。

/* #define M_KEYMAP_CUSTOMIZED */

#include "Console.h"
#include "Singleton.h"
#include <Windows.h>

const class KeyboardEx {
public:
	const void ShowUsage() {
		// キーマップを変更した場合は、内容も変更してください。
		// なお、関数の最初の値を変更しないでください。表示に問題が発生する可能性があります。
		PrintEx(0x2, "   ↑    | 右回転");
		PrintEx(0x3, " ←＋→  | ミノの移動");
		PrintEx(0x4, "   ↓    | ソフトドロップ (ゆっくり落下)");
		PrintEx(0x6, " Space   | ハードドロップ (即落下)");
		PrintEx(0x8, "   Z     | 左回転");
		PrintEx(0x9, "   X     | 右回転");
		PrintEx(0xA, "   C     | ホールド (保持)");
	}
private:
	const void PrintEx(SHORT y, const char* str) {
		Console::Instance()->Print(3, y, GetColor(15, 0), str);
	}
};
