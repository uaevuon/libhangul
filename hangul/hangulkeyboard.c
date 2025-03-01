/* libhangul
 * Copyright (C) 2016 Choe Hwanjin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#if ENABLE_EXTERNAL_KEYBOARDS
#include <locale.h>
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif /* HAVE_GLOB_H */
#include <expat.h>
#endif /* ENABLE_EXTERNAL_KEYBOARDS */

#include "hangul-gettext.h"
#include "hangul.h"
#include "hangulinternals.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fileapi.h>
#define strdup _strdup
#endif /* _WIN32 */

/**
 * @file hangulkeyboard.c
 */

/**
 * @addtogroup hangulkeyboards
 *
 * @section addinghangulkeyboards 한글 키보드 추가 방법
 * libhangul이 한글 키보드 파일을 읽는 위치는 두 곳이다.
 *  - @$(pkgdatadir)/keyboards 디렉토리에 설치된 파일은 전역 시스템에서
 *    인식한다.
 *  - @$HOME/.local/share/libhangul/keyboards 또는 @$XDG_DATA_HOME/libhangul/keyboards
 *    디렉토리에 설치된 파일은 개별 사용자만 인식한다.
 *
 * 키보드 파일의 로딩 순서는 시스템 파일을 먼저 로딩하고 사용자 파일을
 * 로딩한다. 따라서 한글 키보드 목록을 이터레이션하면 사용자 추가 자판은
 * 마지막에 나오게 된다. 사용자 자판 선택 알고리듬이 처음 나온 자판을
 * 인식하는 방식이므로 동일한 id를 가진 자판을 등록하면 먼저 등록된 자판만
 * 인식되므로 주의가 필요하다. 다시 말해서 시스템 자판과 같은 id를 가진
 * 자판은 등록하여 사용할 수 없다.
 */

#define LIBHANGUL_KEYBOARD_DIR LIBHANGUL_DATA_DIR "/keyboards"

#define HANGUL_KEYBOARD_TABLE_SIZE 0x80



// 세벌식 확장모드 글쇠
//0 : 같은 기호 배열을 쓴다 // ㅗ, ㅜ
const char sebeol_3_symbol_key[] = {'0', 'v', '8', 0x00};
//1 : 다른 기호 배열을 쓴다// ㅗ, ㅜ 
const char sebeol_3yet_symbol_key[] = {'1', '/', '9', 0x00};
// J + [ K, L, : ] //2: 같은 기호배열을 쓰고 준비글쇠가 있다.
const char sebeol_3moa_symbol_key[] = {'2', 'J', 'K', 'L', ':', 0x00};
// j + [ k, l, ;]
const char sebeol_3shin_symbol_key[] = {'j', 'k', 'l', ';', 0x00};

// ㅗ, ㅜ
const ucschar sebeol_3_symbol_value[] = {0x1169, 0x116e, 0x0000};  
// 첫소리 ㅇ [J]
const ucschar sebeol_3moa_symbol_value[] = {0x110b, 0x0000};
// 첫소리 ㅇ [j]
const ucschar sebeol_3shin_symbol_value[] = {0x110b, 0x0000};

// 세벌식 옛한글
  //6 : 옛글 배열,  ㅖ, ㅢ  // 공병우 계열
const char sebeol_3yet_yetgeul_key[] = {'6', '7', '8', 0x00};
// ㅖ, ㅢ  // 공병우 계열
const ucschar sebeol_3yet_yetgeul_value[] = {0x1168, 0x1174, 0x0000};

// 세벌식 확장단계 표시
// ®, ①, ②, ③, ④, ⑤
const ucschar sebeol_3_ext_step[] = {0x00AE, 0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x0000};
// 세벌식 겹홀소리 글쇠
  // ㅡ,ㅗ, ㅜ  // 공병우 계열
const char sebeol_3_moeum_key[] = {'8', '/', '9', 0x00};
  // ㅡ, ㅗ, ㅜ  // 신광조 계열
const char sebeol_3shin_moeum_key[] = {'I', 'O', 'P', 0x00};
// ㅗ, ㅜ  // 신세기 계열 2018
const char sebeol_3moa_semoe_2018_moeum_key[] = {'.', 'b', 0x00};
 // ㅗ, ㅜ  // 신세기 계열 2017
const char sebeol_3moa_semoe_2017_moeum_key[] = {'.', 'p', 0x00};
 // ㅗ, ㅜ  // 신세기 계열 2016
const char sebeol_3moa_semoe_2016_moeum_key[] = {'[', 'p', 0x00};
 // ㅗ, ㅜ  // 신세기 계열 2014, 2015
const char sebeol_3moa_semoe_moeum_key_deprecated[] = {'\'', 'p', 0x00};
 // ㅗ, ㅜ  // 신세기 계열
//char sebeol_3moa_semoe_2015_moeum_key[] = {';', 'p', 0x00};
  // ㅗ, ㅜ, ㅡ
const ucschar sebeol_3_moeum_value[] = {0x1169, 0x116e, 0x1173, 0x0000};

#ifndef INIT_IDS_LENGTH
#define INIT_IDS_LENGTH 9
#endif
static const char *keys[INIT_IDS_LENGTH] =
            { "2", "2noshift", 
                "3-90", "3-91", "3-p3", 
                "3moa-semoe-2018", 
                "3shin-2003", "3shin-p2",
                NULL};

typedef struct _HangulCombinationItem HangulCombinationItem;

struct _HangulCombinationItem {
    uint32_t key;
    ucschar code;
};

struct _HangulCombination {
    size_t size;
    size_t size_alloced;
    HangulCombinationItem *table;

    bool is_static;
};

struct _HangulKeyboard {
    char* id;
    char* name;
    // [기본배열]
    ucschar* table[4];
    // [기본조합,추가조합,갈마들이조합]
    HangulCombination* combination[4];

    int type;
    bool is_static;
    
    // 3beol
    ucschar replace_it; // 바꿔 놓기 : 세벌식의 ] -> 아래아
    // 확장배열씀, 갈마들이켜끄기됨, 입력순서〈안〉따짐, 왼/오른ㅗㅜ구분함, 확장겹받침허용〈안〉함
    bool flag[5]; //
    // [모음글쇠, 확장기호글쇠, 확장한글글쇠, ]
    char* addon_key[4];
    // [모음값, 확장기호값, 확장한글값, 확장단계기호값]
    ucschar* addon_value[4];
    // [기호확장함수, 한글확장함수]
    ucschar (*addon_func[2])(int, int, int);
};

typedef struct _HangulKeyboardList {
    size_t n;
    size_t nalloced;
    HangulKeyboard** keyboards;
} HangulKeyboardList;

#include "hangulkeyboard.h"


static const HangulCombination hangul_combination_default_2 = {
    countof(hangul_combination_table_default_2),
    countof(hangul_combination_table_default_2),
    (HangulCombinationItem*)hangul_combination_table_default_2,
    true
};


static const HangulCombination hangul_combination_default_3 = {
    countof(hangul_combination_table_default_3),
    countof(hangul_combination_table_default_3),
    (HangulCombinationItem*)hangul_combination_table_default_3
};

static const HangulCombination hangul_combination_romaja = {
    countof(hangul_combination_table_romaja),
    countof(hangul_combination_table_romaja),
    (HangulCombinationItem*)hangul_combination_table_romaja,
    true
};

static const HangulCombination hangul_combination_full = {
    countof(hangul_combination_table_full),
    countof(hangul_combination_table_full),
    (HangulCombinationItem*)hangul_combination_table_full,
    true
};

static const HangulCombination hangul_combination_ahn = {
    countof(hangul_combination_table_ahn),
    countof(hangul_combination_table_ahn),
    (HangulCombinationItem*)hangul_combination_table_ahn,
    true
};

static const HangulCombination hangul_combination_3_91_noshift = {
    countof(hangul_combination_table_3_91_noshift),
    countof(hangul_combination_table_3_91_noshift),
    (HangulCombinationItem*)hangul_combination_table_3_91_noshift,
    true
};

static const HangulCombination hangul_combination_3_2015 = {
    countof(hangul_combination_table_3_3_2015),
    countof(hangul_combination_table_3_3_2015),
    (HangulCombinationItem*)hangul_combination_table_3_3_2015,
    true
};

static const HangulCombination hangul_combination_3_2015_yet = {
    countof(hangul_combination_table_full_3_2015_yet),
    countof(hangul_combination_table_full_3_2015_yet),
    (HangulCombinationItem*)hangul_combination_table_full_3_2015_yet,
    true
};

static const HangulCombination hangul_combination_3_14_proposal = {
    countof(hangul_combination_table_3_3_14_proposal),
    countof(hangul_combination_table_3_3_14_proposal),
    (HangulCombinationItem*)hangul_combination_table_3_3_14_proposal,
    true
};

static const HangulCombination hangul_combination_3sun_2014 = {
    countof(hangul_combination_table_3_3sun_2014),
    countof(hangul_combination_table_3_3sun_2014),
    (HangulCombinationItem*)hangul_combination_table_3_3sun_2014,
    true
};

