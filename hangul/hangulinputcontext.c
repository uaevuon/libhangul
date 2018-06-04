/* libhangul
 * Copyright (C) 2004 - 2016 Choe Hwanjin
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

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>

#include "hangul-gettext.h"
#include "hangul.h"
#include "hangulinternals.h"



/**
 * @defgroup hangulic 한글 입력 기능 구현
 * 
 * @section hangulicusage Hangul Input Context의 사용법
 * 이 섹션에서는 한글 입력 기능을 구현하는 핵심 기능에 대해 설명한다.
 *
 * 먼저 preedit string과 commit string 이 두 용어에 대해서 설명하겠다.
 * 이 두가지 용어는 Unix 계열의 입력기 framework에서 널리 쓰이는 표현이다.
 *
 * preedit string은 아직 조합중으로 어플리케이션에 완전히 입력되지 않은 
 * 스트링을 가리킨다. 일반적으로 한글 입력기에서는 역상으로 보이고
 * 일본 중국어 입력기에서는 underline이 붙어 나타난다. 아직 완성이 되지
 * 않은 스트링이므로 어플리케이션에 전달이 되지 않고 사라질 수도 있다.
 *
 * commit string은 조합이 완료되어 어플리케이션에 전달되는 스트링이다.
 * 이 스트링은 실제 어플리케이션의 텍스트로 인식이 되므로 이 이후에는
 * 더이상 입력기가 관리할 수 있는 데이터가 아니다.
 *
 * 한글 입력과정은 다음과 같은 과정을 거치게 된다.
 * 입력된 영문 키를 그에 해당하는 한글 자모로 변환한후 한글 자모를 모아
 * 하나의 음절을 만든다. 여기까지 이루어지는 과정을 preedit string 형태로
 * 사용자에게 계속 보이게 하는 것이 필요하다.
 * 그리고는 한글 음절이 완성되고나면 그 글자를 어플리케이션에 commit 
 * string 형태로 보내여 입력을 완료하는 것이다. 다음 키를 받게 되면 
 * 이 과정을 반복해서 수행한다.
 * 
 * libhangul에서 한글 조합 기능은 @ref HangulInputContext 를 이용해서 구현하게
 * 되는데 기본 적인 방법은 @ref HangulInputContext 에 사용자로부터의 입력을
 * 순서대로 전달하면서 그 상태가 바뀜에 따라서 preedit 나 commit 스트링을
 * 상황에 맞게 변화시키는 것이다.
 * 
 * 입력 코드들은 GUI 코드와 밀접하게 붙어 있어서 키 이벤트를 받아서
 * 처리하도록 구현하는 것이 보통이다. 그런데 유닉스에는 많은 입력 프레임웍들이
 * 난립하고 있는 상황이어서 매 입력 프레임웍마다 한글 조합 루틴을 작성해서
 * 넣는 것은 비효율적이다. 간단한 API를 구현하여 여러 프레임웍에서 바로 
 * 사용할 수 있도록 구현하는 편이 사용성이 높아지게 된다.
 *
 * 그래서 libhangul에서는 키 이벤트를 따로 재정의하지 않고 ASCII 코드를 
 * 직접 사용하는 방향으로 재정의된 데이터가 많지 않도록 하였다.
 * 실제 사용 방법은 말로 설명하는 것보다 샘플 코드를 사용하는 편이
 * 이해가 빠를 것이다. 그래서 대략적인 진행 과정을 샘플 코드로 
 * 작성하였다.
 *
 * 아래 예제는 실제로는 존재하지 않는 GUI 라이브러리 코드를 사용하였다.
 * 실제 GUI 코드를 사용하면 코드가 너무 길어져서 설명이 어렵고 코드가
 * 길어지면 핵심을 놓치기 쉽기 때문에 가공의 함수를 사용하였다.
 * 또한 텍스트의 encoding conversion 관련된 부분도 생략하였다.
 * 여기서 사용한 가공의 GUI 코드는 TWin으로 시작하게 하였다.
 *    
 * @code

    HangulInputContext* hic = hangul_ic_new("2");
    ...

    // 아래는 키 입력만 처리하는 이벤트 루프이다.
    // 실제 GUI코드는 이렇게 단순하진 않지만
    // 편의상 키 입력만 처리하는 코드로 작성하였다.

    TWinKeyEvent event = TWinGetKeyEvent(); // 키이벤트를 받는 이런 함수가
                        // 있다고 치자
    while (ascii != 0) {
    bool res;
    if (event.isBackspace()) {
        // backspace를 ascii로 변환하기가 좀 꺼림직해서
        // libhangul에서는 backspace 처리를 위한 
        // 함수를 따로 만들었다.
        res = hangul_ic_backspace(hic);
    } else {
        // 키 입력을 해당하는 ascii 코드로 변환한다.
        // libhangul에서는 이 ascii 코드가 키 이벤트
        // 코드와 마찬가지다.
        int ascii = event.getAscii();

        // 키 입력을 받았으면 이것을 hic에 먼저 보낸다.
        // 그래야 hic가 이 키를 사용할 것인지 아닌지를 판단할 수 있다.
        // 함수가 true를 리턴하면 이 키를 사용했다는 의미이므로 
        // GUI 코드가 이 키 입력을 프로세싱하지 않도록 해야 한다.
        // 그렇지 않으면 한 키입력이 두번 프로세싱된다.
        res = hangul_ic_process(hic, ascii);
    }
    
    // hic는 한번 키입력을 받고 나면 내부 상태 변화가 일어나고
    // 완성된 글자를 어플리케이션에 보내야 하는 상황이 있을 수 있다.
    // 이것을 HangulInputContext에서는 commit 스트링이 있는지로
    // 판단한다. commit 스트링을 받아봐서 스트링이 있다면 
    // 그 스트링으로 입력이 완료된 걸로 본다.
    const ucschar commit;
    commit = hangul_ic_get_commit_string(hic);
    if (commit[0] != 0) {    // 스트링의 길이를 재서 commit 스트링이 있는지
                // 판단한다.
        TWinInputUnicodeChars(commit);
    }

    // 키입력 후에는 preedit string도 역시 변화하게 되는데
    // 입력기 프레임웍에서는 이 스트링을 화면에 보여주어야
    // 조합중인 글자가 화면에 표시가 되는 것이다.
    const ucschar preedit;
    preedit = hangul_ic_get_preedit_string(hic);
    // 이 경우에는 스트링의 길이에 관계없이 항상 업데이트를 
    // 해야 한다. 왜냐하면 이전에 조합중이던 글자가 있다가
    // 조합이 완료되면서 조합중인 상태의 글자가 없어질 수도 있기 때문에
    // 스트링의 길이에 관계없이 현재 상태의 스트링을 preedit 
    // 스트링으로 보여주면 되는 것이다.
    TWinUpdatePreeditString(preedit);

    // 위 두작업이 끝난후에는 키 이벤트를 계속 프로세싱해야 하는지 
    // 아닌지를 처리해야 한다.
    // hic가 키 이벤트를 사용하지 않았다면 기본 GUI 코드에 계속해서
    // 키 이벤트 프로세싱을 진행하도록 해야 한다.
    if (!res)
        TWinForwardKeyEventToUI(ascii);

    ascii = GetKeyEvent();
    }

    hangul_ic_delete(hic);
     
 * @endcode
 */

/**
 * @file hangulinputcontext.c
 */

/**
 * @ingroup hangulic
 * @typedef HangulInputContext
 * @brief 한글 입력 상태를 관리하기 위한 오브젝트
 *
 * libhangul에서 제공하는 한글 조합 루틴에서 상태 정보를 저장하는 opaque
 * 데이타 오브젝트이다. 이 오브젝트에 키입력 정보를 순차적으로 보내주면서
 * preedit 스트링이나, commit 스트링을 받아서 처리하면 한글 입력 기능을
 * 손쉽게 구현할 수 있다.
 * 내부의 데이터 멤버는 공개되어 있지 않다. 각각의 멤버는 accessor 함수로만
 * 참조하여야 한다.
 */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef void   (*HangulOnTranslate)  (HangulInputContext*,
                      int,
                      ucschar*,
                      void*);
typedef bool   (*HangulOnTransition) (HangulInputContext*,
                      ucschar,
                      const ucschar*,
                      void*);

struct _HangulBuffer {
    ucschar choseong;
    ucschar jungseong;
    ucschar jongseong;

    // 3beol
    // 종성 쉬프트를 쓰는 3-91 최종 순아래 글판에서 쓰인다
    ucschar shift;
    // 신세벌식, 한글문와원 314 같은 왼/오른 ㅗㅜaraea 를 구분하는 글판에서 쓰인다
    ucschar right_oua;

    ucschar stack[12];
    int     index;
};

struct _HangulInputContext {
    int type;

    const HangulKeyboard*    keyboard;

    HangulBuffer buffer;
    int output_mode;

    ucschar preedit_string[64];
    ucschar commit_string[64];
    ucschar flushed_string[64];

    HangulOnTranslate   on_translate;
    void*               on_translate_data;

    HangulOnTransition  on_transition;
    void*               on_transition_data;

    unsigned int use_jamo_mode_only : 1;
    unsigned int option_auto_reorder : 1;
    unsigned int option_combi_on_double_stroke : 1;
    unsigned int option_non_choseong_combi : 1;

    // 3beol
    unsigned int option_galmadeuli_method : 1;
    unsigned int option_extended_layout_enable : 1;
    unsigned int option_extended_layout_index;
    unsigned int option_extended_layout_prevkey;
    unsigned int option_extended_layout_step;

};


static void    hangul_buffer_push(HangulBuffer *buffer, ucschar ch);
static ucschar hangul_buffer_pop (HangulBuffer *buffer);
static ucschar hangul_buffer_peek(HangulBuffer *buffer);

static void    hangul_buffer_clear(HangulBuffer *buffer);
static int     hangul_buffer_get_string(HangulBuffer *buffer, ucschar*buf, int buflen);
static int     hangul_buffer_get_jamo_string(HangulBuffer *buffer, ucschar *buf, int buflen);

static void    hangul_ic_flush_internal(HangulInputContext *hic);


static bool
hangul_buffer_is_empty(HangulBuffer *buffer)
{
    return buffer->choseong == 0 && buffer->jungseong == 0 &&
       buffer->jongseong == 0;
}

static bool
hangul_buffer_has_choseong(HangulBuffer *buffer)
{
    return buffer->choseong != 0;
}

static bool
hangul_buffer_has_jungseong(HangulBuffer *buffer)
{
    return buffer->jungseong != 0;
}

static bool
hangul_buffer_has_jongseong(HangulBuffer *buffer)
{
    return buffer->jongseong != 0;
}

static void
hangul_buffer_push(HangulBuffer *buffer, ucschar ch)
{
    if (hangul_is_choseong(ch)) {
    buffer->choseong = ch;
    } else if (hangul_is_jungseong(ch)) {
    buffer->jungseong = ch;
    } else if (hangul_is_jongseong(ch)) {
    buffer->jongseong = ch;
    } else {
    }

    buffer->stack[++buffer->index] = ch;
}

static ucschar
hangul_buffer_pop(HangulBuffer *buffer)
{
    return buffer->stack[buffer->index--];
}

static ucschar
hangul_buffer_peek(HangulBuffer *buffer)
{
    if (buffer->index < 0)
    return 0;

    return buffer->stack[buffer->index];
}

static void
hangul_buffer_clear(HangulBuffer *buffer)
{
    buffer->choseong = 0;
    buffer->jungseong = 0;
    buffer->jongseong = 0;

    buffer->index = -1;
    buffer->stack[0]  = 0;
    buffer->stack[1]  = 0;
    buffer->stack[2]  = 0;
    buffer->stack[3]  = 0;
    buffer->stack[4]  = 0;
    buffer->stack[5]  = 0;
    buffer->stack[6]  = 0;
    buffer->stack[7]  = 0;
    buffer->stack[8]  = 0;
    buffer->stack[9]  = 0;
    buffer->stack[10] = 0;
    buffer->stack[11] = 0;
}

static int
hangul_buffer_get_jamo_string(HangulBuffer *buffer, ucschar *buf, int buflen)
{
    int n = 0;

    if (buffer->choseong || buffer->jungseong || buffer->jongseong) {
    if (buffer->choseong) {
        buf[n++] = buffer->choseong;
    } else {
        buf[n++] = HANGUL_CHOSEONG_FILLER;
    }
    if (buffer->jungseong) {
        buf[n++] = buffer->jungseong;
    } else {
        buf[n++] = HANGUL_JUNGSEONG_FILLER;
    }
    if (buffer->jongseong) {
        buf[n++] = buffer->jongseong;
    }
    }

    buf[n] = 0;

    return n;
}

static int
hangul_jaso_to_string(ucschar cho, ucschar jung, ucschar jong,
              ucschar *buf, int len)
{
    ucschar ch = 0;
    int n = 0;

    if (cho) {
    if (jung) {
        /* have cho, jung, jong or no jong */
        ch = hangul_jamo_to_syllable(cho, jung, jong);
        if (ch != 0) {
        buf[n++] = ch;
        } else {
        /* 한글 음절로 표현 불가능한 경우 */
        buf[n++] = cho;
        buf[n++] = jung;
        if (jong != 0)
            buf[n++] = jong;
        }
    } else {
        if (jong) {
        /* have cho, jong */
        buf[n++] = cho;
        buf[n++] = HANGUL_JUNGSEONG_FILLER;
        buf[n++] = jong;
        } else {
        /* have cho */
        ch = hangul_jamo_to_cjamo(cho);
        if (hangul_is_cjamo(ch)) {
            buf[n++] = ch;
        } else {
            buf[n++] = cho;
            buf[n++] = HANGUL_JUNGSEONG_FILLER;
        }
        }
    }
    } else {
    if (jung) {
        if (jong) {
        /* have jung, jong */
        buf[n++] = HANGUL_CHOSEONG_FILLER;
        buf[n++] = jung;
        buf[n++] = jong;
        } else {
        /* have jung */
        ch = hangul_jamo_to_cjamo(jung);
        if (hangul_is_cjamo(ch)) {
            buf[n++] = ch;
        } else {
            buf[n++] = HANGUL_CHOSEONG_FILLER;
            buf[n++] = jung;
        }
        }
    } else {
        if (jong) { 
        /* have jong */
        ch = hangul_jamo_to_cjamo(jong);
        if (hangul_is_cjamo(ch)) {
            buf[n++] = ch;
        } else {
            buf[n++] = HANGUL_CHOSEONG_FILLER;
            buf[n++] = HANGUL_JUNGSEONG_FILLER;
            buf[n++] = jong;
        }
        } else {
        /* have nothing */
        buf[n] = 0;
        }
    }
    }
    buf[n] = 0;

    return n;
}

static int
hangul_buffer_get_string(HangulBuffer *buffer, ucschar *buf, int buflen)
{
    return hangul_jaso_to_string(buffer->choseong,
                 buffer->jungseong,
                 buffer->jongseong,
                 buf, buflen);
}

static bool
hangul_buffer_backspace(HangulBuffer *buffer)
{
    if (buffer->index >= 0) {
    ucschar ch = hangul_buffer_pop(buffer);
    if (ch == 0)
        return false;

    if (buffer->index >= 0) {
        if (hangul_is_choseong(ch)) {
        ch = hangul_buffer_peek(buffer);
        buffer->choseong = hangul_is_choseong(ch) ? ch : 0;
        return true;
        } else if (hangul_is_jungseong(ch)) {
        ch = hangul_buffer_peek(buffer);
        buffer->jungseong = hangul_is_jungseong(ch) ? ch : 0;
        return true;
        } else if (hangul_is_jongseong(ch)) {
        ch = hangul_buffer_peek(buffer);
        buffer->jongseong = hangul_is_jongseong(ch) ? ch : 0;
        return true;
        }
    } else {
        buffer->choseong = 0;
        buffer->jungseong = 0;
        buffer->jongseong = 0;
        return true;
    }
    }
    return false;
}

