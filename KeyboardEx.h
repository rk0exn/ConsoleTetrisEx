#pragma once

#include "Keyboard.h"

#define UNUSED_OPT -2

// �g��Ȃ��L�[��UNUSED_OPT�Ɏw�肵�Ă��������B
// 1�̃L�[���������蓖�Ă�ꍇ��Keyboard.h�̒�`��D�悵�Ă��������B
// �܂��A�ł��邾��BIG�̕����g�p���Ă��������B(�ʏ�̕����L�[�̏ꍇ��BIG��SMALL�̗������K�v�ɂȂ�܂��B)
// ����ȃL�[�Ɋ��蓖�Ă�ƁA����ɓ��삵�Ȃ��\��������܂��B���؂��s�����Ƃ𐄏����܂��B

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

// �L�[�}�b�v��ύX�����ꍇ�͉���M_KEYMAP_CUSTOMIZED�̃R�����g���������Ă��������B

/* #define M_KEYMAP_CUSTOMIZED */

#include "Console.h"
#include "Singleton.h"
#include <Windows.h>

const class KeyboardEx {
public:
	const void ShowUsage() {
		// �L�[�}�b�v��ύX�����ꍇ�́A���e���ύX���Ă��������B
		// �Ȃ��A�֐��̍ŏ��̒l��ύX���Ȃ��ł��������B�\���ɖ�肪��������\��������܂��B
		PrintEx(0x2, "   ��    | �E��]");
		PrintEx(0x3, " ���{��  | �~�m�̈ړ�");
		PrintEx(0x4, "   ��    | �\�t�g�h���b�v (������藎��)");
		PrintEx(0x6, " Space   | �n�[�h�h���b�v (������)");
		PrintEx(0x8, "   Z     | ����]");
		PrintEx(0x9, "   X     | �E��]");
		PrintEx(0xA, "   C     | �z�[���h (�ێ�)");
	}
private:
	const void PrintEx(SHORT y, const char* str) {
		Console::Instance()->Print(3, y, GetColor(15, 0), str);
	}
};
