/* libhangul
 * Copyright (C) 2004 - 2007 Choe Hwanjin
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef libhangul_hangul_h
#define libhangul_hangul_h

#include <stdbool.h>
#include <inttypes.h>

#ifdef __GNUC__
#define LIBHANGUL_DEPRECATED __attribute__((deprecated))
#else
#define LIBHANGUL_DEPRECATED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* hangulctype.c */
enum {
    HANGUL_CHOSEONG_FILLER  = 0x115f,   /* hangul choseong filler */
    HANGUL_JUNGSEONG_FILLER = 0x1160    /* hangul jungseong filler */
};

typedef uint32_t ucschar;

bool hangul_is_choseong(ucschar c);
bool hangul_is_jungseong(ucschar c);
bool hangul_is_jongseong(ucschar c);
bool hangul_is_choseong_conjoinable(ucschar c);
bool hangul_is_jungseong_conjoinable(ucschar c);
bool hangul_is_jongseong_conjoinable(ucschar c);
bool hangul_is_jamo_conjoinable(ucschar c);
bool hangul_is_syllable(ucschar c);
bool hangul_is_jamo(ucschar c);
bool hangul_is_cjamo(ucschar c);

ucschar hangul_jamo_to_cjamo(ucschar ch);

const ucschar* hangul_syllable_iterator_prev(const ucschar* str,
					     const ucschar* begin);
const ucschar* hangul_syllable_iterator_next(const ucschar* str,
					     const ucschar* end);

int     hangul_syllable_len(const ucschar* str, int max_len);

ucschar hangul_jamo_to_syllable(ucschar choseong,
				ucschar jungseong,
				ucschar jongseong);
void    hangul_syllable_to_jamo(ucschar syllable,
				ucschar* choseong,
				ucschar* jungseong,
				ucschar* jongseong);
int     hangul_jamos_to_syllables(ucschar* dest, int destlen,
				  const ucschar* src, int srclen);


// 3beol
#ifndef libhangul_3beol
    #define libhangul_3beol
#endif

/* hangulinputcontext.c */
typedef struct _HangulKeyboard        HangulKeyboard;
typedef struct _HangulCombination     HangulCombination;
typedef struct _HangulBuffer          HangulBuffer;
typedef struct _HangulInputContext    HangulInputContext;

enum {
    HANGUL_OUTPUT_SYLLABLE,
    HANGUL_OUTPUT_JAMO
};

enum {
    HANGUL_KEYBOARD_TYPE_JAMO,
    HANGUL_KEYBOARD_TYPE_JASO,
    // 3beol
    HANGUL_KEYBOARD_TYPE_3FINALSUN,
	HANGUL_KEYBOARD_TYPE_JASO_SHIN,
    HANGUL_KEYBOARD_TYPE_JASO_SHIN_SHIFT,
    HANGUL_KEYBOARD_TYPE_JASO_SHIN_YET,
    HANGUL_KEYBOARD_TYPE_ROMAJA,
    HANGUL_KEYBOARD_TYPE_JAMO_YET,
    HANGUL_KEYBOARD_TYPE_JASO_YET,
};

enum {
    HANGUL_IC_OPTION_AUTO_REORDER,
    HANGUL_IC_OPTION_COMBI_ON_DOUBLE_STROKE,
    HANGUL_IC_OPTION_NON_CHOSEONG_COMBI,
    // 3beol
	HANGUL_IC_OPTION_EXTENDED_LAYOUT,
	HANGUL_IC_OPTION_EXTENDED_LAYOUT_INDEX,
	HANGUL_IC_OPTION_EXTENDED_LAYOUT_PREVKey,
	HANGUL_IC_OPTION_GALMADEULI_METHOD,
    HANGUL_IC_OPTION_EXTENDED_LAYOUT_STEP,
};

enum {// [모음글쇠, 확장기호글쇠, 확장한글글쇠, ]
  HANGUL_KEYBOARD_KEY_MOEUM,
  HANGUL_KEYBOARD_KEY_SYMBOL,
  HANGUL_KEYBOARD_KEY_YETGEUL,
};
enum {// [모음값, 확장기호값, 확장한글값, 확장단계기호값]
  HANGUL_KEYBOARD_VALUE_MOEUM,
  HANGUL_KEYBOARD_VALUE_SYMBOL,
  HANGUL_KEYBOARD_VALUE_YETGEUL,
  HANGUL_KEYBOARD_VALUE_EXTENDED_STEP,
};

enum {// [기호확장함수, 한글확장함수]
  HANGUL_FUNCTION_SYMBOL,
  HANGUL_FUNCTION_YETHANGEUL,
};

enum {// Combination // [기본조합,추가조합,갈마들이조합]
  HANGUL_COMBINATION_DEFAULT,
  HANGUL_COMBINATION_ADDON,
  HANGUL_COMBINATION_GALMADEULI,
};

enum {
    HANGUL_KEYBOARD_FLAG_EXTENDED,
    HANGUL_KEYBOARD_FLAG_GALMADEULI,
    HANGUL_KEYBOARD_FLAG_LOOSE_ORDER,
    HANGUL_KEYBOARD_FLAG_RIGHT_OU,
    HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT,
};