static inline bool
hangul_ic_push(HangulInputContext *hic, ucschar c)
{
    ucschar buf[64] = { 0, };
    if (hic->on_transition != NULL) {
    ucschar cho, jung, jong;
    if (hangul_is_choseong(c)) {
        cho  = c;
        jung = hic->buffer.jungseong;
        jong = hic->buffer.jongseong;
    } else if (hangul_is_jungseong(c)) {
        cho  = hic->buffer.choseong;
        jung = c;
        jong = hic->buffer.jongseong;
    } else if (hangul_is_jongseong(c)) {
        cho  = hic->buffer.choseong;
        jung = hic->buffer.jungseong;
        jong = c;
    } else {
        hangul_ic_flush_internal(hic);
        return false;
    }

    hangul_jaso_to_string(cho, jung, jong, buf, N_ELEMENTS(buf));
    if (!hic->on_transition(hic, c, buf, hic->on_transition_data)) {
        hangul_ic_flush_internal(hic);
        return false;
    }
    } else {
    if (!hangul_is_jamo(c)) {
        hangul_ic_flush_internal(hic);
        return false;
    }
    }

    hangul_buffer_push(&hic->buffer, c);
    return true;
}

static inline ucschar
hangul_ic_pop(HangulInputContext *hic)
{
    return hangul_buffer_pop(&hic->buffer);
}

static inline ucschar
hangul_ic_peek(HangulInputContext *hic)
{
    return hangul_buffer_peek(&hic->buffer);
}

static inline void
hangul_ic_save_preedit_string(HangulInputContext *hic)
{
    if (hic->output_mode == HANGUL_OUTPUT_JAMO) 
    {
    hangul_buffer_get_jamo_string(&hic->buffer,
                      hic->preedit_string,
                      N_ELEMENTS(hic->preedit_string));
    } else {
    hangul_buffer_get_string(&hic->buffer,
                 hic->preedit_string,
                 N_ELEMENTS(hic->preedit_string));
    }
}

static inline void
hangul_ic_append_commit_string(HangulInputContext *hic, ucschar ch)
{
    int i;

    for (i = 0; i < N_ELEMENTS(hic->commit_string); i++) {
    if (hic->commit_string[i] == 0)
        break;
    }

    if (i + 1 < N_ELEMENTS(hic->commit_string)) {
    hic->commit_string[i++] = ch;
    hic->commit_string[i] = 0;
    }
}

static inline void
hangul_ic_save_commit_string(HangulInputContext *hic)
{
    ucschar *string = hic->commit_string;
    int len = N_ELEMENTS(hic->commit_string);

    while (len > 0) {
    if (*string == 0)
        break;
    len--;
    string++;
    }

    if (hic->output_mode == HANGUL_OUTPUT_JAMO) {
    hangul_buffer_get_jamo_string(&hic->buffer, string, len);
    } else {
    hangul_buffer_get_string(&hic->buffer, string, len);
    }

    // 3beol
    hic->option_extended_layout_index = 0;
    hic->option_extended_layout_prevkey = 0;


    hangul_buffer_clear(&hic->buffer);
}

static ucschar
hangul_ic_choseong_to_jongseong(HangulInputContext* hic, ucschar cho)
{
    ucschar jong = hangul_choseong_to_jongseong(cho);
    if (hangul_is_jongseong_conjoinable(jong)) {
    return jong;
    } else {
    /* 옛글 조합 규칙을 사용하는 자판의 경우에는 종성이 conjoinable
     * 하지 않아도 상관없다 */
    int type = hangul_keyboard_get_type(hic->keyboard);
    switch (type) {
    case HANGUL_KEYBOARD_TYPE_JAMO_YET:
    case HANGUL_KEYBOARD_TYPE_JASO_YET:
        return jong;
    }
    }

    return 0;
}

static ucschar
hangul_ic_combine(HangulInputContext* hic, ucschar first, ucschar second)
{
    if (!hic->option_combi_on_double_stroke) {
        if (hangul_keyboard_get_type(hic->keyboard) == HANGUL_KEYBOARD_TYPE_JAMO) {
            /* 옛한글은 아래 규칙을 적용하지 않아야 입력가능한 글자가 있으므로
            * 적용하지 않는다. */
            if (first == second && hangul_is_jamo_conjoinable(first)) {
                return 0;
            }
        }
    }

    ucschar combined = 0;
    if (first == 0)
    {// 갈마들이.
        combined = hangul_keyboard_combine(hic->keyboard, HANGUL_COMBINATION_GALMADEULI, first, second);

    }
    else 
    {
        combined = hangul_keyboard_combine(hic->keyboard, HANGUL_COMBINATION_ADDON, first, second);
        if (combined)
        {
            // 확장 조합.
        }
        else
        {
            combined = hangul_keyboard_combine(hic->keyboard, HANGUL_COMBINATION_DEFAULT, first, second);
        }
    }

    if (!hic->option_non_choseong_combi) {
        if (hangul_is_choseong(first) && 
            hangul_is_choseong(second) &&
            hangul_is_jongseong(combined)) {
            return 0;
        }
    }

    return combined;
}