static const HangulCombination hangul_combination_3gimguk_38a_yet = {
    countof(hangul_combination_table_full_3gimguk_38A_yet),
    countof(hangul_combination_table_full_3gimguk_38A_yet),
    (HangulCombinationItem*)hangul_combination_table_full_3gimguk_38A_yet,
    true
};

static const HangulCombination hangul_combination_3moa_semoe_2014 = {
    countof(hangul_combination_table_3moa_semoe_2014),
    countof(hangul_combination_table_3moa_semoe_2014),
    (HangulCombinationItem*)hangul_combination_table_3moa_semoe_2014,
    true
};

static const HangulCombination hangul_combination_3moa_semoe_2015 = {
    countof(hangul_combination_table_3moa_semoe_2015),
    countof(hangul_combination_table_3moa_semoe_2015),
    (HangulCombinationItem*)hangul_combination_table_3moa_semoe_2015,
    true
};

static const HangulCombination hangul_combination_3moa_semoe_2016 = {
    countof(hangul_combination_table_3moa_semoe_2016),
    countof(hangul_combination_table_3moa_semoe_2016),
    (HangulCombinationItem*)hangul_combination_table_3moa_semoe_2016,
    true
};

static const HangulCombination hangul_combination_3moa_semoe_2017 = {
    countof(hangul_combination_table_3moa_semoe_2017),
    countof(hangul_combination_table_3moa_semoe_2017),
    (HangulCombinationItem*)hangul_combination_table_3moa_semoe_2017,
    true
};

static const HangulCombination hangul_combination_3moa_semoe_2018 = {
    countof(hangul_combination_table_3moa_semoe_2018),
    countof(hangul_combination_table_3moa_semoe_2018),
    (HangulCombinationItem*)hangul_combination_table_3moa_semoe_2018,
    true
};

static const HangulCombination hangul_replace_2_noshift = {
    countof(hangul_replace_table_2_noshift),
    countof(hangul_replace_table_2_noshift),
    (HangulCombinationItem*)hangul_replace_table_2_noshift,
    true
};

static const HangulCombination hangul_combination_3shin_2015 = {
    countof(hangul_combination_table_3_3shin_2015),
    countof(hangul_combination_table_3_3shin_2015),
    (HangulCombinationItem*)hangul_combination_table_3_3shin_2015,
    true
};

static const HangulCombination hangul_combination_3shin_p_yet = {
    countof(hangul_combination_table_full_3shin_p),
    countof(hangul_combination_table_full_3shin_p),
    (HangulCombinationItem*)hangul_combination_table_full_3shin_p,
    true
};

static const HangulCombination hangul_combination_3shin_p2_yet = {
    countof(hangul_combination_table_full_3shin_p),
    countof(hangul_combination_table_full_3shin_p),
    (HangulCombinationItem*)hangul_combination_table_full_3shin_p,
    true
};

static const HangulCombination hangul_galmadeuli_3_2014 = {
    countof(hangul_galmadeuli_table_3_2014),
    countof(hangul_galmadeuli_table_3_2014),
    (HangulCombinationItem*)hangul_galmadeuli_table_3_2014,
    true
};

static const HangulCombination hangul_galmadeuli_3_2015 = {
    countof(hangul_galmadeuli_table_3_2015),
    countof(hangul_galmadeuli_table_3_2015),
    (HangulCombinationItem*)hangul_galmadeuli_table_3_2015,
    true
};

static const HangulCombination hangul_galmadeuli_3_2015_metal = {
    countof(hangul_galmadeuli_table_3_2015_metal),
    countof(hangul_galmadeuli_table_3_2015_metal),
    (HangulCombinationItem*)hangul_galmadeuli_table_3_2015_metal,
    true
};

static const HangulCombination hangul_galmadeuli_3_2015_patal = {
    countof(hangul_galmadeuli_table_3_2015_patal),
    countof(hangul_galmadeuli_table_3_2015_patal),
    (HangulCombinationItem*)hangul_galmadeuli_table_3_2015_patal,
    true
};

static const HangulCombination hangul_galmadeuli_3_p3 = {
    countof(hangul_galmadeuli_table_3_p3),
    countof(hangul_galmadeuli_table_3_p3),
    (HangulCombinationItem*)hangul_galmadeuli_table_3_p3,
    true
};

static const HangulCombination hangul_galmadeuli_3_14_proposal = {
    countof(hangul_galmadeuli_table_3_14_proposal),
    countof(hangul_galmadeuli_table_3_14_proposal),
    (HangulCombinationItem*)hangul_galmadeuli_table_3_14_proposal,
    true
};

static const HangulCombination hangul_galmadeuli_3shin_1995 = {
    countof(hangul_galmadeuli_table_3shin_1995),
    countof(hangul_galmadeuli_table_3shin_1995),
    (HangulCombinationItem*)hangul_galmadeuli_table_3shin_1995,
    true
};

static const HangulCombination hangul_galmadeuli_3shin_2003 = {
    countof(hangul_galmadeuli_table_3shin_2003),
    countof(hangul_galmadeuli_table_3shin_2003),
    (HangulCombinationItem*)hangul_galmadeuli_table_3shin_2003,
    true
};

static const HangulCombination hangul_galmadeuli_3shin_2012 = {
    countof(hangul_galmadeuli_table_3shin_2012),
    countof(hangul_galmadeuli_table_3shin_2012),
    (HangulCombinationItem*)hangul_galmadeuli_table_3shin_2012,
    true
};

static const HangulCombination hangul_galmadeuli_3shin_2015 = {
    countof(hangul_galmadeuli_table_3shin_2015),
    countof(hangul_galmadeuli_table_3shin_2015),
    (HangulCombinationItem*)hangul_galmadeuli_table_3shin_2015,
    true
};

static const HangulCombination hangul_galmadeuli_3shin_m = {
    countof(hangul_galmadeuli_table_3shin_m),
    countof(hangul_galmadeuli_table_3shin_m),
    (HangulCombinationItem*)hangul_galmadeuli_table_3shin_m,
    true
};

static const HangulCombination hangul_galmadeuli_3shin_p = {
    countof(hangul_galmadeuli_table_3shin_p),
    countof(hangul_galmadeuli_table_3shin_p),
    (HangulCombinationItem*)hangul_galmadeuli_table_3shin_p,
    true
};

static const HangulCombination hangul_galmadeuli_3shin_p2 = {
    countof(hangul_galmadeuli_table_3shin_p2),
    countof(hangul_galmadeuli_table_3shin_p2),
    (HangulCombinationItem*)hangul_galmadeuli_table_3shin_p2,
    true
};

static const HangulCombination hangul_galmadeuli_3moa_semoe_2018 = {
    countof(hangul_galmadeuli_table_3moa_semoe_2018),
    countof(hangul_galmadeuli_table_3moa_semoe_2018),
    (HangulCombinationItem*)hangul_galmadeuli_table_3moa_semoe_2018,
    true
};


