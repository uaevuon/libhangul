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

#ifndef libhangul_hangulinternals_h
#define libhangul_hangulinternals_h

#define N_ELEMENTS(array) (sizeof (array) / sizeof ((array)[0]))
#ifndef countof
#define countof(array) (sizeof (array) / sizeof ((array)[0]))
#endif

ucschar hangul_jongseong_get_diff(ucschar prevjong, ucschar jong);

ucschar hangul_choseong_to_jongseong(ucschar ch);
ucschar hangul_jongseong_to_choseong(ucschar ch);
void    hangul_jongseong_decompose(ucschar ch, ucschar* jong, ucschar* cho);

int     hangul_keyboard_get_type(const HangulKeyboard *keyboard);
ucschar hangul_keyboard_combine(const HangulKeyboard* keyboard,
	    unsigned int id, ucschar first, ucschar second);
ucschar hangul_keyboard_get_mapping(const HangulKeyboard* keyboard,
	    int tableid, unsigned key);

int hangul_keyboard_list_init();
int hangul_keyboard_list_fini();

const HangulKeyboard* hangul_keyboard_list_get_keyboard(const char* id);


// 3beol

// 3beol ctype
// 확장 기호
ucschar hangul_ascii_to_symbol_shin(int ascii, int step, int dummy);
ucschar hangul_ascii_to_symbol_3_p3(int ascii, int step, int press);
ucschar hangul_ascii_to_symbol_semoe(int ascii, int step, int dummy);
ucschar hangul_ascii_to_symbol_3_2011(int ascii, int step, int dummy);
ucschar hangul_ascii_to_symbol_3_2012(int ascii, int step, int dummy);
ucschar hangul_ascii_to_symbol_3_2011_yet(int ascii, int step, int press);
ucschar hangul_ascii_to_symbol_3_2012_yet(int ascii, int step, int press);
// 확장 옛한글
ucschar hangul_ascii_to_hanguel_3_yet(int ascii, int step, int press);



// 3beol inputcontext
static bool
hangul_ic_process_jaso_shin_sebeol (HangulInputContext *hic, int ascii, ucschar ch);
static bool
hangul_ic_process_jamo_dubeol(HangulInputContext *hic, ucschar ch);
static bool
hangul_ic_process_3finalsun (HangulInputContext *hic, int ascii, ucschar ch);
static bool
hangul_ic_process_jaso_sebeol (HangulInputContext *hic, int ascii, ucschar ch);
static bool
hangul_ic_process_jaso_shin_sebeol_yet (HangulInputContext *hic, int ascii, ucschar ch, bool capslock);
static bool
hangul_ic_process_jaso_shin_sebeol_shift (HangulInputContext *hic, int ascii, ucschar ch);

// 3beol keyboard
ucschar
hangul_keyboard_get_replace_it(const HangulKeyboard* keyboard);
char*
hangul_keyboard_get_addon_key(const HangulKeyboard* keyboard, unsigned int index);
ucschar*
hangul_keyboard_get_addon_value(const HangulKeyboard* keyboard, unsigned int index);
ucschar
(*hangul_keyboard_get_addon_func(const HangulKeyboard* keyboard, unsigned int index))(int, int, int);

bool
hangul_keyboard_get_flag(const HangulKeyboard *keyboard, unsigned int option);

bool
hangul_keyboard_is_right_oua(const HangulKeyboard *keyboard, int ascii, ucschar ch, int index);
int
hangul_keyboard_is_extension_key(const HangulKeyboard *keyboard, int ascii, int index);
bool
hangul_keyboard_is_extension_condition_sebeol_shin(HangulInputContext *hic);

void
hangul_keyboard_is_extension_ready_sebeol(HangulInputContext *hic);



#endif /* libhangul_hangulinternals_h */