static bool
hangul_ic_process_jamo(HangulInputContext *hic, ucschar ch)
{
    ucschar jong;
    ucschar combined;

    if (!hangul_is_jamo(ch) && ch > 0) {
    hangul_ic_save_commit_string(hic);
    hangul_ic_append_commit_string(hic, ch);
    return true;
    }

    if (hic->buffer.jongseong) {
    if (hangul_is_choseong(ch)) {
        jong = hangul_ic_choseong_to_jongseong(hic, ch);
        combined = hangul_ic_combine(hic, hic->buffer.jongseong, jong);
        if (hangul_is_jongseong(combined)) {
        if (!hangul_ic_push(hic, combined)) {
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        } else {
        hangul_ic_save_commit_string(hic);
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    } else if (hangul_is_jungseong(ch)) {
        ucschar pop, peek;
        pop = hangul_ic_pop(hic);
        peek = hangul_ic_peek(hic);

        if (hangul_is_jongseong(peek)) {
        ucschar choseong = hangul_jongseong_get_diff(peek,
                         hic->buffer.jongseong);
        if (choseong == 0) {
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        } else {
            hic->buffer.jongseong = peek;
            hangul_ic_save_commit_string(hic);
            hangul_ic_push(hic, choseong);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        } else {
        hic->buffer.jongseong = 0;
        hangul_ic_save_commit_string(hic);
        hangul_ic_push(hic, hangul_jongseong_to_choseong(pop));
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    } else {
        goto flush;
    }
    } else if (hic->buffer.jungseong) {
    if (hangul_is_choseong(ch)) {
        if (hic->buffer.choseong) {
        jong = hangul_ic_choseong_to_jongseong(hic, ch);
        if (hangul_is_jongseong(jong)) {
            if (!hangul_ic_push(hic, jong)) {
            if (!hangul_ic_push(hic, ch)) {
                return false;
            }
            }
        } else {
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        } else {
        if (hic->option_auto_reorder) {
            /* kr 처럼 자모가 역순인 경우 처리 */
            if (!hangul_ic_push(hic, ch)) {
            if (!hangul_ic_push(hic, ch)) {
                return false;
            }
            }
        } else {
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        }
    } else if (hangul_is_jungseong(ch)) {
        combined = hangul_ic_combine(hic, hic->buffer.jungseong, ch);
        if (hangul_is_jungseong(combined)) {
        if (!hangul_ic_push(hic, combined)) {
            return false;
        }
        } else {
        hangul_ic_save_commit_string(hic);
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    } else {
        goto flush;
    }
    } else if (hic->buffer.choseong) {
    if (hangul_is_choseong(ch)) {
        combined = hangul_ic_combine(hic, hic->buffer.choseong, ch);
        /* 초성을 입력한 combine 함수에서 종성이 나오게 된다면
         * 이전 초성도 종성으로 바꿔 주는 편이 나머지 처리에 편리하다.
         * 이 기능은 MS IME 호환기능으로 ㄳ을 입력하는데 사용한다. */
        if (hangul_is_jongseong(combined)) {
        hic->buffer.choseong = 0;
        ucschar pop = hangul_ic_pop(hic);
        ucschar jong = hangul_choseong_to_jongseong(pop);
        hangul_ic_push(hic, jong);
        }

        if (!hangul_ic_push(hic, combined)) {
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    } else {
        if (!hangul_ic_push(hic, ch)) {
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    }
    } else {
    if (!hangul_ic_push(hic, ch)) {
        return false;
    }
    }

    hangul_ic_save_preedit_string(hic);
    return true;

flush:
    hangul_ic_flush_internal(hic);
    return false;
}

static bool
hangul_ic_process_jaso(HangulInputContext *hic, ucschar ch)
{
    if (hangul_is_choseong(ch)) {
    if (hic->buffer.choseong == 0) {
        if (hic->option_auto_reorder) {
        if (!hangul_ic_push(hic, ch)) {
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        } else {
        if (hangul_ic_has_jungseong(hic) || hangul_ic_has_jongseong(hic)) {
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        } else {
            if (!hangul_ic_push(hic, ch)) {
            if (!hangul_ic_push(hic, ch)) {
                return false;
            }
            }
        }
        }
    } else {
        ucschar choseong = 0;
        if (hangul_is_choseong(hangul_ic_peek(hic))) {
        choseong = hangul_ic_combine(hic, hic->buffer.choseong, ch);
        }
        if (choseong) {
        if (!hangul_ic_push(hic, choseong)) {
            if (!hangul_ic_push(hic, choseong)) {
            return false;
            }
        }
        } else {
        hangul_ic_save_commit_string(hic);
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    }
    } else if (hangul_is_jungseong(ch)) {
    if (hic->buffer.jungseong == 0) {
        if (hic->option_auto_reorder) {
        if (!hangul_ic_push(hic, ch)) {
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        } else {
        if (hangul_ic_has_jongseong(hic)) {
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        } else {
            if (!hangul_ic_push(hic, ch)) {
            if (!hangul_ic_push(hic, ch)) {
                return false;
            }
            }
        }
        }
    } else {
        ucschar jungseong = 0;
        if (hangul_is_jungseong(hangul_ic_peek(hic))) {
        jungseong = hangul_ic_combine(hic, hic->buffer.jungseong, ch);
        }
        if (jungseong) {
        if (!hangul_ic_push(hic, jungseong)) {
            if (!hangul_ic_push(hic, jungseong)) {
            return false;
            }
        }
        } else {
        hangul_ic_save_commit_string(hic);
        if (!hangul_ic_push(hic, ch)) {
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        }
    }
    } else if (hangul_is_jongseong(ch)) {
    if (hic->buffer.jongseong == 0) {
        if (!hangul_ic_push(hic, ch)) {
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    } else {
        ucschar jongseong = 0;
        if (hangul_is_jongseong(hangul_ic_peek(hic))) {
        jongseong = hangul_ic_combine(hic, hic->buffer.jongseong, ch);
        }
        if (jongseong) {
        if (!hangul_ic_push(hic, jongseong)) {
            if (!hangul_ic_push(hic, jongseong)) {
            return false;
            }
        }
        } else {
        hangul_ic_save_commit_string(hic);
        if (!hangul_ic_push(hic, ch)) {
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        }
    }
    } else if (ch > 0) {
    hangul_ic_save_commit_string(hic);
    hangul_ic_append_commit_string(hic, ch);
    } else {
    hangul_ic_save_commit_string(hic);
    return false;
    }

    hangul_ic_save_preedit_string(hic);
    return true;
}

static bool
hangul_ic_process_romaja(HangulInputContext *hic, int ascii, ucschar ch)
{
    ucschar jong;
    ucschar combined;

    if (!hangul_is_jamo(ch) && ch > 0) {
    hangul_ic_save_commit_string(hic);
    hangul_ic_append_commit_string(hic, ch);
    return true;
    }

    if (isupper(ascii)) {
    hangul_ic_save_commit_string(hic);
    }

    if (hic->buffer.jongseong) {
    if (ascii == 'x' || ascii == 'X') {
        ch = 0x110c;
        hangul_ic_save_commit_string(hic);
        if (!hangul_ic_push(hic, ch)) {
        return false;
        }
    } else if (hangul_is_choseong(ch) || hangul_is_jongseong(ch)) {
        if (hangul_is_jongseong(ch))
        jong = ch;
        else
        jong = hangul_ic_choseong_to_jongseong(hic, ch);
        combined = hangul_ic_combine(hic, hic->buffer.jongseong, jong);
        if (hangul_is_jongseong(combined)) {
        if (!hangul_ic_push(hic, combined)) {
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        } else {
        hangul_ic_save_commit_string(hic);
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    } else if (hangul_is_jungseong(ch)) {
        if (hic->buffer.jongseong == 0x11bc) {
        hangul_ic_save_commit_string(hic);
        hic->buffer.choseong = 0x110b;
        hangul_ic_push(hic, ch);
        } else {
        ucschar pop, peek;
        pop = hangul_ic_pop(hic);
        peek = hangul_ic_peek(hic);

        if (hangul_is_jungseong(peek)) {
            if (pop == 0x11aa) {
            hic->buffer.jongseong = 0x11a8;
            pop = 0x11ba;
            } else {
            hic->buffer.jongseong = 0;
            }
            hangul_ic_save_commit_string(hic);
            hangul_ic_push(hic, hangul_jongseong_to_choseong(pop));
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        } else {
            ucschar choseong = 0, jongseong = 0; 
            hangul_jongseong_decompose(hic->buffer.jongseong,
                           &jongseong, &choseong);
            hic->buffer.jongseong = jongseong;
            hangul_ic_save_commit_string(hic);
            hangul_ic_push(hic, choseong);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        }
    } else {
        goto flush;
    }
    } else if (hic->buffer.jungseong) {
    if (hangul_is_choseong(ch)) {
        if (hic->buffer.choseong) {
        jong = hangul_ic_choseong_to_jongseong(hic, ch);
        if (hangul_is_jongseong(jong)) {
            if (!hangul_ic_push(hic, jong)) {
            if (!hangul_ic_push(hic, ch)) {
                return false;
            }
            }
        } else {
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        } else {
        if (!hangul_ic_push(hic, ch)) {
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        }
    } else if (hangul_is_jungseong(ch)) {
        combined = hangul_ic_combine(hic, hic->buffer.jungseong, ch);
        if (hangul_is_jungseong(combined)) {
        if (!hangul_ic_push(hic, combined)) {
            return false;
        }
        } else {
        hangul_ic_save_commit_string(hic);
        hic->buffer.choseong = 0x110b;
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    } else if (hangul_is_jongseong(ch)) {
        if (!hangul_ic_push(hic, ch)) {
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    } else {
        goto flush;
    }
    } else if (hic->buffer.choseong) {
    if (hangul_is_choseong(ch)) {
        combined = hangul_ic_combine(hic, hic->buffer.choseong, ch);
        if (combined == 0) {
        hic->buffer.jungseong = 0x1173;
        hangul_ic_flush_internal(hic);
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        } else {
        if (!hangul_ic_push(hic, combined)) {
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        }
    } else if (hangul_is_jongseong(ch)) {
        hic->buffer.jungseong = 0x1173;
        hangul_ic_save_commit_string(hic);
        if (ascii == 'x' || ascii == 'X')
        ch = 0x110c;
        if (!hangul_ic_push(hic, ch)) {
        return false;
        }
    } else {
        if (!hangul_ic_push(hic, ch)) {
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    }
    } else {
    if (ascii == 'x' || ascii == 'X') {
        ch = 0x110c;
    }

    if (!hangul_ic_push(hic, ch)) {
        return false;
    } else {
        if (hic->buffer.choseong == 0 && hic->buffer.jungseong != 0)
        hic->buffer.choseong = 0x110b;
    }
    }

    hangul_ic_save_preedit_string(hic);
    return true;

flush:
    hangul_ic_flush_internal(hic);
    return false;
}

/**
 * @ingroup hangulic
 * @brief 키 입력을 처리하여 실제로 한글 조합을 하는 함수
 * @param hic @ref HangulInputContext 오브젝트
 * @param ascii 키 이벤트
 * @return @ref HangulInputContext 가 이 키를 사용했으면 true,
 *         사용하지 않았으면 false
 *
 * ascii 값으로 주어진 키 이벤트를 받아서 내부의 한글 조합 상태를
 * 변화시키고, preedit, commit 스트링을 저장한다.
 *
 * libhangul의 키 이벤트 프로세스는 ASCII 코드 값을 기준으로 처리한다.
 * 이 키 값은 US Qwerty 자판 배열에서의 키 값에 해당한다.
 * 따라서 유럽어 자판을 사용하는 경우에는 해당 키의 ASCII 코드를 직접
 * 전달하면 안되고, 그 키가 US Qwerty 자판이었을 경우에 발생할 수 있는 
 * ASCII 코드 값을 주어야 한다.
 * 또한 ASCII 코드 이므로 Shift 상태는 대문자로 전달이 된다.
 * Capslock이 눌린 경우에는 대소문자를 뒤바꾸어 보내주지 않으면 
 * 마치 Shift가 눌린 것 처럼 동작할 수 있으므로 주의한다.
 * preedit, commit 스트링은 hangul_ic_get_preedit_string(),
 * hangul_ic_get_commit_string() 함수를 이용하여 구할 수 있다.
 * 
 * 이 함수의 사용법에 대한 설명은 @ref hangulicusage 부분을 참조한다.
 *
 * @remarks 이 함수는 @ref HangulInputContext 의 상태를 변화 시킨다.
 */
bool
hangul_ic_process(HangulInputContext *hic, int ascii)
{
    return hangul_ic_process_with_capslock (hic, ascii, FALSE);
}

bool
hangul_ic_process_with_capslock(HangulInputContext *hic, int ascii, bool capslock)
{
    ucschar c;

    if (hic == NULL)
    return false;

    hic->preedit_string[0] = 0;
    hic->commit_string[0] = 0;

    c = hangul_keyboard_get_mapping(hic->keyboard, 0, ascii);
    if (hic->on_translate != NULL)
    hic->on_translate(hic, ascii, &c, hic->on_translate_data);

    if (ascii == '\b') {
    return hangul_ic_backspace(hic);
    }

    int type = hangul_keyboard_get_type(hic->keyboard);
    switch (type) {
    case HANGUL_KEYBOARD_TYPE_JASO:
    case HANGUL_KEYBOARD_TYPE_JASO_YET:
        #ifdef libhangul_3beol
        return hangul_ic_process_jaso_sebeol (hic, ascii, c);
        #else
    	return hangul_ic_process_jaso(hic, c);
        #endif
    case HANGUL_KEYBOARD_TYPE_JASO_SHIN:
        return hangul_ic_process_jaso_shin_sebeol (hic, ascii, c);
    case HANGUL_KEYBOARD_TYPE_JASO_SHIN_YET:
        return hangul_ic_process_jaso_shin_sebeol_yet (hic, ascii, c, capslock);
    case HANGUL_KEYBOARD_TYPE_JASO_SHIN_SHIFT:
        return hangul_ic_process_jaso_shin_sebeol_shift (hic, ascii, c);
    case HANGUL_KEYBOARD_TYPE_3FINALSUN:
        return hangul_ic_process_3finalsun(hic, ascii, c);
    case HANGUL_KEYBOARD_TYPE_ROMAJA:
    	return hangul_ic_process_romaja(hic, ascii, c);
    default:
        #ifdef libhangul_3beol
        return hangul_ic_process_jamo_dubeol(hic, c);
        #else
    	return hangul_ic_process_jamo(hic, c);
        #endif
    }
}

/**
 * @ingroup hangulic
 * @brief 현재 상태의 preedit string을 구하는 함수
 * @param hic preedit string을 구하고자하는 입력 상태 object
 * @return UCS4 preedit 스트링, 이 스트링은 @a hic 내부의 데이터이므로 
 *         수정하거나 free해서는 안된다.
 * 
 * 이 함수는  @a hic 내부의 현재 상태의 preedit string을 리턴한다.
 * 따라서 hic가 다른 키 이벤트를 처리하고 나면 그 내용이 바뀔 수 있다.
 * 
 * @remarks 이 함수는 @ref HangulInputContext 의 상태를 변화 시키지 않는다.
 */
const ucschar*
hangul_ic_get_preedit_string(HangulInputContext *hic)
{
    if (hic == NULL)
    return NULL;

    return hic->preedit_string;
}

/**
 * @ingroup hangulic
 * @brief 현재 상태의 commit string을 구하는 함수
 * @param hic commit string을 구하고자하는 입력 상태 object
 * @return UCS4 commit 스트링, 이 스트링은 @a hic 내부의 데이터이므로 
 *         수정하거나 free해서는 안된다.
 * 
 * 이 함수는  @a hic 내부의 현재 상태의 commit string을 리턴한다.
 * 따라서 hic가 다른 키 이벤트를 처리하고 나면 그 내용이 바뀔 수 있다.
 *
 * @remarks 이 함수는 @ref HangulInputContext 의 상태를 변화 시키지 않는다.
 */
const ucschar*
hangul_ic_get_commit_string(HangulInputContext *hic)
{
    if (hic == NULL)
    return NULL;

    return hic->commit_string;
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 를 초기상태로 되돌리는 함수
 * @param hic @ref HangulInputContext 를 가리키는 포인터
 * 
 * 이 함수는 @a hic가 가리키는 @ref HangulInputContext 의 상태를 
 * 처음 상태로 되돌린다. preedit 스트링, commit 스트링, flush 스트링이
 * 없어지고, 입력되었던 키에 대한 기록이 없어진다.
 * 영어 상태로 바뀌는 것이 아니다.
 *
 * 비교: hangul_ic_flush()
 *
 * @remarks 이 함수는 @ref HangulInputContext 의 상태를 변화 시킨다.
 */
void
hangul_ic_reset(HangulInputContext *hic)
{
    if (hic == NULL)
    return;

    hic->preedit_string[0] = 0;
    hic->commit_string[0] = 0;
    hic->flushed_string[0] = 0;

    // 3beol
    hic->option_extended_layout_index = 0;
    hic->option_extended_layout_prevkey = 0;

    hangul_buffer_clear(&hic->buffer);
}

/* append current preedit to the commit buffer.
 * this function does not clear previously made commit string. */
static void
hangul_ic_flush_internal(HangulInputContext *hic)
{
    hic->preedit_string[0] = 0;

    hangul_ic_save_commit_string(hic);
    hangul_buffer_clear(&hic->buffer);
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 의 입력 상태를 완료하는 함수
 * @param hic @ref HangulInputContext 를 가리키는 포인터
 * @return 조합 완료된 스트링, 스트링의 길이가 0이면 조합 완료된 스트링이 
 *      없는 것
 *
 * 이 함수는 @a hic가 가리키는 @ref HangulInputContext 의 입력 상태를 완료한다.
 * 조합중이던 스트링을 완성하여 리턴한다. 그리고 입력 상태가 초기 상태로 
 * 되돌아 간다. 조합중이던 글자를 강제로 commit하고 싶을때 사용하는 함수다.
 * 보통의 경우 입력 framework에서 focus가 나갈때 이 함수를 불러서 마지막 
 * 상태를 완료해야 조합중이던 글자를 잃어버리지 않게 된다.
 *
 * 비교: hangul_ic_reset()
 *
 * @remarks 이 함수는 @ref HangulInputContext 의 상태를 변화 시킨다.
 */
const ucschar*
hangul_ic_flush(HangulInputContext *hic)
{
    if (hic == NULL)
    return NULL;

    // get the remaining string and clear the buffer
    hic->preedit_string[0] = 0;
    hic->commit_string[0] = 0;
    hic->flushed_string[0] = 0;

    // 3beol
    hic->option_extended_layout_index = 0;
    hic->option_extended_layout_prevkey = 0;

    if (hic->output_mode == HANGUL_OUTPUT_JAMO) {
    hangul_buffer_get_jamo_string(&hic->buffer, hic->flushed_string,
                 N_ELEMENTS(hic->flushed_string));
    } else {
    hangul_buffer_get_string(&hic->buffer, hic->flushed_string,
                 N_ELEMENTS(hic->flushed_string));
    }

    hangul_buffer_clear(&hic->buffer);

    return hic->flushed_string;
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 가 backspace 키를 처리하도록 하는 함수
 * @param hic @ref HangulInputContext 를 가리키는 포인터
 * @return @a hic가 키를 사용했으면 true, 사용하지 않았으면 false
 * 
 * 이 함수는 @a hic가 가리키는 @ref HangulInputContext 의 조합중이던 글자를
 * 뒤에서부터 하나 지우는 기능을 한다. backspace 키를 눌렀을 때 발생하는 
 * 동작을 한다. 따라서 이 함수를 부르고 나면 preedit string이 바뀌므로
 * 반드시 업데이트를 해야 한다.
 *
 * @remarks 이 함수는 @ref HangulInputContext 의 상태를 변화 시킨다.
 */
bool
hangul_ic_backspace(HangulInputContext *hic)
{
    int ret;

    if (hic == NULL)
    return false;

    hic->preedit_string[0] = 0;
    hic->commit_string[0] = 0;

    ret = hangul_buffer_backspace(&hic->buffer);
    if (ret)
    {
        if (hic->option_extended_layout_index >= 1 && hic->option_extended_layout_index <= 5) {
            hic->option_extended_layout_index -= 1;
        } else {
            // 신세벌식은 단계별 글쇠가 따로 있기때문에 확장모드에서 바로 빠져나온다
            hic->option_extended_layout_index = 0;
        }

        hangul_ic_save_preedit_string(hic);
    }
    return ret;
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 가 조합중인 글자를 가지고 있는지 확인하는 함수
 * @param hic @ref HangulInputContext 를 가리키는 포인터
 *
 * @ref HangulInputContext 가 조합중인 글자가 있으면 true를 리턴한다.
 *
 * @remarks 이 함수는 @ref HangulInputContext 의 상태를 변화 시키지 않는다.
 */
bool
hangul_ic_is_empty(HangulInputContext *hic)
{
    return hangul_buffer_is_empty(&hic->buffer);
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 가 조합중인 초성을 가지고 있는지 확인하는 함수
 * @param hic @ref HangulInputContext 를 가리키는 포인터
 *
 * @ref HangulInputContext 가 조합중인 글자가 초성이 있으면 true를 리턴한다.
 *
 * @remarks 이 함수는 @ref HangulInputContext 의 상태를 변화 시키지 않는다.
 */
bool
hangul_ic_has_choseong(HangulInputContext *hic)
{
    return hangul_buffer_has_choseong(&hic->buffer);
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 가 조합중인 중성을 가지고 있는지 확인하는 함수
 * @param hic @ref HangulInputContext 를 가리키는 포인터
 *
 * @ref HangulInputContext 가 조합중인 글자가 중성이 있으면 true를 리턴한다.
 *
 * @remarks 이 함수는 @ref HangulInputContext 의 상태를 변화 시키지 않는다.
 */
bool
hangul_ic_has_jungseong(HangulInputContext *hic)
{
    return hangul_buffer_has_jungseong(&hic->buffer);
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 가 조합중인 종성을 가지고 있는지 확인하는 함수
 * @param hic @ref HangulInputContext 를 가리키는 포인터
 *
 * @ref HangulInputContext 가 조합중인 글자가 종성이 있으면 true를 리턴한다.
 *
 * @remarks 이 함수는 @ref HangulInputContext 의 상태를 변화 시키지 않는다.
 */
bool
hangul_ic_has_jongseong(HangulInputContext *hic)
{
    return hangul_buffer_has_jongseong(&hic->buffer);
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 의 조합 옵션을 확인하는 함수
 * @param hic @ref HangulInputContext 를 가리키는 포인터
 * @param option @ref HangulInputContext 의 조합 처리 옵션.
 *        @ref hangul_ic_set_option 에서 사용 가능한 옵션을 확인하라.
 *
 * @return @ref HangulInputContext 에 설정된 옵션값을 리턴한다.
 */
bool
hangul_ic_get_option(HangulInputContext* hic, int option)
{
    switch (option) {
    case HANGUL_IC_OPTION_AUTO_REORDER:
        return hic->option_auto_reorder;
    case HANGUL_IC_OPTION_COMBI_ON_DOUBLE_STROKE:
        return hic->option_combi_on_double_stroke;
    case HANGUL_IC_OPTION_NON_CHOSEONG_COMBI:
        return hic->option_non_choseong_combi;
	case HANGUL_IC_OPTION_EXTENDED_LAYOUT:
	    return hic->option_extended_layout_enable;
    case HANGUL_IC_OPTION_EXTENDED_LAYOUT_INDEX:
	    return hic->option_extended_layout_index;
    case HANGUL_IC_OPTION_EXTENDED_LAYOUT_PREVKey:
	    return hic->option_extended_layout_prevkey;
    case HANGUL_IC_OPTION_GALMADEULI_METHOD:
	    return hic->option_galmadeuli_method;
    }

    return false;
}
int 
hangul_ic_get_option_int(HangulInputContext* hic, int option)
{
    switch (option) {
    case HANGUL_IC_OPTION_EXTENDED_LAYOUT_INDEX:
	    return hic->option_extended_layout_index;
    case HANGUL_IC_OPTION_EXTENDED_LAYOUT_PREVKey:
	    return hic->option_extended_layout_prevkey;
    case HANGUL_IC_OPTION_EXTENDED_LAYOUT_STEP:
        return hic->option_extended_layout_step;
    }

    return 0;
}
/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 의 조합 옵션을 설정하는 함수
 * @param hic @ref HangulInputContext 를 가리키는 포인터
 * @param option @ref HangulInputContext 의 조합 처리 옵션.
 *               아래와 같은 옵션이 선택 가능하다.
 *    - HANGUL_IC_OPTION_AUTO_REORDER
 *      - 자동 순서 교정 옵션. 모아치기를 위해서는 켜야 한다.
 *        예) true면 ㅏ+ㄱ -> 가 로 완성시켜 줌.
 *    - HANGUL_IC_OPTION_COMBI_ON_DOUBLE_STROKE
 *      - 두벌식 자판에서 자음 연타로 된소리로 조합하는 옵션.
 *        원래 자판이 자음 연타로 동작하는 세벌식이나 옛한글
 *        자판에서는 옵션이 동작하지 않는다.
 *        MS IME와 호환을 위해서 사용.
 *        예) true면 ㄱ+ㄱ -> ㄲ 으로 조합시켜 줌.
 *    - HANGUL_IC_OPTION_NON_CHOSEONG_COMBI
 *      - 두벌식 자판에서 초성에 없는 겹자음을 조합허용하는 옵션.
 *        두벌식 자판 이외에는 옵션이 동작하지 않는다.
 *        MS IME와 호환을 위해서 사용.
 *        예) true면 ㄱ+ㅅ -> ㄳ 으로 조합시켜 줌.
 * @param value 설정하고자 하는 값, true 또는 false
 */
void
hangul_ic_set_option(HangulInputContext* hic, int option, bool value)
{
    switch (option) {
    case HANGUL_IC_OPTION_AUTO_REORDER:
        if (hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) 
        {
            hic->option_auto_reorder = value;
        } 
        else {
            // LOOSE_ORDER 가 있으면 건너뛴다.
        }
        break;
    case HANGUL_IC_OPTION_COMBI_ON_DOUBLE_STROKE:
        hic->option_combi_on_double_stroke = value;
        break;
    case HANGUL_IC_OPTION_NON_CHOSEONG_COMBI:
        hic->option_non_choseong_combi = value;
        break;
	case HANGUL_IC_OPTION_EXTENDED_LAYOUT:
        hic->option_extended_layout_enable = value;
	    break;
    case HANGUL_IC_OPTION_GALMADEULI_METHOD:
	    hic->option_galmadeuli_method = value;
		break;
    default:
        break;
    }
}

void
hangul_ic_set_option_int(HangulInputContext* hic, int option, int value)
{
    switch (option) {
    case HANGUL_IC_OPTION_EXTENDED_LAYOUT_INDEX:
	    hic->option_extended_layout_index = value;
		break;
    case HANGUL_IC_OPTION_EXTENDED_LAYOUT_PREVKey:
	    hic->option_extended_layout_prevkey = value;
		break;
    case HANGUL_IC_OPTION_EXTENDED_LAYOUT_STEP:
        hic->option_extended_layout_step = value;
        break;
    default:
        break;
    }
}


void
hangul_ic_set_output_mode(HangulInputContext *hic, int mode)
{
    if (hic == NULL)
    return;

    if (!hic->use_jamo_mode_only)
    hic->output_mode = mode;
}

void
hangul_ic_connect_translate (HangulInputContext* hic,
                             HangulOnTranslate callback,
                             void* user_data)
{
    if (hic != NULL) {
    hic->on_translate      = callback;
    hic->on_translate_data = user_data;
    }
}

void
hangul_ic_connect_transition(HangulInputContext* hic,
                             HangulOnTransition callback,
                             void* user_data)
{
    if (hic != NULL) {
    hic->on_transition      = callback;
    hic->on_transition_data = user_data;
    }
}

void hangul_ic_connect_callback(HangulInputContext* hic, const char* event,
                void* callback, void* user_data)
{
    if (hic == NULL || event == NULL)
    return;

    if (strcasecmp(event, "translate") == 0) {
    hic->on_translate      = (HangulOnTranslate)callback;
    hic->on_translate_data = user_data;
    } else if (strcasecmp(event, "transition") == 0) {
    hic->on_transition      = (HangulOnTransition)callback;
    hic->on_transition_data = user_data;
    }
}

void
hangul_ic_set_keyboard(HangulInputContext *hic, const HangulKeyboard* keyboard)
{
    if (hic == NULL || keyboard == NULL)
    return;

    hic->keyboard = keyboard;
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 의 자판 배열을 바꾸는 함수
 * @param hic @ref HangulInputContext 오브젝트
 * @param id 선택하고자 하는 자판, 아래와 같은 값을 선택할 수 있다.
 *   @li "2"   @ref layout_2 자판
 *   @li "2y"  @ref layout_2y 자판
 *   @li "3f"  @ref layout_3f 자판
 *   @li "39"  @ref layout_390 자판
 *   @li "3s"  @ref layout_3s 자판
 *   @li "3y"  @ref layout_3y 자판
 *   @li "32"  @ref layout_32 자판
 *   @li "ro"  @ref layout_ro 자판
 *
 *   libhangul이 지원하는 자판에 대한 정보는 @ref hangulkeyboards 페이지를
 *   참조하라.
 * @return 없음
 * 
 * 이 함수는 @ref HangulInputContext 의 자판을 @a id로 지정된 것으로 변경한다.
 * 
 * @remarks 이 함수는 @ref HangulInputContext 의 내부 조합 상태에는 영향을
 * 미치지 않는다.  따라서 입력 중간에 자판을 변경하더라도 조합 상태는 유지된다.
 */
void
hangul_ic_select_keyboard(HangulInputContext *hic, const char* id)
{
    const HangulKeyboard* keyboard;

    if (hic == NULL)
    return;

    if (id == NULL)
    id = "2";

    keyboard = hangul_keyboard_list_get_keyboard(id);
    hic->keyboard = keyboard;

    // 3beol
    hic->option_extended_layout_enable = hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_EXTENDED);
    hic->option_galmadeuli_method = hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_GALMADEULI);;

}

void
hangul_ic_set_combination(HangulInputContext *hic,
              const HangulCombination* combination)
{
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 오브젝트를 생성한다.
 * @param keyboard 사용하고자 하는 키보드, 사용 가능한 값에 대해서는
 *    hangul_ic_select_keyboard() 함수 설명을 참조한다.
 * @return 새로 생성된 @ref HangulInputContext 에 대한 포인터
 * 
 * 이 함수는 한글 조합 기능을 제공하는 @ref HangulInputContext 오브젝트를 
 * 생성한다. 생성할때 지정한 자판은 나중에 hangul_ic_select_keyboard() 함수로
 * 다른 자판으로 변경이 가능하다.
 * 더이상 사용하지 않을 때에는 hangul_ic_delete() 함수로 삭제해야 한다.
 */
HangulInputContext*
hangul_ic_new(const char* keyboard)
{
    HangulInputContext *hic;

    hic = malloc(sizeof(HangulInputContext));
    if (hic == NULL)
    return NULL;

    hic->preedit_string[0] = 0;
    hic->commit_string[0] = 0;
    hic->flushed_string[0] = 0;

    hic->on_translate      = NULL;
    hic->on_translate_data = NULL;

    hic->on_transition      = NULL;
    hic->on_transition_data = NULL;

    hic->use_jamo_mode_only = FALSE;

    hic->option_auto_reorder = false;
    hic->option_combi_on_double_stroke = true;
    hic->option_non_choseong_combi = true;

    hic->option_extended_layout_enable = false;
    hic->option_extended_layout_index = 0;
    hic->option_extended_layout_prevkey = 0;
    hic->option_galmadeuli_method = false;

    hic->option_extended_layout_step = 0;


    hangul_ic_set_output_mode(hic, HANGUL_OUTPUT_SYLLABLE);
    hangul_ic_select_keyboard(hic, keyboard);

    hangul_buffer_clear(&hic->buffer);

    return hic;
}

/**
 * @ingroup hangulic
 * @brief @ref HangulInputContext 를 삭제하는 함수
 * @param hic @ref HangulInputContext 오브젝트
 * 
 * @a hic가 가리키는 @ref HangulInputContext 오브젝트의 메모리를 해제한다.
 * hangul_ic_new() 함수로 생성된 모든 @ref HangulInputContext 오브젝트는
 * 이 함수로 메모리해제를 해야 한다.
 * 메모리 해제 과정에서 상태 변화는 일어나지 않으므로 마지막 입력된 
 * 조합중이던 내용은 사라지게 된다.
 */
void
hangul_ic_delete(HangulInputContext *hic)
{
    if (hic == NULL)
    return;

    free(hic);
}

/** @deprecated 이 함수 대신 @ref hangul_keyboard_list_get_count 를 사용하라 */
unsigned int
hangul_ic_get_n_keyboards()
{
    return hangul_keyboard_list_get_count();
}

/** @deprecated 이 함수 대신 @ref hangul_keyboard_list_get_keyboard_id 를 사용하라 */
const char*
hangul_ic_get_keyboard_id(unsigned index_)
{
    return hangul_keyboard_list_get_keyboard_id(index_);
}

/** @deprecated 이 함수 대신 @ref hangul_keyboard_list_get_keyboard_name 을 사용하라 */
const char*
hangul_ic_get_keyboard_name(unsigned index_)
{
    return hangul_keyboard_list_get_keyboard_name(index_);
}

/**
 * @ingroup hangulic
 * @brief 주어진 hic가 transliteration method인지 판별
 * @param hic 상태를 알고자 하는 HangulInputContext 포인터
 * @return hic가 transliteration method인 경우 true를 리턴, 아니면 false
 *
 * 이 함수는 @a hic 가 transliteration method인지 판별하는 함수다.
 * 이 함수가 false를 리턴할 경우에는 process 함수에 keycode를 넘기기 전에
 * 키보드 자판 배열에 독립적인 값으로 변환한 후 넘겨야 한다.
 * 그렇지 않으면 유럽어 자판과 한국어 자판을 같이 쓸때 한글 입력이 제대로
 * 되지 않는다.
 */
bool
hangul_ic_is_transliteration(HangulInputContext *hic)
{
    int type;

    if (hic == NULL)
    return false;

    type = hangul_keyboard_get_type(hic->keyboard);
    if (type == HANGUL_KEYBOARD_TYPE_ROMAJA)
    return true;

    return false;
}

/**
 * @ingroup hangulic
 * @breif libhangul을 초기화 하는 함수.
 *
 * libhangul의 함수를 사용하기 전에 호출해야 한다.
 */
int
hangul_init()
{
    int res;
    res = hangul_keyboard_list_init();
    return res;
}

/**
 * @ingroup hangulic
 * @breif libhangul에서 사용한 리소스를 해제하는 함수.
 *
 * libhangul의 함수의 사용이 끝나면 호출해야 한다.
 */
int
hangul_fini()
{
    int res;
    res = hangul_keyboard_list_fini();
    return res;
}



// 3beol
#ifndef libhangul_hangulinputcontext_addon_c
#define libhangul_hangulinputcontext_addon_c
// hangulinputcontext.c 에 있는 함수들의 대부분이 static 라서
// 별도의 .c 가 아닌 .h 로 만들어서
// hangulinputcontext.c 에 끼워넣기로 한다.

unsigned int
hangul_ic_get_keyboard_flag(HangulInputContext *hic)
{
    unsigned int value = 0;
    value += hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_EXTENDED) << 0;
    value += hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_GALMADEULI) << 1;
    value += hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER) << 2;
    value += hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_RIGHT_OU) << 3;
    value += hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT)<< 4;

    return value;
}

void
hangul_ic_set_keyboard_flag(HangulInputContext *hic, bool* value)
{
    //bool flag[5];
    //flag[HANGUL_KEYBOARD_FLAG_EXTENDED] = hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_EXTENDED);
    //flag[HANGUL_KEYBOARD_FLAG_GALMADEULI] = hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_GALMADEULI);
    //flag[HANGUL_KEYBOARD_FLAG_LOOSE_ORDER] = hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER);
    //flag[HANGUL_KEYBOARD_FLAG_RIGHT_OU] = hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_RIGHT_OU);
    //flag[HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT] = hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT);
}
// 확장단계를 한글자소가 아닌 기호로 나타내기위해....
void
hangul_buffer_push_extension_step(HangulBuffer *buffer, ucschar ch)
{
    buffer->choseong = ch;
    buffer->stack[++buffer->index] = ch;
}


bool
hangul_is_extension_condition_sebeol_shin(HangulInputContext *hic)
{
  if (hic == NULL) {
    return false;
  }
    
    ucschar* symbol_value = hangul_keyboard_get_addon_value(hic->keyboard, HANGUL_KEYBOARD_VALUE_SYMBOL);
  if (hic->buffer.choseong == *(symbol_value + 0) && // 첫소리 0x110b (ㅇ) 이 있다
    hic->buffer.jungseong == 0 &&   // 가윗소리가 없다
    hic->option_extended_layout_index == 0) {// 확장모드가 아니다
    return true;
  }
  return false;
}



bool
hangul_is_extension_condition_sebeol(HangulInputContext *hic, int ascii, int max_index)
{
  if (hic == NULL) {
    return false;
  }
  if (hic->option_extended_layout_index < max_index) {
    if (hangul_buffer_is_empty(&(hic->buffer)) ||          // ( 아무것도 없거나
      hangul_buffer_has_jungseong(&(hic->buffer)) || // 가윗소리가 있거나
      hic->option_extended_layout_index != 0) {// 확장모드 일 때
        char* symbol_key = hangul_keyboard_get_addon_key(hic->keyboard, HANGUL_KEYBOARD_KEY_SYMBOL);
      if (*(symbol_key + 0) == '1') {  // 다른 배열을 쓴다
        if (!(hic->option_extended_layout_prevkey) || // 처음 눌렀거나
          (hic->option_extended_layout_prevkey == ascii)) {// 같은 확장 기호 글쇠 누름
          return true;
        }
      }
      else {
        return true;
      }
    }
  }
  return false;
}


void
hangul_is_extension_ready_sebeol(HangulInputContext *hic)
{
  if (hic == NULL) {
    return;
  }

  if (hic->option_extended_layout_index == 0) {// 있던 것을 뿌린다.
      ucschar* symbol_value = hangul_keyboard_get_addon_value(hic->keyboard, HANGUL_KEYBOARD_VALUE_SYMBOL);
    if (symbol_value != NULL) {
      if (hic->buffer.right_oua) {// 가윗소리에 확장글쇠의 겹홀소리가 있으면 지워준다
        hangul_ic_save_commit_string(hic);
        if (hic->buffer.jongseong == 0 &&
          (hic->buffer.jungseong == *(symbol_value + 0) ||
            hic->buffer.jungseong == *(symbol_value + 1))) {
          hic->buffer.jungseong = 0;
        }
      }
    }
    hangul_ic_save_commit_string(hic);
  }
}



static bool
hangul_ic_process_jamo_dubeol(HangulInputContext *hic, ucschar ch)
{
    ucschar jong;
    ucschar combined;
    // ch 가 한글자모가 아닐 경우, 조합하던 글자를 commit_string 에 넣고 ch 를 뒤에 덧붙인다.
    if (!hangul_is_jamo(ch) && ch > 0) {
    hangul_ic_save_commit_string(hic);
    hangul_ic_append_commit_string(hic, ch);
    return true;
    }

    if (hic->buffer.jongseong) {// 끝소리가 있다
    if (hangul_is_choseong(ch)) {// 첫소리가 들어오면
        // 들어온 첫소리를 끝소리로 바꾸어서 있던 끝소리와 조합을 해본다.
        jong = hangul_ic_choseong_to_jongseong(hic, ch);
        combined = hangul_ic_combine(hic, hic->buffer.jongseong, jong);
        if (hangul_is_jongseong(combined)) {// 조합된 것이 끝소리면
        if (!hangul_ic_push(hic, combined)) {// 있던 끝소리 대신 넣는다
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        } else {// 조합된 것이 끝소리가 아니면
        // 조합하던 글자를 commit_string 에 넣고 첫소리 ch 를 넣어 글자조합을 새로 시작한다
        hangul_ic_save_commit_string(hic);
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    } else if (hangul_is_jungseong(ch)) {// 가윗소리가 들어오면
        ucschar pop, peek;
        //stack 에서 마지막에 들어온 것 (끝소리) 을 빼낸다
        pop = hangul_ic_pop(hic);
        // 마지막 것을 빼낸 뒤의 것을 읽어온다
        peek = hangul_ic_peek(hic);
        // 끝소리 ㄺ 이 있었다면 peek : ㄹ, pop : ㄱ,

        if (hangul_is_jongseong(peek)) {// 하나 빼낸 뒤의 것도 끝소리면
        // 끝소리를 앞의 것은 끝소리, 뒤의 것은 첫소리로 나누어 본다.
        // 끝소리 ㄺ 있었을 때, 나누어 진다면 끝소리 ㄱ 을 첫소리 ㄱ 으로 바꾸어 줄 것이다.
        ucschar choseong = hangul_jongseong_get_diff(peek,
                         hic->buffer.jongseong);
        if (choseong == 0) {// 나누어지지 않는다면
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        } else {// 끝소리, 첫소리로 나누어 진다면
            hic->buffer.jongseong = peek;
            hangul_ic_save_commit_string(hic);
            hangul_ic_push(hic, choseong);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        } else {// 하나 빼낸 뒤의 것이 끝소리가 아니면
        hic->buffer.jongseong = 0;
        hangul_ic_save_commit_string(hic);
        // 빼낸 끝소리 pop 을 첫소리로 바꾸어 넣는다
        hangul_ic_push(hic, hangul_jongseong_to_choseong(pop));
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    } else {// 첫소리, 가윗소리는 앞에서 확인했으니 끝소리
        goto flush;
    }
    } else if (hic->buffer.jungseong) {// 끝소리 없이 가윗소리가 있다
    if (hangul_is_choseong(ch)) {// 첫소리가 들어왔다
        if (hic->buffer.choseong) {// 첫소리가 있다
        // 들어온 첫소리를 끝소리로 바꾼다
        jong = hangul_ic_choseong_to_jongseong(hic, ch);
        if (hangul_is_jongseong(jong)) {// 첫소리가 끝소리로 바뀌었다
            if (!hangul_ic_push(hic, jong)) {
            if (!hangul_ic_push(hic, ch)) {
                return false;
            }
            }
        } else {// 첫소리가 끝소리로 바뀌지 않았다
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        } else {// 첫소리가 없다
        if (hic->option_auto_reorder || hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) {
            /* kr 처럼 자모가 역순인 경우 처리 */
            if (!hangul_ic_push(hic, ch)) {
            if (!hangul_ic_push(hic, ch)) {
                return false;
            }
            }
        } else {
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
            return false;
            }
        }
        }
    } else if (hangul_is_jungseong(ch)) {// 가윗소리가 들어왔다
        ucschar compress = 0;
        // 바꿔놓기를 위한 곳이다. <두벌 순아래>, < 북한 국규>
        // 첫소리 + 가윗소리 + 가윗소리 = 된소리 + 가윗소리
        if (hic->buffer.choseong) {// 첫소리가 있으면 된소리로 바꾸어 본다
            if (ch == hangul_ic_peek(hic)) {// 같은 가윗소리가 들어왔다
            compress = hangul_ic_combine(hic, 0x0000, hic->buffer.choseong);
            }

        }

        if (compress) {// 첫소리가 된소리로 바뀌었다
            if (!hangul_ic_push(hic, compress)) {
                 return false;
            }
        } else {
        // 첫소리가 된소리로 바뀌지 않았다
        // 있던 가윗소리와 들어온 가윗소리를 하나로 만들어 본다
        combined = hangul_ic_combine(hic, hic->buffer.jungseong, ch);
        if (hangul_is_jungseong(combined)) {// 가윗소리 둘이 하나가 되었다
        if (!hangul_ic_push(hic, combined)) {
            return false;
        }
        } else {// 가윗소리 둘이 하나가 되지 못했다
        hangul_ic_save_commit_string(hic);
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
            }
    } else {// 남은 것은 끝소리
        goto flush;
    }
    } else if (hic->buffer.choseong) {// 끝소리, 가윗소리 없이 첫소리가 있다
    if (hangul_is_choseong(ch)) {// 첫소리가 들어왔다
        // 있던 첫소리와 하나로 만들어 본다
        combined = hangul_ic_combine(hic, hic->buffer.choseong, ch);
        /* 초성을 입력한 combine 함수에서 종성이 나오게 된다면
         * 이전 초성도 종성으로 바꿔 주는 편이 나머지 처리에 편리하다.
         * 이 기능은 MS IME 호환기능으로 ㄳ을 입력하는데 사용한다. */
        if (hangul_is_jongseong(combined)) {
        hic->buffer.choseong = 0;
        ucschar pop = hangul_ic_pop(hic);
        ucschar jong = hangul_choseong_to_jongseong(pop);
        hangul_ic_push(hic, jong);
        }

        if (!hangul_ic_push(hic, combined)) {// 첫소리 둘이 하나가 되었다
        if (!hangul_ic_push(hic, ch)) {// 첫소리 둘이 하나가 되지 못했으니 그냥 넣는다
            return false;
        }
        }
    } else {// 나머지 가윗소리, 끝소리는 없으니 그냥 넣는다
        if (!hangul_ic_push(hic, ch)) {
        if (!hangul_ic_push(hic, ch)) {
            return false;
        }
        }
    }
    } else {// 아무것도 없다. 그냥 넣는다
    if (!hangul_ic_push(hic, ch)) {
        return false;
    }
    }

    hangul_ic_save_preedit_string(hic);
    return true;

flush:
    hangul_ic_flush_internal(hic);
    return false;
}


static bool
hangul_ic_process_jaso_shin_sebeol (HangulInputContext *hic, int ascii, ucschar ch)
{
    if (hic->option_extended_layout_enable) 
    {
        const int symbol_key = hangul_keyboard_is_extension_key (hic->keyboard, ascii, HANGUL_KEYBOARD_KEY_SYMBOL);
        if (symbol_key) 
        {// 확장글쇠가 들어왔다
            if (hangul_is_extension_condition_sebeol_shin(hic)) 
            {// 조건을 만족한다
                if (hic->option_extended_layout_index == 0) 
                {// 첫소리 0x110b (ㅇ) 이 있던 것을 지우고 뿌린다.
                    hic->buffer.choseong = 0;
                    hangul_ic_save_commit_string(hic);
                }
                // 신세벌식은 단계별 글쇠가 따로 있기 때문에 [1, 2, 3] 대신 [11, 12, 13] 을 쓴다.
                // bool hangul_ic_backspace(HangulInputContext *hic)
                switch (symbol_key) 
                {
                    case 1 :
                    case 2 :
                    case 3 :
                        hic->option_extended_layout_index =  symbol_key + 10;

                        ucschar* step_value = hangul_keyboard_get_addon_value(hic->keyboard, HANGUL_KEYBOARD_VALUE_EXTENDED_STEP);
                        if (step_value != NULL) 
                        {
                            hangul_buffer_push_extension_step(&hic->buffer, *(step_value + symbol_key));
                        }
                        break;
                    default :
                        hangul_buffer_clear(&hic->buffer);
                        hangul_ic_save_commit_string(hic);
                        break;
                }

                hangul_ic_save_preedit_string(hic);
                return true;
            }
        }

        if (hic->option_extended_layout_index >= 11 && hic->option_extended_layout_index <= 13) {
            //index = [ #1, #2, #3 / 11, 12, 13 ]   // 확장 기호를 다룬다.
            if (ch > 0) {
                ucschar extended = 0;
                const ucschar(*addon_func)(int, int, int) = hangul_keyboard_get_addon_func(hic->keyboard, HANGUL_FUNCTION_SYMBOL);
                if (addon_func != NULL) {
                    extended = addon_func(ascii, hic->option_extended_layout_index - 10, 0);
                }

                hangul_buffer_clear(&hic->buffer);//확장단계 표시 첫소리를 없앤다
                hangul_ic_save_commit_string(hic);

                if (extended) 
                {// 확장기호가 있는 글쇠
                    hangul_ic_append_commit_string(hic, extended);
                }
                hangul_ic_save_preedit_string(hic);

                return true;
            } else {
                hangul_ic_save_commit_string(hic);
                
                return false;
            }
        }
    }


    // 아래아
    if (ascii == '[') {
        //첫소리가 있고 끝소리가 없으며, 가윗소리가 없거나 아래아 일 때
        if (hic->buffer.choseong && hic->buffer.jongseong == 0) {
          ucschar replace_it = hangul_keyboard_get_replace_it(hic->keyboard);
          if (replace_it) {// [ 는 아래 아(ㆍ)로 구실한다
              if ((hic->buffer.jungseong == 0) || (hic->buffer.jungseong == replace_it)) 
              {
                  ch = replace_it;// 아래아
                  hic->buffer.right_oua = replace_it;
              }
          }

        }
    }

    if (hangul_is_choseong(ch)) {
        if (hic->buffer.choseong == 0) {//첫소리 없고, 들어온 첫소리를 첫소리로 다룬다.
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        } else {//첫소리 있고, 첫소리가 들어왔다.
            if (hic->buffer.jungseong == 0) {// 가윗소리가 없다
                // 갈마들이를 먼저해보고 첫소리 조합으로 간다
                ucschar jungseong = hangul_ic_combine(hic, 0x0000, ch);

                if (hangul_is_jungseong(jungseong)) {// 갈마들이 가윗소리 ㅜ, ㅗ, 아래아 가 겹홀소리에 쓰인다.
                    if (jungseong != 0x1174) {// 0x1174 (ㅢ) 는 뺀다.
                        hic->buffer.right_oua = jungseong;
                    }
                    if (!hangul_ic_push(hic, jungseong)) {
                        return false;
                    }
                } else {// 첫소리를 하나로 만들어 본다
                    ucschar choseong = 0;
                    if (hangul_is_choseong(hangul_ic_peek(hic))) {
                        choseong = hangul_ic_combine(hic, hic->buffer.choseong, ch);
                    }
                    if (hangul_is_choseong(choseong)) {// 첫소리가 하나가 되었다
                        if (!hangul_ic_push(hic, choseong)) {
                            return false;
                        }
                    } else {// 첫소리가 하나가 되지 못 했다
                        hangul_ic_save_commit_string(hic);
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            } else {//가윗소리가 있으니 첫소리로
                hangul_ic_save_commit_string(hic);
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        }
    } else if (hangul_is_jungseong(ch)) {
        if (hic->buffer.jungseong == 0) {//가윗소리가 없다.
            if (hic->buffer.jongseong == 0) {// 끝소리 없다. 첫소리가 있으나 없으나 가윗소리로
                //if (hic->buffer.choseong == 0) {
                    // 첫소리가 없으면 왼쪽 ㅗ·ㅜ로도 ㅘ·ㅙ·ㅚ·ㅝ·ㅞ·ㅟ를 조합할 수 있다.
                    // (왼쪽 ㅗ·ㅜ와 오른쪽 ㅗ·ㅜ의 동작이 같음)
                //    hic->buffer.right_oua = ch;
                //} else
                if (hangul_keyboard_is_right_oua (hic->keyboard, ascii, ch, HANGUL_KEYBOARD_KEY_MOEUM)) {//shift+첫소리 [ᅟㅗ, ㅜ]
                    hic->buffer.right_oua = ch;
                }//shift+ㅁ [아래아] 는 홑홀소리
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            } else {// 끝소리 있다
                if (hic->buffer.choseong == 0) {//첫소리가 없다.
                    if (hangul_keyboard_is_right_oua(hic->keyboard, ascii, ch, HANGUL_KEYBOARD_KEY_MOEUM)) {//shift+첫소리 [ㅗㅜ]
                        // 새로운 글자로
                        hangul_ic_save_commit_string(hic);
                        hic->buffer.right_oua = ch;
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    } else { // 끝소리만 있다. 갈마들이로 가보자
                            //초성체에서 겹닿소리만 넣을 때는 끝소리를 아무거나 넣고 shift+끝글쇠[홀소리]로 넣는다.
                            // 가윗소리를 끝소리로 바꾼다
                            ucschar jongseong = hangul_ic_combine(hic, 0x0000, ch);

                            if (hangul_is_jongseong(jongseong)) {// shift+글쇠에 겹받침이 있으면 끝소리로 다룬다. 초성체 겹닿소리
                                if (hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT) == TRUE) {// 겹받침 확장이 없다
                                    ucschar jong_conjoin = 0;
                                    jong_conjoin = hangul_ic_combine(hic, hic->buffer.jongseong, jongseong);

                                    if (hangul_is_jongseong(jong_conjoin)) { // 겹받침이 되었다
                                        if (!hangul_ic_push(hic, jong_conjoin)) {
                                            return false;
                                        }
                                    } else {// 겹받침이 되지 못 했으니 가윗소리로 다룬다
                                        hangul_ic_save_commit_string(hic);
                                        if (!hangul_ic_push(hic, ch)) {
                                            return false;
                                        }
                                    }
                                } else { // 겹받침 확장 배열의 겹받침이다
                                    if (!hangul_ic_push(hic, jongseong)) {
                                        return false;
                                    }
                                }
                            } else {// 갈마들이 겹닿소리가 없다, 새로운 글자로
                                hangul_ic_save_commit_string(hic);
                                if (!hangul_ic_push(hic, ch)) {
                                    return false;
                                }
                            }
                    }
                } else {// 첫소리, 끝소리가 있다. 새로운 글자로
                    //Shift + 글쇠인 가윗소리를 넣는다
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            }
        } else {// 가윗소리가 있다
            if (hic->buffer.jongseong == 0) {// 끝소리가 없으니
                //가윗소리가 있을 때 shift+첫소리의 가윗소리가 들어오면 무조건 새로운 글자로
                if (hangul_keyboard_is_right_oua (hic->keyboard, ascii, ch, HANGUL_KEYBOARD_VALUE_MOEUM)) {
                    hangul_ic_save_commit_string(hic);
                    hic->buffer.right_oua = ch;
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                } else {
                    if (hic->buffer.right_oua || hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT))
                    {// 가윗소리 조합
                        ucschar jungseong = 0;
                        if (hangul_is_jungseong(hangul_ic_peek(hic))) {
                            jungseong = hangul_ic_combine(hic, hic->buffer.jungseong, ch);
                        }
                        if (hangul_is_jungseong(jungseong)) {// 조합된 가윗소리
                            if (!hangul_ic_push(hic, jungseong)) {
                                return false;
                            }
                        } else {// 조합이 되지 않으니 새로운 글자로
                            hangul_ic_save_commit_string(hic);
                            if (!hangul_ic_push(hic, ch)) {
                                return false;
                            }
                        }
                    } else {// 조합이 되지 않는 가윗소리가 있으니, 갈마들이 받침으로 해보자
                            // 가윗소리를 갈마들이 끝소리로 바꾼다
                        ucschar jongseong = hangul_ic_combine(hic, 0x0000, ch);

                        if (jongseong &&
                            hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT) == FALSE)
                        {// shift+끝소리에 겹받침이 있으면 겹받침으로 다룬다.
                            if (!hangul_ic_push(hic, jongseong)) {
                                return false;
                            }
                        } else {// shift+글쇠의 갈마들이 겹닿소리가 없으니, 새로운 글자의 가윗소리로
                            hangul_ic_save_commit_string(hic);
                            if (!hangul_ic_push(hic, ch)) {
                                return false;
                            }
                        }
                    }
                }
            } else {// 가윗소리, 끝소리가 있다
                hangul_ic_save_commit_string(hic);
                if (hangul_keyboard_is_right_oua (hic->keyboard, ascii, ch, HANGUL_KEYBOARD_VALUE_MOEUM)) {//shift+첫소리
                    hic->buffer.right_oua = ch;
                }
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        }
    } else if (hangul_is_jongseong(ch)) {
        if (hic->buffer.jungseong == 0) {// 가윗소리가 없다.
            if (hic->buffer.choseong == 0) {//첫소리가 없다.
                if (hic->buffer.jongseong == 0) {//끝소리가 없다.
                   if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                } else {//끝소리가 있다. // 초성체를 위해
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            } else {//첫소리가 있다.
                ucschar jungseong = hangul_ic_combine(hic, 0x0000, ch);

                if (hangul_is_jungseong(jungseong)) {//가윗소리가 있는 글쇠는 가윗소리로.
                    if (!hangul_ic_push(hic, jungseong)) {
                        return false;
                    }
                } else {
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            }
        } else {//가윗소리가 있다.
            if (hic->buffer.jongseong == 0) {//끝소리가 없다.
                // 먼저 끝소리를 가윗소리로 바꾸어 가윗소리 조합을 해본다.
                if (hangul_is_jungseong(hangul_ic_peek(hic)) && hic->buffer.right_oua) {
                    ucschar jungseong = 0;
                    ucschar jong_to_jung = hangul_ic_combine(hic, 0x0000, ch);

                    if (jong_to_jung) {
                        jungseong = hangul_ic_combine(hic, hic->buffer.jungseong, jong_to_jung);
                    }
                    if (hangul_is_jungseong(jungseong)) {// 끝소리를 가윗소리로 바꾸어 가윗소리 조합이 되었다
                        if (!hangul_ic_push(hic, jungseong)) {
                            return false;
                        }
                    } else {// 끝소리로 넣는다
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                } else { // 끝소리를 가윗소리로 바꾸지 않고 끝소리로 다룬다
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            } else {//끝소리가 있다.
                ucschar jongseong = 0;
                if (hangul_is_jongseong(hangul_ic_peek(hic))) {
                    if (hic->buffer.jongseong == ch) {//두 번 누를 때 겹받침으로 바꾼다.

                            ucschar jong_to_jung = 0;
                            // 끝소리를 가윗소리로 바꾼 뒤에 그 가윗소리를 다시 겹받침으로 바꾼다.
                            jong_to_jung = hangul_ic_combine(hic, 0x0000, ch);
                            if (jong_to_jung) {
                                jongseong = hangul_ic_combine(hic, 0x0000, jong_to_jung);
                            }

                    }
                    if (jongseong == 0 || hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT) == TRUE) {
                        //끝소리가 다르거나 두 번 누를 때 겹받침으로 바꾸지 못하면 종성결합 규칙으로.
                        jongseong = hangul_ic_combine(hic, hic->buffer.jongseong, ch);
                    }
                }
                if (hangul_is_jongseong(jongseong)) { // 겹받침이 되었다
                    if (!hangul_ic_push(hic, jongseong)) {
                        return false;
                    }
                } else {
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            }
        }
    } else if (ch > 0) {
        if (hic->keyboard != NULL &&
                ( (hic->buffer.jungseong != 0 && hic->buffer.jongseong == 0) ||
                 (hic->buffer.choseong == 0 && hic->buffer.jungseong == 0 && hic->buffer.jongseong != 0) ) &&
              ascii == 'Z' && ch == 0x203B) {//※: 0x203B
            // 신세벌 2003 의 ㅁ (※) 의 윗글쇠에  홀소리가 없어서 shift + ㅁ = ㄻ (0x11b1) 은 여기서 다룬다.
            // 나머지는 hangul_shin_jungseong_to_conjoin_jongseong 에서 다룬다.
            ch = 0x11b1; /* ㄻ */
            if (!hangul_ic_push(hic, ch)) {
                return false;
            }
        } else {
            hangul_ic_save_commit_string(hic);
            hangul_ic_append_commit_string(hic, ch);
        }
    } else {
        hangul_ic_save_commit_string(hic);
        return false;
    }

    hangul_ic_save_preedit_string(hic);
    return true;
}


static bool
hangul_ic_process_3finalsun (HangulInputContext *hic, int ascii, ucschar ch)
{
    #define SHKEY 0x11ff
    ucschar orig_ch = ch;

    if (ascii == '[') {//
        if (hangul_buffer_is_empty (&hic->buffer) == FALSE) {
            if (hic->buffer.shift == 0) {
                ch = SHKEY;
            }
        }
    }

    if (hangul_is_choseong(ch)) {
        if (hic->buffer.choseong == 0) {
            if (hic->option_auto_reorder || hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) {
                if (!hangul_ic_push(hic, ch)) {
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            } else {
                if (hangul_ic_has_jungseong(hic) || hangul_ic_has_jongseong(hic)) {
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                } else {
                    if (!hangul_ic_push(hic, ch)) {
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            }
        } else {
            ucschar choseong = 0;
            if (hangul_is_choseong(hangul_ic_peek(hic))) {
                choseong = hangul_ic_combine(hic, hic->buffer.choseong, ch);
            }
            if (choseong) {
                if (!hangul_ic_push(hic, choseong)) {
                    if (!hangul_ic_push(hic, choseong)) {
                        return false;
                    }
                }
            } else {
                hangul_ic_save_commit_string(hic);
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        }
    } else if (hangul_is_jungseong(ch)) {
        if (hic->buffer.jungseong == 0) {
            // 가윗소리가 없으면 들어온 가윗소리를 넣는다
            if (hic->option_auto_reorder || hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) {
                if (!hangul_ic_push(hic, ch)) {
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            } else {
                if (hangul_ic_has_jongseong(hic)) {
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                } else {
                    if (!hangul_ic_push(hic, ch)) {
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            }
            if (hic->buffer.jongseong == 0) {//
                if (hic->buffer.shift) {
                // 끝소리가 없고 종성시프트가 있다면 기윗소리에 놓인 겹받침을 넣는다
                    ucschar jongseong = 0;
                    jongseong = hangul_ic_combine(hic, hic->buffer.shift, ch);
                                        
                    if (jongseong) {
                        hic->buffer.shift = 0;
                        if (!hangul_ic_push(hic, jongseong)) {
                            if (!hangul_ic_push(hic, jongseong)) {
                                return false;
                            }
                        }
                    }
                }
            }
        } else {// 들어온 가윗소리와 조합을 해본다
            ucschar jungseong = 0;
            jungseong = hangul_ic_combine(hic, hic->buffer.jungseong, ch);
            
            if (jungseong) {
                if (!hangul_ic_push(hic, jungseong)) {
                    if (!hangul_ic_push(hic, jungseong)) {
                        return false;
                    }
                }
            } else {
            // 가윗소리 조합이 되지 않고 종성시프트가 있다면 가윗소리에 놓인 겹받침으로 바꿔본다
                if (hic->buffer.jongseong == 0) {//
                    if (hic->buffer.shift) {
                        ucschar jongseong = 0;
                        jongseong = hangul_ic_combine(hic, hic->buffer.shift, ch);
                                                
                        if (jongseong) {
                            if (!hangul_ic_push(hic, jongseong)) {
                                if (!hangul_ic_push(hic, jongseong)) {
                                    return false;
                                }
                            }
                        } else {
                            hangul_ic_save_commit_string(hic);
                            if (!hangul_ic_push(hic, ch)) {
                                if (!hangul_ic_push(hic, ch)) {
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (hangul_is_jongseong(ch)) {
        if (ch == SHKEY) {// 종성시프트가 눌렸다면
            if (hic->buffer.jongseong == 0) {// 끝소리가 없을 때는 있는 가윗소리에 놓인 겹받침을 넣는다
                ucschar jongseong = 0;
                jongseong = hangul_ic_combine(hic, ch, hic->buffer.jungseong);
                
                if (jongseong) {
                    if (!hangul_ic_push(hic, jongseong)) {
                        if (!hangul_ic_push(hic, jongseong)) {
                            return false;
                        }
                    }
                } else {
                    hic->buffer.shift = ch;
                }
            } else {// 끝소리가 있으면 겹받침으로 바꾼다
                ucschar jongseong = 0;
                jongseong = hangul_ic_combine(hic, ch, hic->buffer.jongseong);
                
                if (jongseong) {
                    if (!hangul_ic_push(hic, jongseong)) {
                        if (!hangul_ic_push(hic, jongseong)) {
                            return false;
                        }
                    }
                } else {
                    hangul_ic_save_commit_string(hic);
                    hangul_ic_append_commit_string(hic, orig_ch);
                }
            }
        } else {
            if (hic->buffer.shift) {// 종성시프트가 있으면 끝소리는 겹받침이 된다
                if (hic->buffer.jongseong == 0) {
                    ucschar jongseong = 0;
                    jongseong = hangul_ic_combine(hic, hic->buffer.shift, ch);
                    if (jongseong) {
                        if (!hangul_ic_push(hic, jongseong)) {
                            if (!hangul_ic_push(hic, jongseong)) {
                                return false;
                            }
                        }
                    } else {
                        hangul_ic_save_commit_string(hic);
                        if (!hangul_ic_push(hic, ch)) {
                            if (!hangul_ic_push(hic, ch)) {
                                return false;
                            }
                        }
                    }
                } else {
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            } else {
                if (hic->buffer.jongseong == 0) {
                    if (!hangul_ic_push(hic, ch)) {
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                } else {
                    ucschar jongseong = 0;
                    if (hangul_is_jongseong(hangul_ic_peek(hic))) {
                        jongseong = hangul_ic_combine(hic, hic->buffer.jongseong, ch);
                    }
                    if (jongseong) {
                        if (!hangul_ic_push(hic, jongseong)) {
                            if (!hangul_ic_push(hic, jongseong)) {
                                return false;
                            }
                        }
                    } else {
                        hangul_ic_save_commit_string(hic);
                        if (!hangul_ic_push(hic, ch)) {
                            if (!hangul_ic_push(hic, ch)) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    } else if (ch > 0) {
        hangul_ic_save_commit_string(hic);
        hangul_ic_append_commit_string(hic, ch);
    } else {
        hangul_ic_save_commit_string(hic);
        return false;
    }

    hangul_ic_save_preedit_string(hic);
    return true;
}

static bool
hangul_ic_process_jaso_sebeol (HangulInputContext *hic, int ascii, ucschar ch)
{
    if (hic->option_extended_layout_enable) {
        int symbol_key = 0;
        int yetgeul_key = 0;

        symbol_key = hangul_keyboard_is_extension_key (hic->keyboard, ascii, HANGUL_KEYBOARD_KEY_SYMBOL);
        if (symbol_key) {
            if (symbol_key > 20) {// 기호 배열로 들어가기 위한 준비글쇠가 있다.
                if (hic->option_extended_layout_prevkey) {
                    if (hic->option_extended_layout_index) {
                        ;
                    } else {
                        // 확장 단계를 올린다
                        hic->option_extended_layout_index = symbol_key;
                        ucschar* step = hangul_keyboard_get_addon_value(hic->keyboard, HANGUL_KEYBOARD_VALUE_EXTENDED_STEP);
                        if (step != NULL) {// 확장 기호 단계가 있다
                            hangul_buffer_push_extension_step(&hic->buffer, *(step + (hic->option_extended_layout_index - 21)));
                        }
                        hangul_ic_save_preedit_string(hic);
                        return true;
                    }
                } else if (symbol_key == 21) { // 기호 배열을 준비한다
                    hangul_ic_save_commit_string(hic);

                    hic->option_extended_layout_prevkey = ascii;
                    ucschar* step = hangul_keyboard_get_addon_value(hic->keyboard, HANGUL_KEYBOARD_VALUE_EXTENDED_STEP);
                    if (step != NULL) {// 확장 기호 단계가 있다
                            hangul_buffer_push_extension_step(&hic->buffer, *(step + 0));
                    }
                    hangul_ic_save_preedit_string(hic);
                    return true;
                }
            } else if (symbol_key > 10) {// 기호 글쇠가 각각 다른 배열을 쓴다
                if (hangul_is_extension_condition_sebeol(hic, ascii, 5)) {// 조건을 만족한다
                    // 있던 것을 뿌린다
                    hangul_is_extension_ready_sebeol(hic);

                    // 확장 단계를 올린다
                    hic->option_extended_layout_index += 1;
                    hic->option_extended_layout_prevkey = ascii;

                    switch (hic->option_extended_layout_index) {
                        case 1 :
                        case 2 :
                        case 3 :
                        case 4 :
                        case 5 :
                        {
                            const ucschar* step = hangul_keyboard_get_addon_value(hic->keyboard, HANGUL_KEYBOARD_VALUE_EXTENDED_STEP);
                            if (step != NULL) {// 확장 기호 단계가 있다
                                hangul_buffer_push_extension_step(&hic->buffer, *(step + (hic->option_extended_layout_index)));
                            }
                            break;
                        }
                        default :
                            hangul_buffer_clear(&hic->buffer);
                            hangul_ic_save_commit_string(hic);
                            break;
                    }
                    hangul_ic_save_preedit_string(hic);
                    return true;
                }
            } else {// 기호 글쇠가 같은 배열을 쓴다
                do { // 기호글쇠에 배정된 기호를 넣위해서 쓴다
                    if (hangul_is_extension_condition_sebeol(hic, ascii, 3)) {// 조건을 만족한다
                        // 있던 것을 뿌린다
                        hangul_is_extension_ready_sebeol(hic);

                        //앞에서와 다른 확장글쇠면 1단에서 바로 3단으로 뛴다.
                        if (hic->option_extended_layout_prevkey && hic->option_extended_layout_prevkey != ascii) {
                            if (hic->option_extended_layout_index > 1) {
                                break;
                            } else {
                                hic->option_extended_layout_index += 2;
                            }
                        } else {
                            hic->option_extended_layout_index += 1;
                        }
                        hic->option_extended_layout_prevkey = ascii;

                        switch (hic->option_extended_layout_index) {
                            case 1 :
                            case 2 :
                            case 3 :
                            {
                                ucschar* step = hangul_keyboard_get_addon_value(hic->keyboard, HANGUL_KEYBOARD_VALUE_EXTENDED_STEP);
                                if (step != NULL) {// 확장 기호 단계가 있다
                                    hangul_buffer_push_extension_step(&hic->buffer, *(step +( hic->option_extended_layout_index)));
                                }
                                break;
                            }
                            default :
                                hangul_buffer_clear(&hic->buffer);
                                hangul_ic_save_commit_string(hic);
                                break;
                        }

                        hangul_ic_save_preedit_string(hic);
                        return true;
                    }
                } while (false);
            }
        } else {
            yetgeul_key = hangul_keyboard_is_extension_key (hic->keyboard, ascii, HANGUL_KEYBOARD_KEY_YETGEUL);
            if (yetgeul_key) {// 확장 한글 글쇠 누름
                // 여기는 한글조합에 쓰이기 때문에 있던 것을 뿌리지 않는다
                if (hic->option_extended_layout_index == 0) {
                    hic->option_extended_layout_prevkey = ascii;
                } else if (hic->option_extended_layout_prevkey != ascii) {
                    // 앞에서와 다른 한글확장 글쇠가 들어와서 조합을 끝낸다
                    hangul_ic_save_commit_string(hic);
                    return true;
                }

                // 확장 단계를 올린다
                if (hic->option_extended_layout_index < 2) {
                    hic->option_extended_layout_index += 1;
                } else {
                    hic->option_extended_layout_index = 0;
                }

                // 여기는 한글조합에 쓰이기 때문에 단계를 나타내지 않는다

                hangul_ic_save_preedit_string(hic);
                return true;
            }
        }

        if ( (hic->option_extended_layout_index >= 1 && hic->option_extended_layout_index <= 5) || //index = [ 1, 2, 3, 4, 5 ]   // 세벌식 확장 기호를 다룬다.
                (hic->option_extended_layout_index >= 22 && hic->option_extended_layout_index <= 24) ) { //index = [ 22, 23, 24 ]   // 세벌식 세모이 확장 기호를 다룬다.
            if (ch > 0) {
                ucschar extended = 0;
                int index = 0;

                symbol_key = hangul_keyboard_is_extension_key (hic->keyboard, hic->option_extended_layout_prevkey, HANGUL_KEYBOARD_KEY_SYMBOL);
                if (symbol_key) {
                    const ucschar(*addon_func)(int, int, int) = hangul_keyboard_get_addon_func(hic->keyboard, HANGUL_FUNCTION_SYMBOL);
                    if (addon_func != NULL) {
                        if (symbol_key > 20) {
                            index = hic->option_extended_layout_index - 21;
                        } else if (symbol_key > 10) {
                            symbol_key -= 11;
                            index = hic->option_extended_layout_index;
                        } else {
                            symbol_key -= 1;
                            index = hic->option_extended_layout_index;
                        }
                        extended = addon_func(ascii, index, symbol_key);
                    }
                    hangul_buffer_clear(&hic->buffer);//확장단계 표시 첫소리를 없앤다
                    hangul_ic_save_commit_string(hic);
                    if (extended) {// 확장기호가 있는 글쇠
                        hangul_ic_append_commit_string(hic, extended);
                    }
                    hangul_ic_save_preedit_string(hic);
                    return true;
                } else {
                    yetgeul_key = hangul_keyboard_is_extension_key (hic->keyboard, hic->option_extended_layout_prevkey, HANGUL_KEYBOARD_KEY_YETGEUL);
                    if (yetgeul_key) {
                        index = hic->option_extended_layout_index;
                        const ucschar(*addon_func)(int, int, int) = hangul_keyboard_get_addon_func(hic->keyboard, HANGUL_FUNCTION_YETHANGEUL);
                        if (addon_func != NULL) {
                            extended = addon_func(ascii, index, yetgeul_key - 1);
                        }
                        // 한글확장에서는 버퍼 청소를 하지 않으니 여기서 확장단계를 없애준다
                        hic->option_extended_layout_index = 0;
                        if (extended) {
                            ch = extended;
                        } else {// 참을 돌려줘야 값이 없는 글쇠를 눌렀을 때 [ 알파벳 ]이 들어가지 않는다
                            hangul_ic_save_commit_string(hic);
                            return true;
                        }
                    } else {
                        hangul_ic_save_commit_string(hic);
                        return false;
                    }
                }
            } else {
                hangul_ic_save_commit_string(hic);
                return false;
            }
        } else if (hic->option_extended_layout_prevkey) {
            hangul_buffer_clear(&hic->buffer);//확장단계 표시를 없앤다
            hangul_ic_save_commit_string(hic);
        }
    }

    // 아래아
    if (ascii == '[') {
        //첫소리가 있고 끝소리가 없으며, 가윗소리가 없거나 아래아 일 때
        if (hic->buffer.choseong && hic->buffer.jongseong == 0) {
            ucschar replace_it = hangul_keyboard_get_replace_it(hic->keyboard);
            if (replace_it) {// [ 는 아래 아(ㆍ)로 구실한다
                if ((hic->buffer.jungseong == 0) || (hic->buffer.jungseong == replace_it)) {
                    ch = replace_it;// 아래아
                    //hic->buffer.right_oua = replace_it;
                }
            }
        }
    }

    if (hangul_is_choseong(ch)) {// 첫소리가 들어왔다
        if (hic->buffer.choseong == 0) {// 첫소리가 없다
            if (hic->option_auto_reorder || hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) {
                if (!hangul_ic_push(hic, ch)) {
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            } else {
                if (hangul_ic_has_jungseong(hic) || hangul_ic_has_jongseong(hic)) {
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                } else {
                    if (!hangul_ic_push(hic, ch)) {
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            }
        } else {// 첫소리가 있다
            ucschar choseong = 0;
            bool combine = false;
            if (hangul_is_choseong(hangul_ic_peek(hic))) {
                // 바로 앞의 것이 첫소리면 하나로 만들어 본다
                combine = true;
            } else if (hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) {
                combine = true;
            }
            if (combine) {
                choseong = hangul_ic_combine (hic, hic->buffer.choseong ,ch);
            }
            if (hangul_is_choseong(choseong)) {// 하나로 되었다
                if (!hangul_ic_push(hic, choseong)) {
                    if (!hangul_ic_push(hic, choseong)) {
                        return false;
                    }
                }
            } else {// 하나로 되지 못했다
                hangul_ic_save_commit_string(hic);
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        }
    } else if (hangul_is_jungseong(ch)) {// 가윗소리가 들어왔다
        if (hic->buffer.jungseong == 0) {// 가윗소리가 없다
            // 겹홀소리에 쓰이는 ㅗ, ㅜ 가 들어왔다
            if (hangul_keyboard_is_right_oua (hic->keyboard, ascii, ch, HANGUL_KEYBOARD_KEY_MOEUM)) {
                hic->buffer.right_oua = ch;
            } else if (ascii == '[' && ch == hangul_keyboard_get_replace_it(hic->keyboard)) {
                // [ 를 아래아로 바꾸었다
                hic->buffer.right_oua = ch;
            }

            if (hic->option_auto_reorder || hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) {
                if (!hangul_ic_push(hic, ch)) {
                    if (!hangul_ic_push(hic, ch)) {
                    return false;
                    }
                }
            } else {
                if (hangul_ic_has_jongseong(hic)) {
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                } else {
                        if (!hangul_ic_push(hic, ch)) {
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            }
        } else {// 가윗소리가 있으니 먼저 가윗소리 조합을 해보고 안 되면 갈마들이로 간다
            ucschar jungseong = 0;
            bool combine = false;
            bool new_start = false;

            if (hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_RIGHT_OU)) {
                // 왼/오른 ㅗ, ㅜ 를 반드시 구분해야 하는 글판 (갈마들이 글판)
                // 가윗소리가 있고 겹홀소리의 ㅗ, ㅜ 가 들어왔다
                if (hangul_keyboard_is_right_oua (hic->keyboard, ascii, ch, HANGUL_KEYBOARD_KEY_MOEUM)) {
                    new_start = true;
                    combine = false;
                } else if (hic->buffer.right_oua || (hic->buffer.choseong == 0) ||
                                (hic->option_galmadeuli_method == FALSE) ) {
                    combine = hangul_is_jungseong(hangul_ic_peek(hic));
                } else if (hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) {
                    // 입력 순서를 따지지 않는다, 모아치기 2016
                    combine = true;
                } else {
                    combine = false;
                }
            } else {
                if (hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) {
                    // 입력 순서를 따지지 않는다, 모아치기 2014
                    combine = true;
                } else {
                    //가윗소리 조합은 바로 앞에서 홀소리를 넣었을 때만 한다.
                    combine = hangul_is_jungseong(hangul_ic_peek(hic));
                }
            }

            if (new_start) {// 새로운 글자를 시작한다
                // 왼/오른 ㅗㅜ 를 구분하는 글판에서 가윗소리가 있을 때 오른 ㅗㅜ 라면 새로운 글자가 된다
                hangul_ic_save_commit_string(hic);
                hic->buffer.right_oua = ch;
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            } else {
                if (combine) {// 가윗소리 조합이 된다면, 가윗소리 둘을 하나로 만들어 본다
                    jungseong = hangul_ic_combine (hic, hic->buffer.jungseong, ch);
                }
                if (hangul_is_jungseong(jungseong)) {// 둘이 하나가 되었다
                    if (!hangul_ic_push(hic, jungseong)) {
                        if (!hangul_ic_push(hic, jungseong)) {
                            return false;
                        }
                    }
                } else {// 둘이 하나가 되지 못했다. 갈마들이로 해보자
                    ucschar jung_jongseong = 0;
                    if (hic->option_galmadeuli_method) {//  새로운 글자가 아니면 갈마들이를 해본다
                        if (hic->buffer.choseong) {// 첫소리, 가윗소리가 있다
                            jung_jongseong = hangul_ic_combine(hic, 0, ch);
                        }
                    }
                    if (jung_jongseong) {// 갈마들이에 소리가 있다
                        if (hangul_is_jungseong(jung_jongseong)) {// 가윗소리다
                            // ㅣ+ㅐ, ㅐ+ㅐ
                            if (hangul_is_jungseong(hangul_ic_peek(hic)) && // 바로 앞에서 가윗소리를 눌렀다
                                    ( (hic->buffer.jungseong == 0x1175) ||  (hic->buffer.jungseong == ch) ) ) {
                                if (!hangul_ic_push(hic, jung_jongseong)) {
                                    if (!hangul_ic_push(hic, jung_jongseong)) {
                                        return false;
                                    }
                                }
                            } else {// 가윗소리 조합이 안 되고 갈마들이 가윗소리로 바꾸지도 못 했다
                                hangul_ic_save_commit_string(hic);
                                if (!hangul_ic_push(hic, ch)) {
                                    if (!hangul_ic_push(hic, ch)) {
                                        return false;
                                    }
                                }
                            }
                        } else {// 갈마들이 끝소리다
                            if (hic->buffer.jongseong == 0) {
                                if (!hangul_ic_push(hic, jung_jongseong)) {
                                    if (!hangul_ic_push(hic, jung_jongseong)) {
                                        return false;
                                    }
                                }
                            } else {
                                ucschar jongseong = 0;
                                combine = false;
                                if (hangul_is_jongseong(hangul_ic_peek(hic))) {
                                    combine = true;
                                } else if (hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) {
                                    combine = true;
                                }
                                if (combine) {
                                    jongseong = hangul_ic_combine(hic, hic->buffer.jongseong, ch);
                                    if (jongseong == 0) {
                                        // 끝소리 조합이 안 되고 끝홑닿소리와 갈마들이 끝홑닿소리가 같으면, 갈마들이 연타 겹받침으로
                                        if (hic->option_galmadeuli_method) {
                                            if (hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT) == FALSE) {
                                                if (hic->buffer.jongseong == jung_jongseong) {
                                                    jongseong = hangul_ic_combine(hic, 0, jung_jongseong);
                                                }
                                            }
                                        }
                                    }
                                }
                                if (hangul_is_jongseong(jongseong)) {// 하나가 되었다
                                    if (!hangul_ic_push(hic, jongseong)) {
                                        return false;
                                    }
                                } else {//끝겹닿소리가 안 되니 본래의 값인 가윗소리로 한다
                                    hangul_ic_save_commit_string(hic);
                                    if (!hangul_ic_push(hic, ch)) {
                                        return false;
                                    }
                                }
                            }
                        }
                    } else {// 가윗소리 조합도 안 되고 갈마들이 소리도 없으니 가윗소리로 한다
                        hangul_ic_save_commit_string(hic);
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            }
        }
    } else if (hangul_is_jongseong(ch)) {// 끝소리가 들어왔다
        if (hic->buffer.jongseong == 0) {// 끝소리가 없다
            if (hic->buffer.jungseong == 0) {// 가윗소리가 없다
                if (!hangul_ic_push(hic, ch)) {
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            } else {
                ucschar jongseong = 0;
                if (hic->option_galmadeuli_method) {// 갈마들이를 켰을 때, shift+가윗소리=겹받침
                    if (hic->buffer.choseong) {// 첫소리, 가윗소리가 있다
                        // 갈마들이 겹받침으로.
                            jongseong = hangul_ic_combine(hic, 0, ch);
                    }
                }
                if (hangul_is_jongseong(jongseong)) {
                    if (!hangul_ic_push(hic, jongseong)) {
                        if (!hangul_ic_push(hic, jongseong)) {
                            return false;
                        }
                    }
                } else {
                    if (!hangul_ic_push(hic, ch)) {
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            }
        } else {// 끝소리가 있으니 끝소리 조합을 먼저 해보자
            ucschar jongseong = 0;
            bool combine = false;
            // 바로 앞의 소리가 끝소리면, 끝소리 둘을 하나로 만들어 본다
            // 모아치기 2014 에서는 바로 앞에서 끝소리를 넣지 않았어도 끝소리가 있으면 조합해 본다
            if (hangul_is_jongseong(hangul_ic_peek(hic))) {
                combine = true;
            } else if (hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_LOOSE_ORDER)) {
                combine = true;
            }

            if (combine) {
                jongseong = hangul_ic_combine(hic, hic->buffer.jongseong, ch);
                if (jongseong == 0) {
                    if (hic->option_galmadeuli_method) {
                        if (hangul_keyboard_get_flag(hic->keyboard, HANGUL_KEYBOARD_FLAG_NO_ADDED_GGEUT) == FALSE) {
                            // 끝소리 조합이 안 되고 끝홑닿소리와들어온 끝홑닿소리가 같으면, 갈마들이 연타 겹받침으로
                            if (hic->buffer.jongseong == ch) {
                                jongseong = hangul_ic_combine(hic, 0, ch);
                            }
                        }
                    }
                }
            }

            if (hangul_is_jongseong(jongseong)) {// 겹받침이 되었다
                if (!hangul_ic_push(hic, jongseong)) {
                    if (!hangul_ic_push(hic, jongseong)) {
                        return false;
                    }
                }
            } else {// 안 되었다
                hangul_ic_save_commit_string(hic);
                if (!hangul_ic_push(hic, ch)) {
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            }
        }
    } else if (ch > 0) {// 한글 자소가 아닌 것이 들어왔다
        hangul_ic_save_commit_string(hic);
        hangul_ic_append_commit_string(hic, ch);
    } else {// 이것은 뭣도 아닌 것이 들어왔네
        hangul_ic_save_commit_string(hic);
        return false;
    }

    hangul_ic_save_preedit_string(hic);
    return true;
}


static bool
hangul_ic_process_jaso_shin_sebeol_yet (HangulInputContext *hic, int ascii, ucschar ch, bool capslock)
{
    if ( (!capslock) && //capslock 이 있으면 확장 배열을 끈다
            hic->option_extended_layout_enable) {
        int symbol_key = hangul_keyboard_is_extension_key (hic->keyboard, ascii, HANGUL_KEYBOARD_KEY_SYMBOL);
        if (symbol_key) {// 확장글쇠가 들어왔다
            if (hangul_is_extension_condition_sebeol_shin(hic)) {// 조건을 만족한다
                if (hic->option_extended_layout_index == 0) {// 첫소리 0x110b (ㅇ) 이 있던 것을 지우고 뿌린다.
                    hic->buffer.choseong = 0;
                    hangul_ic_save_commit_string(hic);
                }
                // 신세벌식은 단계별 글쇠가 따로 있기 때문에 [1, 2, 3] 대신 [11, 12, 13] 을 쓴다.
                // bool hangul_ic_backspace(HangulInputContext *hic)
                switch (symbol_key) {
                    case 1 :
                    case 2 :
                    case 3 :
                        hic->option_extended_layout_index =  symbol_key + 10;
                        ucschar* step_value = hangul_keyboard_get_addon_value(hic->keyboard, HANGUL_KEYBOARD_VALUE_EXTENDED_STEP);
                        if (step_value != NULL) 
                        {
                            hangul_buffer_push_extension_step(&hic->buffer, *(step_value+symbol_key));
                        }
                        break;
                    default :
                        hangul_buffer_clear(&hic->buffer);
                        hangul_ic_save_commit_string(hic);
                        break;
                }
                hangul_ic_save_preedit_string(hic);
                return true;
            }
        }

        if (hic->option_extended_layout_index >= 11 && hic->option_extended_layout_index <= 13) {
            //index = [ #1, #2, #3 / 11, 12, 13 ]   // 확장 기호를 다룬다.
            if (ch > 0) {
                ucschar extended = 0;
                const ucschar(*addon_func)(int, int, int) = hangul_keyboard_get_addon_func(hic->keyboard, HANGUL_FUNCTION_SYMBOL);
                if (addon_func != NULL) {
                    extended = addon_func(ascii, hic->option_extended_layout_index - 10, 0);
                }

                hangul_buffer_clear(&hic->buffer);//확장단계 표시 첫소리를 없앤다
                hangul_ic_save_commit_string(hic);

                if (extended) {// 확장기호가 있는 글쇠
                    hangul_ic_append_commit_string(hic, extended);
                }
                hangul_ic_save_preedit_string(hic);
                return true;
            } else {
                hangul_ic_save_commit_string(hic);
                return false;
            }
        }
    }

    if (hangul_is_choseong(ch)) {
        if (hic->buffer.choseong == 0) {//첫소리 없고, 들어온 첫소리를 첫소리로 다룬다.
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        } else {//첫소리 있고, 첫소리가 들어왔다.
            if (hic->buffer.jungseong == 0) {// 가윗소리가 없다
                // 갈마들이를 먼저해보고 첫소리 조합으로 간다
                ucschar jungseong = 0;
                if (!capslock) {//capslock 이 있으면 오른손 글쇠의 갈마들이 홀소리를 끈다
                    jungseong = hangul_ic_combine(hic, 0x0000, ch);
                }
                if (hangul_is_jungseong(jungseong)) {// 갈마들이 가윗소리 ㅜ, ㅗ, 아래아 가 겹홀소리에 쓰인다.
                    if (jungseong != 0x1174) {// 0x1174 (ㅢ) 는 뺀다.
                        hic->buffer.right_oua = jungseong;
                    }
                    if (!hangul_ic_push(hic, jungseong)) {
                        return false;
                    }
                } else {// 첫소리를 하나로 만들어 본다
                    ucschar choseong = 0;
                    if (hangul_is_choseong(hangul_ic_peek(hic))) {
                        choseong = hangul_ic_combine(hic, hic->buffer.choseong, ch);
                    }
                    if (hangul_is_choseong(choseong)) {// 첫소리가 하나가 되었다
                        if (!hangul_ic_push(hic, choseong)) {
                            return false;
                        }
                    } else {// 첫소리가 하나가 되지 못 했다
                        hangul_ic_save_commit_string(hic);
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            } else {//가윗소리가 있으니 첫소리로
                hangul_ic_save_commit_string(hic);
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        }
    } else if (hangul_is_jungseong(ch)) {
        if (hic->buffer.jungseong == 0) {//가윗소리가 없다.
            if (hic->buffer.jongseong == 0) {// 끝소리 없다. 첫소리가 있으나 없으나 가윗소리로
                if (hangul_keyboard_is_right_oua (hic->keyboard, ascii, ch, HANGUL_KEYBOARD_KEY_MOEUM)) {//shift+첫소리 [ᅟㅗ, ㅜ]
                    hic->buffer.right_oua = ch;
                }//shift+ㅁ [아래아] 는 홑홀소리
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            } else {// 끝소리 있다
                if (hic->buffer.choseong == 0) {//첫소리가 없다.
                    if (hangul_keyboard_is_right_oua (hic->keyboard, ascii, ch, HANGUL_KEYBOARD_KEY_MOEUM)) {//shift+첫소리 [ㅗㅜ]
                        // 새로운 글자로
                        hangul_ic_save_commit_string(hic);
                        hic->buffer.right_oua = ch;
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    } else { // 끝소리만 있다. 갈마들이로 가보자
                        //겹받침만 넣을 때 있던 끝소리에 shift+끝글쇠[홀소리]로 넣는다.
                        ucschar jongseong = 0;
                        if (hangul_is_jongseong(hangul_ic_peek(hic))) {
                            // 가윗소리를 끝소리로 바꾼다
                            ucschar jung_to_jong = 0;
                            jung_to_jong = hangul_ic_combine(hic, 0, ch);
                            if (jung_to_jong) {
                                jongseong = hangul_ic_combine(hic, hic->buffer.jongseong, jung_to_jong);
                            }
                        }
                        if (hangul_is_jongseong(jongseong)) {// 겹받침이 되었다
                            if (!hangul_ic_push(hic, jongseong)) {
                                return false;
                            }
                        } else {// 겹받침이 되지 못 했으니 가윗소리로 다룬다
                            hangul_ic_save_commit_string(hic);
                            if (!hangul_ic_push(hic, ch)) {
                                return false;
                            }
                        }
                    }
                } else {// 첫소리, 끝소리가 있다. 새로운 글자로
                    //Shift + 글쇠인 가윗소리를 넣는다
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            }
        } else {// 가윗소리가 있다
            if (hic->buffer.jongseong == 0) {// 끝소리가 없으니
                //가윗소리가 있을 때 shift+첫소리의 가윗소리가 들어오면 무조건 새로운 글자로
                if (hangul_keyboard_is_right_oua (hic->keyboard, ascii, ch, HANGUL_KEYBOARD_KEY_MOEUM)) {
                    hangul_ic_save_commit_string(hic);
                    hic->buffer.right_oua = ch;
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                } else {
                    // 옛한글에서는 왼쪽의 홀소리 끼리의 조합이 가능해야 한다
                    ucschar jungseong = 0;
                    if (hangul_is_jungseong(hangul_ic_peek(hic))) {
                        jungseong = hangul_ic_combine(hic, hic->buffer.jungseong, ch);
                    }
                    if (hangul_is_jungseong(jungseong)) {// 조합된 가윗소리
                        if (!hangul_ic_push(hic, jungseong)) {
                            return false;
                        }
                    } else {// 조합이 되지 않으니 새로운 글자로
                        hangul_ic_save_commit_string(hic);
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            } else {// 가윗소리, 끝소리가 있다, 새로운 글자로
                hangul_ic_save_commit_string(hic);
                if (hangul_keyboard_is_right_oua (hic->keyboard, ascii, ch, HANGUL_KEYBOARD_KEY_MOEUM)) {//shift+첫소리
                    hic->buffer.right_oua = ch;
                }
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        }
    } else if (hangul_is_jongseong(ch)) {
        if (hic->buffer.jungseong == 0) {// 가윗소리가 없다.
            if (hic->buffer.choseong == 0) {//첫소리가 없다.
                if (hic->buffer.jongseong == 0) {//끝소리가 없다.
                   if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                } else {//끝소리가 있다. // 초성체를 위해
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            } else {//첫소리가 있다.
                ucschar jungseong = 0;
                jungseong = hangul_ic_combine(hic, 0, ch);

                if (hangul_is_jungseong(jungseong)) {//가윗소리가 있는 글쇠는 가윗소리로.
                    if (!hangul_ic_push(hic, jungseong)) {
                        return false;
                    }
                } else {
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            }
        } else {//가윗소리가 있다.
            if (hic->buffer.jongseong == 0) {//끝소리가 없다.
                // 먼저 끝소리를 가윗소리로 바꾸어 가윗소리 조합을 해본다.
                if (hangul_is_jungseong(hangul_ic_peek(hic)) && hic->buffer.right_oua) {
                    ucschar jungseong = 0;
                    ucschar jong_to_jung = hangul_ic_combine(hic, 0, ch);

                    if (jong_to_jung) {
                        jungseong = hangul_ic_combine(hic, hic->buffer.jungseong, jong_to_jung);
                    }
                    if (hangul_is_jungseong(jungseong)) {// 끝소리를 가윗소리로 바꾸어 가윗소리 조합이 되었다
                        if (!hangul_ic_push(hic, jungseong)) {
                            return false;
                        }
                    } else {// 끝소리로 넣는다
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                } else { // 끝소리를 가윗소리로 바꾸지 않고 끝소리로 다룬다
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            } else {//끝소리가 있다.
                ucschar jongseong = 0;
                if (hangul_is_jongseong(hangul_ic_peek(hic))) {
                    jongseong = hangul_ic_combine(hic, hic->buffer.jongseong, ch);
                }
                if (hangul_is_jongseong(jongseong)) { // 겹받침이 되었다
                    if (!hangul_ic_push(hic, jongseong)) {
                        return false;
                    }
                } else {
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            }
        }
    } else if (ch > 0) {
        if (ch == 0x00D7 || ch == 0x25CB)
        {// 옛한글 조합 상태에서는 3-93 옛한글 자판과 같이
        // U 자리와 Y 자리에 방점 2개( 0x302E (〮),  0x302F (〯) )를 넣는다
            ucschar temp = 0;
            temp = hangul_ic_combine(hic, 0, ch);

            if (temp == 0) {
                temp = ch;
            }
            hangul_ic_save_commit_string(hic);
            hangul_ic_append_commit_string(hic, temp);
        } else {
            hangul_ic_save_commit_string(hic);
            hangul_ic_append_commit_string(hic, ch);
        }
    } else {
        hangul_ic_save_commit_string(hic);
        return false;
    }

    hangul_ic_save_preedit_string(hic);
    return true;
}

static bool
hangul_ic_process_jaso_shin_sebeol_shift (HangulInputContext *hic, int ascii, ucschar ch)
{
    if (hangul_is_choseong(ch)) {
        if (hic->buffer.choseong == 0) {//첫소리 없고, 들어온 첫소리를 첫소리로 다룬다.
            hangul_ic_save_commit_string(hic);
            if (!hangul_ic_push(hic, ch)) {
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        } else {//첫소리 있고, 첫소리가 들어왔다.
            if (hic->buffer.jungseong == 0) {// 가윗소리가 없다
                // 갈마들이로 해보고 첫소리 조합으로 간다
                ucschar jungseong = 0;
                jungseong = hangul_ic_combine(hic, 0, ch);

                if (hangul_is_jungseong(jungseong)) {// 갈마들이 가윗소리 ㅜ, ㅗ, 아래아 가 겹홀소리에 쓰인다.
                    if (jungseong != 0x1174) {// 0x1174 (ㅢ) 는 뺀다.
                        hic->buffer.right_oua = jungseong;
                    }
                        if (!hangul_ic_push(hic, jungseong)) {
                        return false;
                    }
                } else {// 첫소리를 하나로 만들어 본다
                    ucschar choseong = 0;
                    if (hangul_is_choseong(hangul_ic_peek(hic))) {
                        choseong = hangul_ic_combine(hic, hic->buffer.choseong, ch);
                    }
                    if (hangul_is_choseong(choseong)) {
                        if (!hangul_ic_push(hic, choseong)) {
                            return false;
                        }
                    } else {// 첫소리가 하나가 되지 못 했다.
                        hangul_ic_save_commit_string(hic);
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            } else {//가윗소리가 있으니 첫소리로
                hangul_ic_save_commit_string(hic);
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        }
    } else if (hangul_is_jungseong(ch)) {
        if (hic->buffer.jungseong == 0) {// 가윗소리가 없다.
            if (hic->buffer.jongseong == 0) {//끝소리가 없다.
                //if (hic->buffer.choseong == 0) {
                    // 첫소리가 없으면 왼쪽 ㅗ·ㅜ로도 ㅘ·ㅙ·ㅚ·ㅝ·ㅞ·ㅟ를 조합할 수 있다.
                    // (왼쪽 ㅗ·ㅜ와 오른쪽 ㅗ·ㅜ의 동작이 같음)
                //    hic->buffer.right_oua = ch;
                //}
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            } else {
                hangul_ic_save_commit_string(hic);
                // 첫소리가 없으면 왼쪽 ㅗ·ㅜ로도 ㅘ·ㅙ·ㅚ·ㅝ·ㅞ·ㅟ를 조합할 수 있다.
                // (왼쪽 ㅗ·ㅜ와 오른쪽 ㅗ·ㅜ의 동작이 같음)
                hic->buffer.right_oua = ch;
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        } else {//가윗소리가 있다.
            if (hic->buffer.jongseong == 0) {//끝소리가 없다.
                // 먼저 가윗소리 조합을 해본다.
                if (hangul_is_jungseong(hangul_ic_peek(hic)) && hic->buffer.right_oua) {
                    ucschar jungseong = 0;
                    jungseong = hangul_ic_combine(hic, hic->buffer.jungseong, ch);

                    // 가윗소리 조합이 되었다
                    if (hangul_is_jungseong(jungseong)) {
                        if (!hangul_ic_push(hic, jungseong)) {
                            return false;
                        }
                    } else {
                        if (hic->buffer.choseong == 0) {
                            // 첫소리가 없으면 새로운 글자로 한다
                            hangul_ic_save_commit_string(hic);
                            hic->buffer.right_oua = ch;
                            if (!hangul_ic_push(hic, ch)) {
                                return false;
                            }
                        } else {
                            ucschar jongseong = 0;
                            jongseong = hangul_ic_combine(hic, 0, ch);

                            if (hangul_is_jongseong(jongseong)) {
                                if (!hangul_ic_push(hic, jongseong)) {
                                    return false;
                                }
                            } else {// 첫소리, 가윗소리가 있는데 갈마들이 끝소리가 없다
                                hangul_ic_save_commit_string(hic);
                                hic->buffer.right_oua = ch;
                                if (!hangul_ic_push(hic, ch)) {
                                    return false;
                                }
                            }
                        }
                    }
                } else {
                    ucschar jongseong = hangul_ic_combine(hic, 0, ch);

                    if (hangul_is_jongseong(jongseong)) {
                        if (!hangul_ic_push(hic, jongseong)) {
                            return false;
                        }
                    } else {
                        hangul_ic_save_commit_string(hic);
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                }
            } else {//끝소리가 있다.
                ucschar jongseong = 0;
                if (hangul_is_jongseong(hangul_ic_peek(hic))) {
                    // 가윗소리를 끝소리로 바꾸어서 다룬다
                    ucschar jong = 0;
                    jong = hangul_ic_combine(hic, 0, ch);

                    if (hangul_is_jongseong(jong)) {
                        if (hic->buffer.jongseong == jong) {//두 번 누를 때 겹받침으로 바꾼다.
                            jongseong = hangul_ic_combine(hic, 0, jong);
                        }
                        if (jongseong == 0) {//끝소리가 다르거나 두 번 누를 때 겹받침으로 바꾸지 못하면 종성결합 규칙으로.
                            jongseong = hangul_ic_combine(hic, hic->buffer.jongseong, jong);
                        }
                        if (hangul_is_jongseong(jongseong)) { // 겹받침이 되었다
                            if (!hangul_ic_push(hic, jongseong)) {
                                return false;
                            }
                        } else {// 겹받침으로 바꾸지 못 했다
                            hangul_ic_save_commit_string(hic);
                            if (!hangul_ic_push(hic, ch)) {
                                return false;
                            }
                        }
                    } else {// 갈마들이로 바꾸지 못 했다
                        hangul_ic_save_commit_string(hic);
                        if (!hangul_ic_push(hic, ch)) {
                            return false;
                        }
                    }
                } else {
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            }
        }
    } else if (hangul_is_jongseong(ch)) {
        if (hic->buffer.jongseong == 0) {// 끝소리 없다. 첫소리가 있으나 없으나 가윗소리로
            if (!hangul_ic_push(hic, ch)) {
                return false;
            }
        } else {// 끝소리 있다
            ucschar jongseong = 0;
            if (hangul_is_jongseong(hangul_ic_peek(hic))) {
                jongseong = hangul_ic_combine(hic, hic->buffer.jongseong, ch);

                if (hangul_is_jongseong(jongseong)) {// 조합된 끝소리
                    if (!hangul_ic_push(hic, jongseong)) {
                        return false;
                    }
                } else {// 조합이 되지 않으니 새로운 글자로
                    hangul_ic_save_commit_string(hic);
                    if (!hangul_ic_push(hic, ch)) {
                        return false;
                    }
                }
            } else {
                hangul_ic_save_commit_string(hic);
                if (!hangul_ic_push(hic, ch)) {
                    return false;
                }
            }
        }
    } else if (ch > 0) {
        hangul_ic_save_commit_string(hic);
        hangul_ic_append_commit_string(hic, ch);
    } else {
        hangul_ic_save_commit_string(hic);
        return false;
    }

    hangul_ic_save_preedit_string(hic);
    return true;
}


#endif /* libhangul_hangulinputcontext_addon_c */