static const HangulKeyboard hangul_keyboard_2 = {
    (char*)"2",
    (char*)N_("Dubeolsik KSX 5002"),
    { (ucschar*)hangul_keyboard_table_2, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_2, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JAMO,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_2y = {
    (char*)"2y",
    (char*)N_("Dubeolsik Yetgeul"),
    { (ucschar*)hangul_keyboard_table_2y, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_full, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JAMO_YET,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_32 = {
    (char*)"32",
    (char*)N_("Sebeolsik Dubeol Layout"),
    { (ucschar*)hangul_keyboard_table_32, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_2, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3_90 = {
    (char*)"3-90",
    (char*)N_("Sebeolsik 3-90"),
    { (ucschar*)hangul_keyboard_table_390, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x119e,
    {false, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3_91_final = {
    (char*)"3-91",
    (char*)N_("Sebeolsik 3-91 Final"),
    { (ucschar*)hangul_keyboard_table_3final, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x119e,
    {false, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3sun_1990 = {
    (char*)"3sun-1990",
    (char*)N_("Sebeolsik Noshift 1990"),
    { (ucschar*)hangul_keyboard_table_3sun, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3_93_yet = {
    (char*)"3-93-yet",
    (char*)N_("Sebeolsik 3-93 Yetgeul"),
    { (ucschar*)hangul_keyboard_table_3yet, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_full, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_YET,
    true,
    0x0000,
    {false, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_romaja = {
    (char*)"ro",
    (char*)N_("Romaja"),
    { (ucschar*)hangul_keyboard_table_romaja, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_romaja, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_ROMAJA,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_ahn = {
    (char*)"ahn",
    (char*)N_("Ahnmatae"),
    { (ucschar*)hangul_keyboard_table_ahn, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_ahn, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};

// 3beol
static const HangulKeyboard hangul_keyboard_2noshift = {
    (char*)"2noshift",
    (char*)N_("Dubeolsik Noshift"),
    { (ucschar*)hangul_keyboard_table_2, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_2, //기본조합
      NULL,//추가조합
      (HangulCombination*)&hangul_replace_2_noshift, //갈마들이조합
      NULL
    },
    HANGUL_KEYBOARD_TYPE_JAMO,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};


static const HangulKeyboard hangul_keyboard_2north9256 = {
    (char*)"2n9256",
    (char*)N_("Dubeolsik North 9256"),
    { (ucschar*)hangul_keyboard_table_2north9256, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_2, //기본조합
      NULL,//추가조합
      (HangulCombination*)&hangul_replace_2_noshift, //갈마들이조합
      NULL
    },
    HANGUL_KEYBOARD_TYPE_JAMO,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3_89 = {
    (char*)"3-89",
    (char*)N_("Sebeolsik 3-89"),
    { (ucschar*)hangul_keyboard_table_3_89, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3,//기본조합
      NULL,//추가조합
      NULL,//갈마들이조합
      NULL
    },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3_91_final_noshift = {
    (char*)"3-91-noshift",
    (char*)N_("Sebeolsik 3-91 Final Noshift"),
    { (ucschar*)hangul_keyboard_table_3_91_final_noshift, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_3_91_noshift,//기본조합
      NULL,//추가조합
      NULL,//갈마들이조합
      NULL
    },
    HANGUL_KEYBOARD_TYPE_3FINALSUN,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};


static const HangulKeyboard hangul_keyboard_3_95 = {
    (char*)"3-95",
    (char*)N_("Sebeolsik 3-95"),
    { (ucschar*)hangul_keyboard_table_3_95, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x119e,
    {false, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3ahnmatae = {
    (char*)"3-ahn",
    (char*)N_("Sebeolsik 3-Ahnmatae"),
    { (ucschar*)hangul_keyboard_table_3ahnmatae, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_ahn, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3_2011 = {
    (char*)"3-2011",
    (char*)N_("Sebeolsik 3-2011"),
    { (ucschar*)hangul_keyboard_table_3_2011, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x119e,
    {true, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, (char*)&sebeol_3_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_3_2011, NULL}
};

static const HangulKeyboard hangul_keyboard_3_2011_yet = {
    (char*)"3-2011-yet",
    (char*)N_("Sebeolsik 3-2011 Yetgeul"),
    { (ucschar*)hangul_keyboard_table_3_2011, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_full, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_YET,
    true,
    0x119e,
    {true, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, (char*)&sebeol_3yet_symbol_key, (char*)&sebeol_3yet_yetgeul_key, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_3_2011_yet, 
        (ucschar(*)(int, int, int))&hangul_ascii_to_hanguel_3_yet
    }
};

static const HangulKeyboard hangul_keyboard_3_2012 = {
    (char*)"3-2012",
    (char*)N_("Sebeolsik 3-2012"),
    { (ucschar*)hangul_keyboard_table_3_2012, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x119e,
    {true, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, (char*)&sebeol_3_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_3_2012, NULL}
};

static const HangulKeyboard hangul_keyboard_3_2012_yet = {
    (char*)"3-2012-yet",
    (char*)N_("Sebeolsik 3-2012 Yetgeul"),
    { (ucschar*)hangul_keyboard_table_3_2012, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_full, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_YET,
    true,
    0x119e,
    {true, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, (char*)&sebeol_3yet_symbol_key, (char*)&sebeol_3yet_yetgeul_key, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_3_2012_yet, 
        (ucschar(*)(int, int, int))&hangul_ascii_to_hanguel_3_yet
    }
};

static const HangulKeyboard hangul_keyboard_3_2014 = {
    (char*)"3-2014",
    (char*)N_("Sebeolsik 3-2014"),
    { (ucschar*)hangul_keyboard_table_3_2014, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3, NULL, (HangulCombination*)&hangul_galmadeuli_3_2014, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x119e,
    {true, true, false, true, false},
    {(char*)&sebeol_3_moeum_key, (char*)&sebeol_3yet_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_3_2012_yet, NULL}
};

static const HangulKeyboard hangul_keyboard_3_2014_yet = {
    (char*)"3-2014-yet",
    (char*)N_("Sebeolsik 3-2014 Yetgeul"),
    { (ucschar*)hangul_keyboard_table_3_2014, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_full, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_YET,
    true,
    0x119e,
    {true, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, (char*)&sebeol_3yet_symbol_key, (char*)&sebeol_3yet_yetgeul_key, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_3_2012_yet, 
        (ucschar(*)(int, int, int))&hangul_ascii_to_hanguel_3_yet
    }
};

static const HangulKeyboard hangul_keyboard_3_2015 = {
    (char*)"3-2015",
    (char*)N_("Sebeolsik 3-2015"),
    { (ucschar*)hangul_keyboard_table_3_2015, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        (HangulCombination*)&hangul_combination_3_2015, 
        (HangulCombination*)&hangul_galmadeuli_3_2015, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, true, false, true, true},
    {(char*)&sebeol_3_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3_2015_yet = {
    (char*)"3-2015-yet",
    (char*)N_("Sebeolsik 3-2015 Yetgeul"),
    { (ucschar*)hangul_keyboard_table_3_2015_yet, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_full, 
        (HangulCombination*)&hangul_combination_3_2015_yet, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3_2015_metal = {
    (char*)"3-2015-metal",
    (char*)N_("Sebeolsik 3-2015M"),
    { (ucschar*)hangul_keyboard_table_3_2015_metal, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        (HangulCombination*)&hangul_combination_3_2015, 
        (HangulCombination*)&hangul_galmadeuli_3_2015_metal, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, true, false, true, true},
    {(char*)&sebeol_3_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3_2015_patal = {
    (char*)"3-2015-patal",
    (char*)N_("Sebeolsik 3-2015P"),
    { (ucschar*)hangul_keyboard_table_3_2015_patal, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        NULL, 
        (HangulCombination*)&hangul_galmadeuli_3_2015_patal, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x119e,
    {true, true, false, true, false},
    {(char*)&sebeol_3_moeum_key, (char*)&sebeol_3yet_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_3_2012_yet, NULL}
};

static const HangulKeyboard hangul_keyboard_3_2015_patal_yet = {
    (char*)"3-2015-patal-yet",
    (char*)N_("Sebeolsik 3-2015P Yetgeul"),
    { (ucschar*)hangul_keyboard_table_3_2015_patal, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_full, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x119e,
    {true, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, (char*)&sebeol_3yet_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_3_2012_yet, 
        (ucschar(*)(int, int, int))&hangul_ascii_to_hanguel_3_yet
    }
};

static const HangulKeyboard hangul_keyboard_3_p2 = {
    (char*)"3-p2",
    (char*)N_("Sebeolsik 3-P2"),
    { (ucschar*)hangul_keyboard_table_3_p2, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3, NULL, (HangulCombination*)&hangul_galmadeuli_3_p3, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x119e,
    {true, true, false, true, true},
    {(char*)&sebeol_3_moeum_key, (char*)&sebeol_3yet_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_3_2012_yet, NULL}
};

static const HangulKeyboard hangul_keyboard_3_p3 = {
    (char*)"3-p3",
    (char*)N_("Sebeolsik 3-P3"),
    { (ucschar*)hangul_keyboard_table_3_p3, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3, 
        NULL, 
        (HangulCombination*)&hangul_galmadeuli_3_p3, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x119e,
    {true, true, false, true, true},
    {(char*)&sebeol_3_moeum_key, (char*)&sebeol_3yet_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_3_p3, NULL}
};

static const HangulKeyboard hangul_keyboard_3_14_proposal = {
    (char*)"3-14-proposal",
    (char*)N_("Sebeolsik 3-14 Proposal"),
    { (ucschar*)hangul_keyboard_table_3_14_proposal, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        (HangulCombination*)&hangul_combination_3_14_proposal,
        (HangulCombination*)&hangul_galmadeuli_3_14_proposal, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, true, false, true, false},
    {(char*)&sebeol_3_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3moa_semoe_2014 = {
    (char*)"3moa-semoe-2014",
    (char*)N_("Sebeolsik Semoe 2014"),
    { (ucschar*)hangul_keyboard_table_3moa_semoe_2014, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_3moa_semoe_2014, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, false, true, false, false},
    {(char*)&sebeol_3moa_semoe_moeum_key_deprecated, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3moa_semoe_2015 = {
    (char*)"3moa-semoe-2015",
    (char*)N_("Sebeolsik Semoe 2015"),
    { (ucschar*)hangul_keyboard_table_3moa_semoe_2015, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_3moa_semoe_2015, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, false, true, false, false},
    {(char*)&sebeol_3moa_semoe_moeum_key_deprecated, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3moa_semoe_2016 = {
    (char*)"3moa-semoe-2016",
    (char*)N_("Sebeolsik Semoe 2016"),
    { (ucschar*)hangul_keyboard_table_3moa_semoe_2016, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_3moa_semoe_2016, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {true, false, true, false, false},
    {(char*)&sebeol_3moa_semoe_2016_moeum_key, (char*)&sebeol_3moa_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_semoe, NULL}
};

static const HangulKeyboard hangul_keyboard_3moa_semoe_2017 = {
    (char*)"3moa-semoe-2017",
    (char*)N_("Sebeolsik Semoe 2017"),
    { (ucschar*)hangul_keyboard_table_3moa_semoe_2017, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_3moa_semoe_2017, NULL, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {true, false, true, false, false},
    {(char*)&sebeol_3moa_semoe_2017_moeum_key, (char*)&sebeol_3moa_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_semoe, NULL}
};

static const HangulKeyboard hangul_keyboard_3moa_semoe_2018 = {
    (char*)"3moa-semoe-2018",
    (char*)N_("Sebeolsik Semoe 2018"),
    { (ucschar*)hangul_keyboard_table_3moa_semoe_2018, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_3moa_semoe_2018, 
        (HangulCombination*)&hangul_galmadeuli_3moa_semoe_2018, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {true, false, true, false, false},
    {(char*)&sebeol_3moa_semoe_2018_moeum_key, (char*)&sebeol_3moa_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_semoe, NULL}
};

static const HangulKeyboard hangul_keyboard_3sun_2014 = {
    (char*)"3sun-2014",
    (char*)N_("Sebeolsik Noshift 2014"),
    { (ucschar*)hangul_keyboard_table_3sun_2014, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        (HangulCombination*)&hangul_combination_3sun_2014, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, false, false, false, false},
    {(char*)&sebeol_3_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3gimguk_38a_yet = {
    (char*)"3gimguk-38a-yet",
    (char*)N_("Sebeolsik 3Gimguk-38A Yetgeul"),
    { (ucschar*)hangul_keyboard_table_3gimguk_38A_yet, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_full, 
        (HangulCombination*)&hangul_combination_3gimguk_38a_yet, NULL, NULL },
    HANGUL_KEYBOARD_TYPE_JASO,
    true,
    0x0000,
    {false, false, false, false, false},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3shin_1995 = {
    (char*)"3shin-1995",
    (char*)N_("Sebeolsik Shin 1995"),
    { (ucschar*)hangul_keyboard_table_3shin_1995, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        NULL, 
        (HangulCombination*)&hangul_galmadeuli_3shin_1995, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_SHIN,
    true,
    0x0000,
    {true, false, false, true, false},
    {(char*)&sebeol_3shin_moeum_key, (char*)&sebeol_3shin_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3shin_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_shin, NULL}
};

static const HangulKeyboard hangul_keyboard_3shin_2003 = {
    (char*)"3shin-2003",
    (char*)N_("Sebeolsik Shin 2003"),
    { (ucschar*)hangul_keyboard_table_3shin_2003, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        NULL, 
        (HangulCombination*)&hangul_galmadeuli_3shin_2003, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_SHIN,
    true,
    0x0000,
    {true, false, false, true, false},
    {(char*)&sebeol_3shin_moeum_key, (char*)&sebeol_3shin_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3shin_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_shin, NULL}
};

static const HangulKeyboard hangul_keyboard_3shin_2012 = {
    (char*)"3shin-2012",
    (char*)N_("Sebeolsik Shin 2012"),
    { (ucschar*)hangul_keyboard_table_3shin_2012, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        NULL, 
        (HangulCombination*)&hangul_galmadeuli_3shin_2012, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_SHIN,
    true,
    0x0000,
    {true, false, false, true, false},
    {(char*)&sebeol_3shin_moeum_key, (char*)&sebeol_3shin_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3shin_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_shin, NULL}
};

static const HangulKeyboard hangul_keyboard_3shin_2015 = {
    (char*)"3shin-2015",
    (char*)N_("Sebeolsik Shin 2015"),
    { (ucschar*)hangul_keyboard_table_3shin_2015, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        (HangulCombination*)&hangul_combination_3shin_2015, 
        (HangulCombination*)&hangul_galmadeuli_3shin_2015, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_SHIN_SHIFT,
    true,
    0x0000,
    {true, false, false, true, false},
    {(char*)&sebeol_3shin_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3shin_m = {
    (char*)"3shin-m",
    (char*)N_("Sebeolsik Shin M"),
    { (ucschar*)hangul_keyboard_table_3shin_m, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        NULL, 
        (HangulCombination*)&hangul_galmadeuli_3shin_m, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_SHIN_SHIFT,
    true,
    0x0000,
    {true, false, false, true, false},
    {(char*)&sebeol_3shin_moeum_key, NULL, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, NULL, NULL, NULL},
    {NULL, NULL}
};

static const HangulKeyboard hangul_keyboard_3shin_p = {
    (char*)"3shin-p",
    (char*)N_("Sebeolsik Shin P"),
    { (ucschar*)hangul_keyboard_table_3shin_p, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_default_3, 
        NULL, 
        (HangulCombination*)&hangul_galmadeuli_3shin_p, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_SHIN,
    true,
    0x0000,
    {true, false, false, true, true},
    {(char*)&sebeol_3shin_moeum_key, (char*)&sebeol_3shin_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3shin_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_shin, NULL}
};

static const HangulKeyboard hangul_keyboard_3shin_p_yet = {
    (char*)"3shin-p-yet",
    (char*)N_("Sebeolsik Shin P Yetgeul"),
    { (ucschar*)hangul_keyboard_table_3shin_p, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_full, 
        (HangulCombination*)&hangul_combination_3shin_p_yet, 
        (HangulCombination*)&hangul_galmadeuli_3shin_p, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_SHIN_YET,
    true,
    0x0000,
    {true, false, false, true, true},
    {NULL, (char*)&sebeol_3shin_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3shin_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_shin, NULL}
};

static const HangulKeyboard hangul_keyboard_3shin_p2 = {
    (char*)"3shin-p2",
    (char*)N_("Sebeolsik Shin P2"),
    { (ucschar*)hangul_keyboard_table_3shin_p2, NULL, NULL, NULL },
    { (HangulCombination*)&hangul_combination_default_3, //기본조합
      NULL,//추가조합
      (HangulCombination*)&hangul_galmadeuli_3shin_p2, //갈마들이조합
      NULL
    },
    HANGUL_KEYBOARD_TYPE_JASO_SHIN,
    true ,
  // replace_it // FALSE
  0x0000,
  // 확장배열씀, 갈마들이켜끄기됨, 입력순서〈안〉따짐, 왼/오른ㅗㅜ구분함, 확장겹받침허용〈안〉함
  // flag // 갈마들이는 필수 기능이라 꺼지면 안된다
  {true, false, false, true, true},
  //moeum_key, symbol_key, yetgeul_key
  { NULL, (char*)&sebeol_3shin_symbol_key, NULL, NULL },
  //moeum_value, symbol_value, yetgeul_value, ext_step_value
  { (ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3shin_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step },
  //(*symbolFunc)(int, int, int), (*yetgeulFunc)(int, int, int)
  { (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_shin, NULL }
};

static const HangulKeyboard hangul_keyboard_3shin_p2_yet = {
    (char*)"3shin-p2-yet",
    (char*)N_("Sebeolsik Shin P2 Yetgeul"),
    { (ucschar*)hangul_keyboard_table_3shin_p2, NULL, NULL, NULL },
    { 
        (HangulCombination*)&hangul_combination_full, 
        (HangulCombination*)&hangul_combination_3shin_p2_yet, 
        (HangulCombination*)&hangul_galmadeuli_3shin_p2, NULL },
    HANGUL_KEYBOARD_TYPE_JASO_SHIN_YET,
    true,
    0x0000,
    {true, false, false, true, false},
    {(char*)&sebeol_3shin_moeum_key, (char*)&sebeol_3shin_symbol_key, NULL, NULL},
    {(ucschar*)&sebeol_3_moeum_value, (ucschar*)&sebeol_3shin_symbol_value, NULL, (ucschar*)&sebeol_3_ext_step},
    {
        (ucschar(*)(int, int, int))&hangul_ascii_to_symbol_shin, NULL}
};


static const HangulKeyboard* hangul_builtin_keyboards[] = {
    // 3beol
    &hangul_keyboard_2,
    &hangul_keyboard_2noshift,
    &hangul_keyboard_2north9256,
    &hangul_keyboard_3_90,
    &hangul_keyboard_3_91_final,
    &hangul_keyboard_3_p3,
    &hangul_keyboard_3moa_semoe_2018,
    &hangul_keyboard_3sun_2014,
    &hangul_keyboard_3shin_p2,
    &hangul_keyboard_2y,
    &hangul_keyboard_32,
    &hangul_keyboard_romaja,
    &hangul_keyboard_ahn,
    &hangul_keyboard_3sun_1990,
    &hangul_keyboard_3_89,
    &hangul_keyboard_3_91_final_noshift,
    &hangul_keyboard_3_93_yet,
    &hangul_keyboard_3_95,
    &hangul_keyboard_3ahnmatae,
    &hangul_keyboard_3_2011,
    &hangul_keyboard_3_2011_yet,
    &hangul_keyboard_3_2012,
    &hangul_keyboard_3_2012_yet,
    &hangul_keyboard_3_2014,
    &hangul_keyboard_3_2014_yet,
    &hangul_keyboard_3_2015,
    &hangul_keyboard_3_2015_yet,
    &hangul_keyboard_3_2015_metal,
    &hangul_keyboard_3_2015_patal,
    &hangul_keyboard_3_2015_patal_yet,
    &hangul_keyboard_3_p2,
    &hangul_keyboard_3_14_proposal,
    &hangul_keyboard_3moa_semoe_2014,
    &hangul_keyboard_3moa_semoe_2015,
    &hangul_keyboard_3moa_semoe_2016,
    &hangul_keyboard_3moa_semoe_2017,
    &hangul_keyboard_3gimguk_38a_yet,
    &hangul_keyboard_3shin_1995,
    &hangul_keyboard_3shin_2003,
    &hangul_keyboard_3shin_2012,
    &hangul_keyboard_3shin_2015,
    &hangul_keyboard_3shin_m,
    &hangul_keyboard_3shin_p,
    &hangul_keyboard_3shin_p_yet,
    &hangul_keyboard_3shin_p2_yet
};
static unsigned int hangul_builtin_keyboard_count = countof(hangul_builtin_keyboards);

static HangulKeyboardList hangul_keyboards = { 0, 0, NULL };

typedef struct _HangulKeyboardLoadContext {
    const char* path_stack[64];
    int path_stack_top;
    HangulKeyboard* keyboard;
    int current_id;
    const char* current_element;
    bool save_name;
} HangulKeyboardLoadContext;

static void    hangul_keyboard_parse_file(const char* path, HangulKeyboardLoadContext* context);
static bool    hangul_keyboard_list_append(HangulKeyboard* keyboard);

HangulCombination*
hangul_combination_new()
{
    HangulCombination *combination = malloc(sizeof(HangulCombination));
    if (combination != NULL) {
	combination->size = 0;
	combination->size_alloced = 0;
	combination->table = NULL;
	combination->is_static = false;
	return combination;
    }

    return NULL;
}

void
hangul_combination_delete(HangulCombination *combination)
{
    if (combination == NULL)
	return;

    if (combination->is_static)
	return;

    if (combination->table != NULL)
	free(combination->table);

    free(combination);
}

static uint32_t
hangul_combination_make_key(ucschar first, ucschar second)
{
    return first << 16 | second;
}

bool
hangul_combination_set_data(HangulCombination* combination,
			    ucschar* first, ucschar* second, ucschar* result,
			    unsigned int n)
{
    if (combination == NULL)
	return false;

    if (n == 0 || n > SIZE_MAX / sizeof(HangulCombinationItem))
	return false;

    combination->table = malloc(sizeof(HangulCombinationItem) * n);
    if (combination->table != NULL) {
	int i;

	combination->size = n;
	for (i = 0; i < n; i++) {
	    combination->table[i].key = hangul_combination_make_key(first[i], second[i]);
	    combination->table[i].code = result[i];
	}
	return true;
    }

    return false;
}

static bool
hangul_combination_add_item(HangulCombination* combination,
	ucschar first, ucschar second, ucschar result)
{
    if (combination == NULL)
	return false;

    if (combination->is_static)
	return false;

    if (combination->size >= combination->size_alloced) {
	size_t size_need = combination->size_alloced * 2;
	if (size_need == 0) {
	    // 처음 할당할 때에는 64개를 기본값으로 한다.
	    size_need = 64;
	}

	HangulCombinationItem* table = combination->table;
	table = realloc(table, size_need * sizeof(table[0]));
	if (table == NULL)
	    return false;

	combination->size_alloced = size_need;
	combination->table = table;
    }

    uint32_t key = hangul_combination_make_key(first, second);
    size_t i = combination->size;
    combination->table[i].key = key;
    combination->table[i].code = result;
    combination->size = i + 1;
    return true;
}

static int
hangul_combination_cmp(const void* p1, const void* p2)
{
    const HangulCombinationItem *item1 = p1;
    const HangulCombinationItem *item2 = p2;

    /* key는 unsigned int이므로 단순히 빼서 리턴하면 안된다.
     * 두 수의 차가 큰 경우 int로 변환하면서 음수가 될 수 있다. */
    if (item1->key < item2->key)
	return -1;
    else if (item1->key > item2->key)
	return 1;
    else
	return 0;
}

static void
hangul_combination_sort(HangulCombination* combination)
{
    if (combination == NULL)
	return;

    if (combination->is_static)
	return;

    qsort(combination->table, combination->size,
	sizeof(combination->table[0]), hangul_combination_cmp);
}

static ucschar
hangul_combination_combine(const HangulCombination* combination,
			   ucschar first, ucschar second)
{
    HangulCombinationItem *res;
    HangulCombinationItem key;

    if (combination == NULL)
	return 0;

    key.key = hangul_combination_make_key(first, second);
    res = bsearch(&key, combination->table, combination->size,
	          sizeof(combination->table[0]), hangul_combination_cmp);
    if (res != NULL)
	return res->code;

    return 0;
}

HangulKeyboard*
hangul_keyboard_new()
{
    HangulKeyboard *keyboard = malloc(sizeof(HangulKeyboard));
    if (keyboard == NULL)
	return NULL;

    keyboard->id = NULL;
    keyboard->name = NULL;

    keyboard->table[0] = NULL;
    keyboard->table[1] = NULL;
    keyboard->table[2] = NULL;
    keyboard->table[3] = NULL;

    keyboard->combination[0] = NULL;
    keyboard->combination[1] = NULL;
    keyboard->combination[2] = NULL;
    keyboard->combination[3] = NULL;

    keyboard->type = HANGUL_KEYBOARD_TYPE_JAMO;
    keyboard->is_static = false;

    // 3beol
    // 바꿔 놓기 : 세벌식의 ] -> 아래아
    keyboard->replace_it = 0x0000; 
    // 확장배열씀, 갈마들이켜끄기됨, 입력순서〈안〉따짐, 왼/오른ㅗㅜ구분함, 확장겹받침허용〈안〉함
    bool flag[5] = {false, false, false, false, false}; //
    // [모음글쇠, 확장기호글쇠, 확장한글글쇠, ]
    char* addon_key[4] = {NULL, NULL, NULL, NULL};
    // [모음값, 확장기호값, 확장한글값, 확장단계기호값]
    ucschar* addon_value[4] = {NULL, NULL, NULL, NULL};
    // [기호확장함수, 한글확장함수]
    ucschar (*addon_func[2])(int, int, int) = {NULL, NULL};

    return keyboard;
}

static void
hangul_keyboard_set_id(HangulKeyboard* keyboard, const char* id)
{
    if (keyboard == NULL)
	return;

    if (keyboard->is_static)
	return;

    free(keyboard->id);
    keyboard->id = strdup(id);
}

static void
hangul_keyboard_set_name(HangulKeyboard* keyboard, const char* name)
{
    if (keyboard == NULL)
	return;

    if (keyboard->is_static)
	return;

    free(keyboard->name);
    keyboard->name = strdup(name);
}

ucschar
hangul_keyboard_get_mapping(const HangulKeyboard* keyboard, int tableid, unsigned key)
{
    if (keyboard == NULL)
	return 0;

    if (tableid >= countof(keyboard->table))
	return 0;

    if (key >= HANGUL_KEYBOARD_TABLE_SIZE)
	return 0;

    ucschar* table = keyboard->table[tableid];
    if (table == NULL)
	return 0;

    return table[key];
}

static void
hangul_keyboard_set_mapping(HangulKeyboard *keyboard, int tableid, unsigned key, ucschar value)
{
    if (keyboard == NULL)
	return;

    if (tableid >= countof(keyboard->table))
	return;

    if (key >= HANGUL_KEYBOARD_TABLE_SIZE)
	return;

    if (keyboard->table[tableid] == NULL) {
	ucschar* new_table = malloc(sizeof(ucschar) * HANGUL_KEYBOARD_TABLE_SIZE);
	if (new_table == NULL)
	    return;

	unsigned i;
	for (i = 0; i < HANGUL_KEYBOARD_TABLE_SIZE; ++i) {
	    new_table[i] = 0;
	}
	keyboard->table[tableid] = new_table;
    }

    ucschar* table = keyboard->table[tableid];
    table[key] = value;
}

void
hangul_keyboard_set_value(HangulKeyboard *keyboard, int key, ucschar value)
{
    hangul_keyboard_set_mapping(keyboard, 0, key, value);
}

int
hangul_keyboard_get_type(const HangulKeyboard *keyboard)
{
    int type = 0;
    if (keyboard != NULL) {
	type = keyboard->type;
    }
    return type;
}

void
hangul_keyboard_set_type(HangulKeyboard *keyboard, int type)
{
    if (keyboard != NULL) {
	keyboard->type = type;
    }
}

void
hangul_keyboard_delete(HangulKeyboard *keyboard)
{
    if (keyboard == NULL)
	return;

    if (keyboard->is_static)
	return;

    free(keyboard->id);
    free(keyboard->name);

    unsigned i;
    for (i = 0; i < countof(keyboard->table); ++i) {
	if (keyboard->table[i] != NULL) {
	    free(keyboard->table[i]);
	}
    }

    for (i = 0; i < countof(keyboard->combination); ++i) {
	if (keyboard->combination[i] != NULL) {
	    hangul_combination_delete(keyboard->combination[i]);
	}
    }

    free(keyboard);
}

ucschar
hangul_keyboard_combine(const HangulKeyboard* keyboard,
	unsigned int id, ucschar first, ucschar second)
{
    if (keyboard == NULL)
	return 0;

    if (id >= countof(keyboard->combination))
	return 0;

    HangulCombination* combination = keyboard->combination[id];
    
    if (hangul_keyboard_get_flag(keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER))
    {
        // 입력순서를 따지지 않을 때, 작은 값을 앞에 둔다.
        ucschar temp = first;
        if (first > second)
        {
            first = second;
            second = temp;
        }
    }
    
    ucschar res = hangul_combination_combine(combination, first, second);
    return res;
}

#if ENABLE_EXTERNAL_KEYBOARDS
static const char*
attr_lookup(const char** attr, const char* name)
{
    if (attr == NULL)
	return NULL;

    int i;
    for (i = 0; attr[i] != NULL; i += 2) {
	if (strcmp(attr[i], name) == 0) {
	    return attr[i + 1];
	}
    }

    return NULL;
}

static unsigned int
attr_lookup_as_uint(const char** attr, const char* name)
{
    const char* valuestr = attr_lookup(attr, name);
    if (valuestr == NULL)
	return 0;

    unsigned int value = strtoul(valuestr, NULL, 0);
    return value;
}

static void XMLCALL
on_element_start(void* data, const XML_Char* element, const XML_Char** attr)
{
    HangulKeyboardLoadContext* context = (HangulKeyboardLoadContext*)data;

    if (strcmp(element, "hangul-keyboard") == 0) {
	if (context->keyboard != NULL) {
	    hangul_keyboard_delete(context->keyboard);
	}
	context->keyboard = hangul_keyboard_new();

	const char* id = attr_lookup(attr, "id");
	hangul_keyboard_set_id(context->keyboard, id);

	const char* typestr = attr_lookup(attr, "type");
	int type = HANGUL_KEYBOARD_TYPE_JAMO;
	if (strcmp(typestr, "jamo") == 0) {
	    type = HANGUL_KEYBOARD_TYPE_JAMO;
	} else if (strcmp(typestr, "jamo-yet") == 0) {
	    type = HANGUL_KEYBOARD_TYPE_JAMO_YET;
	} else if (strcmp(typestr, "jaso") == 0) {
	    type = HANGUL_KEYBOARD_TYPE_JASO;
	} else if (strcmp(typestr, "jaso-yet") == 0) {
	    type = HANGUL_KEYBOARD_TYPE_JASO_YET;
	} else if (strcmp(typestr, "romaja") == 0) {
	    type = HANGUL_KEYBOARD_TYPE_ROMAJA;
	} else if (strcmp(typestr, "shin") == 0) {
	    type = HANGUL_KEYBOARD_TYPE_JASO_SHIN;
	}

	hangul_keyboard_set_type(context->keyboard, type);
    } else if (strcmp(element, "name") == 0) {
	if (context->keyboard == NULL)
	    return;

	const char* lang = attr_lookup(attr, "xml:lang");
	if (lang == NULL) {
	    context->save_name = true;
	} else {
	    const char* locale = setlocale(LC_ALL, NULL);
	    size_t n = strlen(lang);
	    if (strncmp(lang, locale, n) == 0) {
		context->save_name = true;
	    }
	}
	context->current_element = "name";
    } else if (strcmp(element, "map") == 0) {
	if (context->keyboard == NULL)
	    return;

	unsigned int id = attr_lookup_as_uint(attr, "id");
	if (id < countof(context->keyboard->table)) {
	    context->current_id = id;
	    context->current_element = "map";
	}
    } else if (strcmp(element, "combination") == 0) {
	if (context->keyboard == NULL)
	    return;

	unsigned int id = attr_lookup_as_uint(attr, "id");
	if (id < countof(context->keyboard->combination)) {
	    if (context->keyboard->combination[id] != NULL) {
		hangul_combination_delete(context->keyboard->combination[id]);
	    }

	    context->current_id = id;
	    context->current_element = "combination";
	    context->keyboard->combination[id] = hangul_combination_new();
	}
    } else if (strcmp(element, "item") == 0) {
	if (context->keyboard == NULL)
	    return;

	unsigned int id = context->current_id;
	if (strcmp(context->current_element, "map") == 0) {
	    HangulKeyboard* keyboard = context->keyboard;
	    unsigned int key = attr_lookup_as_uint(attr, "key");
	    unsigned int value = attr_lookup_as_uint(attr, "value");
	    hangul_keyboard_set_mapping(keyboard, id, key, value);
	} else if (strcmp(context->current_element, "combination") == 0) {
	    HangulCombination* combination = context->keyboard->combination[id];
	    unsigned int first = attr_lookup_as_uint(attr, "first");
	    unsigned int second = attr_lookup_as_uint(attr, "second");
	    unsigned int result = attr_lookup_as_uint(attr, "result");
	    hangul_combination_add_item(combination, first, second, result);
	}
    } else if (strcmp(element, "include") == 0) {
	const char* file = attr_lookup(attr, "file");
	if (file == NULL)
	    return;

        int top = context->path_stack_top;
        if (top < 0)
            return;

        size_t n = strlen(file) + strlen(context->path_stack[top]) + 1;
	char* path = malloc(n);
	if (path == NULL)
	    return;

	if (file[0] == '/') {
	    strncpy(path, file, n);
	} else {
	    char* orig_path = strdup(context->path_stack[top]);
	    char* last_slash = strrchr(orig_path, '/');
	    if (last_slash)
		last_slash[0] = '\0';

	    snprintf(path, n, "%s/%s", orig_path, file);
	    free(orig_path);
	}

	hangul_keyboard_parse_file(path, context);
	free(path);
    }
}

static void XMLCALL
on_element_end(void* data, const XML_Char* element)
{
    HangulKeyboardLoadContext* context = (HangulKeyboardLoadContext*)data;

    if (context->keyboard == NULL)
	return;

    if (strcmp(element, "name") == 0) {
	context->current_element = "";
	context->save_name = false;
    } else if (strcmp(element, "map") == 0) {
	context->current_id = 0;
	context->current_element = "";
    } else if (strcmp(element, "combination") == 0) {
	unsigned int id = context->current_id;
	HangulCombination* combination = context->keyboard->combination[id];
	hangul_combination_sort(combination);
	context->current_id = 0;
	context->current_element = "";
    }
}

static void XMLCALL
on_char_data(void* data, const XML_Char* s, int len)
{
    HangulKeyboardLoadContext* context = (HangulKeyboardLoadContext*)data;

    if (context->keyboard == NULL)
	return;

    if (context->current_element != NULL && strcmp(context->current_element, "name") == 0) {
	if (context->save_name) {
	    char buf[1024];
	    if (len >= sizeof(buf))
		len = sizeof(buf) - 1;
	    memcpy(buf, s, len);
	    buf[len] = '\0';
	    hangul_keyboard_set_name(context->keyboard, buf);
	}
    }
}

static void
hangul_keyboard_parse_file(const char* path, HangulKeyboardLoadContext* context)
{
    int top = context->path_stack_top + 1;
    if (top >= sizeof(context->path_stack) / sizeof(context->path_stack[0])) {
        return;
    }

    context->path_stack[top] = path;
    context->path_stack_top = top;

    XML_Parser parser = XML_ParserCreate(NULL);

    XML_SetUserData(parser, context);
    XML_SetElementHandler(parser, on_element_start, on_element_end);
    XML_SetCharacterDataHandler(parser, on_char_data);

    FILE* file = fopen(path, "r");
    if (file == NULL) {
        goto done;
    }

    char buf[8192];

    while (true) {
	size_t n = fread(buf, 1, sizeof(buf), file);
	int is_final = feof(file);
	int res = XML_Parse(parser, buf, n, is_final);
	if (res == XML_STATUS_ERROR) {
	    goto close;
	}
	if (is_final)
	    break;
    }

close:
    fclose(file);
done:
    XML_ParserFree(parser);

    context->path_stack_top--;
}

HangulKeyboard*
hangul_keyboard_new_from_file(const char* path)
{
    HangulKeyboardLoadContext context;
    memset(&context, 0, sizeof(context));
    context.path_stack_top = -1;

    hangul_keyboard_parse_file(path, &context);

    return context.keyboard;
}

static unsigned
hangul_keyboard_list_load_dir(const char* path)
{
    if (path == NULL) {
        return 0;
    }

    const char* subpattern = "/*.xml";
    size_t len = strlen(path) + strlen(subpattern) + 1;
    char* pattern = (char*)malloc(len);
    if (pattern == NULL) {
        return 0;
    }

    snprintf(pattern, len, "%s%s", path, subpattern);

#ifdef HAVE_GLOB_H
    glob_t result;
    int res = glob(pattern, GLOB_ERR, NULL, &result);
    if (res != 0) {
        free(pattern);
	return 0;
    }

    size_t i;
    for (i = 0; i < result.gl_pathc; ++i) {
	HangulKeyboard* keyboard = hangul_keyboard_new_from_file(result.gl_pathv[i]);
	if (keyboard == NULL)
	    continue;
	hangul_keyboard_list_append(keyboard);
    }

    globfree(&result);
    free(pattern);
#else /* _WIN32 */
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind;
    int n = (strlen(pattern) + 1) * sizeof(WCHAR);

    LPWSTR wpattern = (LPWSTR)malloc(n);
    if (wpattern == NULL) {
	free(pattern);
	return 0;
    }

    MultiByteToWideChar(CP_ACP, 0, pattern, -1, wpattern, n);

    hFind = FindFirstFileW(wpattern, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
	free(wpattern);
	free(pattern);
	return 0;
    }

    do {
	n = WideCharToMultiByte(CP_ACP, 0, findFileData.cFileName, -1, NULL, 0, NULL, NULL);
	if (n == 0)
	    continue;

	int path_len = strlen(path);
	len = path_len + n + 2;
	char* file = (char*)malloc(len);
	if (file == NULL)
	    continue;

	memcpy(file, path, path_len);
	file[path_len] = '/';
	char* pfile = &file[path_len + 1];
	WideCharToMultiByte(CP_ACP, 0, findFileData.cFileName, -1, pfile, n, NULL, NULL);

	HangulKeyboard* keyboard = hangul_keyboard_new_from_file(file);
	free(file);

	if (keyboard == NULL)
	    continue;
	hangul_keyboard_list_append(keyboard);
    } while(FindNextFileW(hFind, &findFileData));

    FindClose(hFind);
    free(wpattern);
    free(pattern);
#endif /* HAVE_GLOB_H */

    return hangul_keyboards.n;
}
#endif /* ENABLE_EXTERNAL_KEYBOARDS */

static void
hangul_keyboard_list_clear()
{
    size_t i;
    for (i = 0; i < hangul_keyboards.n; ++i) {
	hangul_keyboard_delete(hangul_keyboards.keyboards[i]);
    }

    free(hangul_keyboards.keyboards);

    hangul_keyboards.n = 0;
    hangul_keyboards.nalloced = 0;
    hangul_keyboards.keyboards = NULL;
}

#if ENABLE_EXTERNAL_KEYBOARDS
static char*
hangul_keyboard_get_default_keyboard_path()
{
    char* keyboard_path = NULL;
    size_t keyboard_path_len = 1;

    /* default LIBHANGUL_KEYBOARD_PATH is
     * SYSTEM_KEYBOARD_DIR:USER_KEYBOARD_DIR */

#ifdef _WIN32
    /* system default dir */
    char* system_dir = NULL;
    const char* data_dir = getenv("APPDATA");
    if (data_dir != NULL) {
        const char* subdir = "/libhangul/keyboards";
        size_t system_dir_len = strlen(data_dir) + strlen(subdir) + 1;
        system_dir = (char*)malloc(system_dir_len);
        if (system_dir != NULL) {
            snprintf(system_dir, system_dir_len, "%s%s", data_dir, subdir);
            keyboard_path_len += strlen(system_dir);
        }
    }
    /* user default dir */
    char* home_dir = getenv("USERPROFILE");
    if (home_dir == NULL) {
        /* no user data dir */
        return system_dir;
    } else {
        const char* subdir = "/.libhangul/keyboards";
        keyboard_path_len += strlen(home_dir) + strlen(subdir);
        keyboard_path = (char*)malloc(keyboard_path_len);
        if (keyboard_path != NULL) {
            if (system_dir != NULL) {
                keyboard_path_len++;
                snprintf(keyboard_path, keyboard_path_len, "%s;%s%s", system_dir, home_dir, subdir);
	    } else {
                snprintf(keyboard_path, keyboard_path_len, "%s%s", home_dir, subdir);
	    }
        }
    }
#else
    /* system default dir */
    const char* system_dir = LIBHANGUL_KEYBOARD_DIR;
    keyboard_path_len += strlen(system_dir);

    /* user default dir */
    char* xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home == NULL) {
        char* home_dir = getenv("HOME");
        if (home_dir == NULL) {
            /* no user data dir */
            keyboard_path = (char*)malloc(keyboard_path_len);
            if (keyboard_path != NULL) {
                snprintf(keyboard_path, keyboard_path_len, "%s", system_dir);
            }
        } else {
            const char* subdir = "/.local/share/libhangul/keyboards";
            keyboard_path_len += 1 + strlen(home_dir) + strlen(subdir);
            keyboard_path = (char*)malloc(keyboard_path_len);
            if (keyboard_path != NULL) {
                snprintf(keyboard_path, keyboard_path_len, "%s:%s%s", system_dir, home_dir, subdir);
            }
        }
    } else {
        const char* subdir = "/libhangul/keyboards";
        keyboard_path_len += 1 + strlen(xdg_data_home) + strlen(subdir);
        keyboard_path = (char*)malloc(keyboard_path_len);
        if (keyboard_path != NULL) {
            snprintf(keyboard_path, keyboard_path_len, "%s:%s%s", system_dir, xdg_data_home, subdir);
        }
    }
#endif /* _WIN32 */

    return keyboard_path;
}

static char*
hangul_keyboard_get_keyboard_path()
{
    char* keyboard_path = getenv("LIBHANGUL_KEYBOARD_PATH");
    if (keyboard_path == NULL) {
        keyboard_path = hangul_keyboard_get_default_keyboard_path();
    } else {
        keyboard_path = strdup(keyboard_path);
    }

    return keyboard_path;
}
#endif /* ENABLE_EXTERNAL_KEYBOARDS */

int
hangul_keyboard_list_init()
{
#if ENABLE_EXTERNAL_KEYBOARDS
    /* 이 함수를 중복 호출할 경우에 대한 처리
     * 이미 등록된 자판이 있다면 중복 호출된 것으로 보고
     * 함수를 종료한다. */
    if (hangul_keyboards.n > 0)
	return 2;

    /* hangul_init을 호출하면 builtin keyboard는 disable되도록 처리한다.
     * 기본 자판은 외부 파일로 부터 로딩하는 것이 기본 동작이고
     * builtin 키보드는 하위 호환을 위해 남겨둔다. */
    hangul_builtin_keyboard_count = 0;

    /* libhangul data dir에서 keyboard 로딩 */
    char* libhangul_keyboard_path = hangul_keyboard_get_keyboard_path();

    unsigned n = 0;

    char* dir = libhangul_keyboard_path;
#ifdef _WIN32
    char sep = ';';
#else
    char sep = ':';
#endif /* _WIN32 */
    while (dir != NULL && dir[0] != '\0') {
        char* next = strchr(dir, sep);
        if (next != NULL) {
            next[0] = '\0';
            ++next;
        }

        n += hangul_keyboard_list_load_dir(dir);
        dir = next;
    }

    free(libhangul_keyboard_path);

    if (n == 0)
	return 1;

#endif /* ENABLE_EXTERNAL_KEYBOARDS */
    return 0;
}

int
hangul_keyboard_list_fini()
{
    hangul_keyboard_list_clear();
    hangul_builtin_keyboard_count = countof(hangul_builtin_keyboards);
    return 0;
}

static char*
hangul_builtin_keyboard_list_get_keyboard_id(unsigned int index_)
{
    if (index_ >= hangul_builtin_keyboard_count)
	return NULL;

    const HangulKeyboard* keyboard = hangul_builtin_keyboards[index_];
    if (keyboard == NULL)
	return NULL;

    return keyboard->id;
}

static const char*
hangul_builtin_keyboard_list_get_keyboard_name(unsigned int index_)
{
#ifdef ENABLE_NLS
    static bool isGettextInitialized = false;
    if (!isGettextInitialized) {
	isGettextInitialized = true;
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    }
#endif

    if (index_ >= hangul_builtin_keyboard_count)
	return NULL;

    const HangulKeyboard* keyboard = hangul_builtin_keyboards[index_];
    if (keyboard == NULL)
	return NULL;

    return _(keyboard->name);
}

static const HangulKeyboard*
hangul_builtin_keyboard_list_get_keyboard(const char* id)
{
    size_t i;
    for (i = 0; i < hangul_builtin_keyboard_count; ++i) {
	const HangulKeyboard* keyboard = hangul_builtin_keyboards[i];
	if (strcmp(id, keyboard->id) == 0) {
	    return keyboard;
	}
    }
    return NULL;
}

/**
 * @ingroup hangulkeyboards
 * @brief libhangul에서 제공하는 자판 개수를 구하는 함수
 *
 * 이 함수의 리턴값을 이용해서 자판을 iteration할 수 있다.
 * 한글 자판의 설치 위치에 대한 정보는 @ref addinghangulkeyboards 를 참고하라.
 * @return @ref HangulInputContext 에서 선택 가능한 자판 개수
 */
unsigned int
hangul_keyboard_list_get_count()
{
    if (hangul_builtin_keyboard_count > 0)
	return hangul_builtin_keyboard_count;

    return hangul_keyboards.n;
}

/**
 * @ingroup hangulkeyboards
 * @brief libhangul에서 제공하는 자판의 id를 구하는 함수
 *
 * @a index_ 로 지정된 자판의 id 값을 리턴한다. 이 id는 @ref
 * hangul_ic_select_keyboard() 함수의 인자로 사용하는 문자열이다.
 * @return 지정된 자판의 id. 이 문자열은 libhangul 내부에서 관리하는 것으로
 *         free해서는 안된다.
 */
const char*
hangul_keyboard_list_get_keyboard_id(unsigned int index_)
{
    if (hangul_builtin_keyboard_count > 0) {
      char* id = hangul_builtin_keyboard_list_get_keyboard_id(index_);
      if (id != NULL) {
        return id;
      }
      //return hangul_builtin_keyboard_list_get_keyboard_id(index_);
    }

    if (index_ >= hangul_keyboards.n)
	return NULL;

    HangulKeyboard* keyboard = hangul_keyboards.keyboards[index_];
    if (keyboard == NULL)
	return NULL;

    return keyboard->id;
}

/**
 * @ingroup hangulkeyboards
 * @brief libhangul에서 제공하는 자판의 이름을 구하는 함수
 *
 * @a index_ 로 지정된 자판의 이름을 리턴한다. 이 문자열은 사용자에게 보여주기
 * 위한 것으로, 번역되어 사람이 읽을 수 있는 형태의 문자열이다.
 * @return 지정된 자판의 이름. 이 문자열은 libhangul 내부에서 관리하는 것으로
 *         free해서는 안된다.
 */
const char*
hangul_keyboard_list_get_keyboard_name(unsigned int index_)
{
    if (hangul_builtin_keyboard_count > 0) {
      const char* name = hangul_builtin_keyboard_list_get_keyboard_name(index_);
      if (name != NULL) {
        return name;
      }
	 //return hangul_builtin_keyboard_list_get_keyboard_name(index_);
    }

    if (index_ >= hangul_keyboards.n)
	return NULL;

    HangulKeyboard* keyboard = hangul_keyboards.keyboards[index_];
    if (keyboard == NULL)
	return NULL;

    return keyboard->name;
}

/**
 * @ingroup hangulkeyboards
 * @brief libhangul에서 제공하는 자판의 HangulKeyboard 포인터를 구하는 함수
 * @return id로 찾아진 자판의 HangulKeyboard 포인터, 못찾으면 NULL.
 *         이 스트럭처는 libhangul 내부에서 관리하는 것으로 free해서는 안된다.
 */
const HangulKeyboard*
hangul_keyboard_list_get_keyboard(const char* id)
{
    if (hangul_builtin_keyboard_count > 0) {
      const HangulKeyboard* keyboard = hangul_builtin_keyboard_list_get_keyboard(id);
      if (keyboard != NULL) {
        return keyboard;
      }
    //return hangul_builtin_keyboard_list_get_keyboard(id);
    }

    /* 키보드 목록에서 순차 검색을 하여 찾으므로 같은 id로 서로다른
     * 자판이 등록되어 있다고 하면 먼저 로딩된 자판만 인식된다. */
    size_t i;
    for (i = 0; i < hangul_keyboards.n; ++i) {
	HangulKeyboard* keyboard = hangul_keyboards.keyboards[i];
	if (strcmp(id, keyboard->id) == 0) {
	    return keyboard;
	}
    }
    return NULL;
}

static bool
hangul_keyboard_list_append(HangulKeyboard* keyboard)
{
    if (hangul_keyboards.n >= hangul_keyboards.nalloced) {
	size_t n = hangul_keyboards.nalloced * 2;
	if (n == 0) {
	    n = 16;
	}
	HangulKeyboard** keyboards = hangul_keyboards.keyboards;
	keyboards = realloc(keyboards, n * sizeof(keyboards[0]));
	if (keyboards == NULL)
	    return false;

	hangul_keyboards.nalloced = n;
	hangul_keyboards.keyboards = keyboards;
    }

    size_t i = hangul_keyboards.n;
    hangul_keyboards.keyboards[i] = keyboard;
    hangul_keyboards.n = i + 1;

    return true;
}

/**
 * @ingroup hangulkeyboards
 * @brief keyboard 를 글로벌 키보드 리스트에 등록함
 * @param keyboard 등록할 키보드
 * @return keyboard 의 id, 키보드를 선택하거나 unregister할때 사용하는 id다.
 *
 * 여기에 등록된 키보드는 hangul_ic_select()를 통해서 선택될 수 있게 된다.
 * 이후 @a keyboard 는 libhangul이 관리하므로 사용자가 임의로 삭제해서는 안된다.
 * hangul_fini() 함수 안에서 삭제될 것이다.
 */
const char*
hangul_keyboard_list_register_keyboard(HangulKeyboard* keyboard)
{
    if (keyboard == NULL)
        return NULL;

    bool res = hangul_keyboard_list_append(keyboard);
    if (!res) {
        return NULL;
    }

    return keyboard->id;
}

/**
 * @ingroup hangulkeyboards
 * @brief id로 지정된 키보드를 글로벌 키보드 리스트에 삭제함
 * @param id 삭제할 키보드 id
 * @return 리스트에서 삭제된 HangulKeyboard 의 포인터, 이 포인터는 더이상 libhangul에서
 *         관리하지 않으므로 사용자가 hangul_keyboard_delete() 함수로 삭제해야 한다.
 */
HangulKeyboard*
hangul_keyboard_list_unregister_keyboard(const char* id)
{
    HangulKeyboard* keyboard = NULL;

    size_t i;
    for (i = 0; i < hangul_keyboards.n; ++i) {
        keyboard = hangul_keyboards.keyboards[i];
        if (strcmp(id, keyboard->id) == 0) {
            break;
        }
    }

    if (keyboard == NULL) {
        return NULL;
    }

    for (++i; i < hangul_keyboards.n; ++i) {
        hangul_keyboards.keyboards[i - 1] = hangul_keyboards.keyboards[i];
    }

    hangul_keyboards.keyboards[i - 1] = NULL;
    hangul_keyboards.n--;

    return keyboard;
}




// 3beol
#ifndef libhangul_hangulkeyboard_addon_c
#define libhangul_hangulkeyboard_addon_c
ucschar
hangul_keyboard_get_replace_it(const HangulKeyboard* keyboard)
{
    return keyboard->replace_it;
}


char*
hangul_keyboard_get_addon_key(const HangulKeyboard* keyboard, unsigned int index) 
{
    return keyboard->addon_key[index];
}

ucschar*
hangul_keyboard_get_addon_value(const HangulKeyboard* keyboard, unsigned int index)
{
    return keyboard->addon_value[index];
}

//ucschar (*addon_func[2])(int, int, int);
ucschar
(*hangul_keyboard_get_addon_func(const HangulKeyboard* keyboard, unsigned int index))(int, int, int)
{
    return keyboard->addon_func[index];
}


bool
hangul_keyboard_is_right_oua(const HangulKeyboard *keyboard, int ascii, ucschar ch, int index)
{
  if (keyboard == NULL) {
    return false;
  }
  if (keyboard->addon_key[index] == NULL) {
    return false;
  }

  int i;
  for (i = 0; *(keyboard->addon_key[index] + i) != 0x00; i++) {
    if (ascii == *(keyboard->addon_key[index] + i)) {
      for (i = 0; *(keyboard->addon_value[index] + i) != 0x0000; i++) {
        if (ch == *(keyboard->addon_value[index] + i)) {
          return true;
        }
      }
    }
  }
  return false;
}


int
hangul_keyboard_is_extension_key(const HangulKeyboard *keyboard, int ascii, int index)
{
  if (keyboard == NULL) 
  {
    return 0;
  }
  if (keyboard->addon_key[index] == NULL) 
  {
    return 0;
  }


  int i;
  for (i = 1; *(keyboard->addon_key[index] + i) != 0x00; i++) 
  {
    if (ascii == *(keyboard->addon_key[index] + i)) 
    {
      if (*(keyboard->addon_key[index] + 0) == '1') 
      {
        i += 10;
      }
      else if (*(keyboard->addon_key[index] + 0) == '2') 
      {
        i += 20;
      }
      else if (*(keyboard->addon_key[index] + 0) == '6') 
      {
        // 옛글 배열.
        return i;
      }
      else  
      {
      }
      return i;
    }
  }

  return 0;
}


bool
hangul_keyboard_get_flag(const HangulKeyboard *keyboard, unsigned int option)
{
    if (keyboard == NULL) 
    {
        return false;
    }

    switch(option)
    {
    case HANGUL_KEYBOARD_FLAG_EXTENDED:
    case HANGUL_KEYBOARD_FLAG_GALMADEULI:
    case HANGUL_KEYBOARD_FLAG_LOOSE_ORDER:
    case HANGUL_KEYBOARD_FLAG_RIGHT_OU:
    case HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT:
        return keyboard->flag[option];
    }

    return false;
}

unsigned int 
libhangul_get_init_keyboard_ids_length ()
{
    return INIT_IDS_LENGTH - 1;
}

char** 
libhangul_get_init_keyboard_ids ()
{
    return (char **)keys;
}



#endif /* libhangul_hangulkeyboard_addon_c */