/* library */
int hangul_init();
int hangul_fini();

/* keyboard */
HangulKeyboard* hangul_keyboard_new(void);
HangulKeyboard* hangul_keyboard_new_from_file(const char* path);
void    hangul_keyboard_delete(HangulKeyboard *keyboard);
void    hangul_keyboard_set_type(HangulKeyboard *keyboard, int type);

unsigned int hangul_keyboard_list_get_count();

const char* hangul_keyboard_list_get_keyboard_id(unsigned index_);
const char* hangul_keyboard_list_get_keyboard_name(unsigned index_);
const HangulKeyboard* hangul_keyboard_list_get_keyboard(const char* id);
const char* hangul_keyboard_list_register_keyboard(HangulKeyboard* keyboard);
HangulKeyboard* hangul_keyboard_list_unregister_keyboard(const char* id);

char** 
libhangul_get_init_keyboard_ids (void);
unsigned int 
libhangul_get_init_keyboard_ids_length (void);

/* combination */
HangulCombination* hangul_combination_new(void);
void hangul_combination_delete(HangulCombination *combination);
bool hangul_combination_set_data(HangulCombination* combination, 
		     ucschar* first, ucschar* second, ucschar* result, unsigned int n);

/* input context */
HangulInputContext* hangul_ic_new(const char* keyboard);
void hangul_ic_delete(HangulInputContext *hic);
bool hangul_ic_process(HangulInputContext *hic, int ascii);
// 3beol
bool
hangul_ic_process_with_capslock(HangulInputContext *hic, int ascii, bool capslock);
unsigned int
hangul_ic_get_keyboard_flag(HangulInputContext *hic);
void
hangul_ic_set_keyboard_flag(HangulInputContext *hic, bool* value);

void hangul_ic_reset(HangulInputContext *hic);
bool hangul_ic_backspace(HangulInputContext *hic);

bool hangul_ic_is_empty(HangulInputContext *hic);
bool hangul_ic_has_choseong(HangulInputContext *hic);
bool hangul_ic_has_jungseong(HangulInputContext *hic);
bool hangul_ic_has_jongseong(HangulInputContext *hic);
bool hangul_ic_is_transliteration(HangulInputContext *hic);

bool hangul_ic_get_option(HangulInputContext *hic, int option);
void hangul_ic_set_option(HangulInputContext *hic, int option, bool value);
// 3beol
void
hangul_ic_set_option_int(HangulInputContext* hic, int option, int value);
int 
hangul_ic_get_option_int(HangulInputContext* hic, int option);

void hangul_ic_set_output_mode(HangulInputContext *hic, int mode);
void hangul_ic_set_keyboard(HangulInputContext *hic,
			    const HangulKeyboard *keyboard);
void hangul_ic_select_keyboard(HangulInputContext *hic,
			       const char* id);
void hangul_ic_connect_callback(HangulInputContext* hic, const char* event,
				void* callback, void* user_data);

const ucschar* hangul_ic_get_preedit_string(HangulInputContext *hic);
const ucschar* hangul_ic_get_commit_string(HangulInputContext *hic);
const ucschar* hangul_ic_flush(HangulInputContext *hic);

/* hanja.c */
typedef struct _Hanja Hanja;
typedef struct _HanjaList HanjaList;
typedef struct _HanjaTable HanjaTable;

HanjaTable*  hanja_table_load(const char *filename);
HanjaList*   hanja_table_match_exact(const HanjaTable* table, const char *key);
HanjaList*   hanja_table_match_prefix(const HanjaTable* table, const char *key);
HanjaList*   hanja_table_match_suffix(const HanjaTable* table, const char *key);
void         hanja_table_delete(HanjaTable *table);

int          hanja_list_get_size(const HanjaList *list);
const char*  hanja_list_get_key(const HanjaList *list);
const Hanja* hanja_list_get_nth(const HanjaList *list, unsigned int n);
const char*  hanja_list_get_nth_key(const HanjaList *list, unsigned int n);
const char*  hanja_list_get_nth_value(const HanjaList *list, unsigned int n);
const char*  hanja_list_get_nth_comment(const HanjaList *list, unsigned int n);
void         hanja_list_delete(HanjaList *list);

const char*  hanja_get_key(const Hanja* hanja);
const char*  hanja_get_value(const Hanja* hanja);
const char*  hanja_get_comment(const Hanja* hanja);

#ifdef __cplusplus
}
#endif

void    hangul_keyboard_set_value(HangulKeyboard *keyboard,
	int key, ucschar value) LIBHANGUL_DEPRECATED;
void hangul_ic_set_combination(HangulInputContext *hic,
	const HangulCombination *combination) LIBHANGUL_DEPRECATED;

unsigned    hangul_ic_get_n_keyboards() LIBHANGUL_DEPRECATED;
const char* hangul_ic_get_keyboard_id(unsigned index_) LIBHANGUL_DEPRECATED;
const char* hangul_ic_get_keyboard_name(unsigned index_) LIBHANGUL_DEPRECATED;

#undef LIBHANGUL_DEPRECATED

#endif /* libhangul_hangul_h */
