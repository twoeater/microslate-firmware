#include "hangul_engine.h"
#include "config.h"

static const uint32_t HANGUL_BASE = 0xAC00;
static const uint32_t HANGUL_MEDIAL_COUNT = 21;
static const uint32_t HANGUL_FINAL_COUNT = 28;

// Choseong (초성) index mapping for Unicode syllable calculation
// Index: ㄱ=0, ㄲ=1, ㄳ=2, ㄴ=3, ㄵ=4, ㄶ=5, ㄷ=6, ㄸ=7, ㄹ=8, ㄺ=9, ㄻ=10, ㄼ=11, ㄽ=12, ㄾ=13, ㄿ=14, ㅀ=15, ㅁ=16, ㅂ=17, ㅃ=18, ㅄ=19, ㅅ=20, ㅆ=21, ㅇ=22, ㅈ=23, ㅉ=24, ㅊ=25, ㅋ=26, ㅌ=27, ㅍ=28, ㅎ=29
static const uint32_t CHOSEONG_JAMO[30] = {
    0x1100, 0x1101, 0x1102, 0x1103, 0x1104, 0x1105,
    0x1106, 0x1107, 0x1108, 0x1109, 0x110A, 0x110B,
    0x110C, 0x110D, 0x110E, 0x110F, 0x1110, 0x1111,
    0x1112, 0x1113, 0x1114, 0x1115, 0x1116, 0x1117,
    0x1118, 0x1119, 0x111A, 0x111B, 0x111C, 0x111D
};

// Jungseong (중성) index mapping
// Index: ㅏ=0, ㅐ=1, ㅑ=2, ㅒ=3, ㅓ=4, ㅔ=5, ㅕ=6, ㅖ=7, ㅗ=8, ㅘ=9, ㅙ=10, ㅚ=11, ㅛ=12, ㅜ=13, ㅝ=14, ㅞ=15, ㅟ=16, ㅡ=17, ㅢ=18, ㅣ=19
static const uint32_t JUNGSEONG_JAMO[21] = {
    0x1161, 0x1162, 0x1163, 0x1164, 0x1165, 0x1166,
    0x1167, 0x1168, 0x1169, 0x116A, 0x116B, 0x116C,
    0x116D, 0x116E, 0x116F, 0x1170, 0x1171, 0x1172,
    0x1173, 0x1174
};

// Jongseong (종성) index mapping (0 = none)
// Index: 없음=0, ㄱ=1, ㄲ=2, ㄳ=3, ㄴ=4, ㄵ=5, ㄶ=6, ㄷ=7, ㄸ=8, ㄹ=9, ㄺ=10, ㄻ=11, ㄼ=12, ㄽ=13, ㄾ=14, ㄿ=15, ㅀ=16, ㅁ=17, ㅂ=18, ㅃ=19, ㅄ=20, ㅅ=21, ㅆ=22, ㅇ=23, ㅈ=24, ㅉ=25, ㅊ=26, ㅋ=27, ㅌ=28, ㅍ=29, ㅎ=30
static const uint32_t JONGSEONG_JAMO[31] = {
    0, 0x1100, 0x1101, 0x1102, 0x1103, 0x1104, 0x1105,
    0x1106, 0x1107, 0x1108, 0x1109, 0x110A, 0x110B,
    0x110C, 0x110D, 0x110E, 0x110F, 0x1110, 0x1111,
    0x1112, 0x1113, 0x1114, 0x1115, 0x1116, 0x1117,
    0x1118, 0x1119, 0x111A, 0x111B, 0x111C, 0x111D
};

// Two-Set keyboard mapping
// Consonants: Q=ㅂ, W=ㅈ, E=ㄷ, R=ㄱ, T=ㅅ, A=ㅁ, S=ㄴ, D=ㅇ, F=ㄹ, G=ㅎ, Z=ㅋ, X=ㅌ, C=ㅊ, V=ㅍ
// Double consonants (Shift): Q=ㅃ, W=ㅉ, E=ㄸ, R=ㄲ, T=ㅆ
// Vowels: Y=ㅛ, U=ㅕ, I=ㅑ, O=ㅐ, P=ㅔ, H=ㅗ, J=ㅓ, K=ㅏ, L=ㅣ, B=ㅠ, N=ㅜ, M=ㅡ
// Double vowels (Shift): O=ㅒ, P=ㅖ

HangulEngine::HangulEngine()
    : currentInitial(0), currentMedial(0), currentFinal(0),
      stage(HangulStage::IDLE), utf8Len(0) {}

uint32_t HangulEngine::initialIndex(uint32_t jamo) const {
    for (uint32_t i = 0; i < 30; i++) {
        if (CHOSEONG_JAMO[i] == jamo) return i;
    }
    return 0;
}

uint32_t HangulEngine::medialIndex(uint32_t jamo) const {
    for (uint32_t i = 0; i < 21; i++) {
        if (JUNGSEONG_JAMO[i] == jamo) return i;
    }
    return 0;
}

uint32_t HangulEngine::finalIndex(uint32_t jamo) const {
    for (uint32_t i = 1; i < 31; i++) {
        if (JONGSEONG_JAMO[i] == jamo) return i;
    }
    return 0;
}

bool HangulEngine::isInitialConsonant(uint32_t jamo) const {
    for (uint32_t i = 0; i < 30; i++) {
        if (CHOSEONG_JAMO[i] == jamo) return true;
    }
    return false;
}

bool HangulEngine::isFinalConsonant(uint32_t jamo) const {
    for (uint32_t i = 1; i < 31; i++) {
        if (JONGSEONG_JAMO[i] == jamo) return true;
    }
    return false;
}

bool HangulEngine::isVowel(uint32_t jamo) const {
    for (uint32_t i = 0; i < 21; i++) {
        if (JUNGSEONG_JAMO[i] == jamo) return true;
    }
    return false;
}

uint32_t HangulEngine::composeSyllable(uint32_t initIdx, uint32_t medIdx, uint32_t finIdx) const {
    return HANGUL_BASE + (initIdx * HANGUL_MEDIAL_COUNT * HANGUL_FINAL_COUNT)
                         + (medIdx * HANGUL_FINAL_COUNT)
                         + finIdx;
}

void HangulEngine::encodeUtf8(uint32_t cp, uint8_t buf[4], size_t& len) const {
    if (cp < 0x80) {
        buf[0] = (uint8_t)cp;
        len = 1;
    } else if (cp < 0x800) {
        buf[0] = (uint8_t)(0xC0 | (cp >> 6));
        buf[1] = (uint8_t)(0x80 | (cp & 0x3F));
        len = 2;
    } else {
        buf[0] = (uint8_t)(0xE0 | (cp >> 12));
        buf[1] = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
        buf[2] = (uint8_t)(0x80 | (cp & 0x3F));
        len = 3;
    }
}

uint32_t HangulEngine::getComposedChar() const {
    if (stage == HangulStage::IDLE) return 0;

    uint32_t initIdx = initialIndex(currentInitial);
    uint32_t medIdx = medialIndex(currentMedial);
    uint32_t finIdx = finalIndex(currentFinal);

    return composeSyllable(initIdx, medIdx, finIdx);
}

HangulStage HangulEngine::getStage() const {
    return stage;
}

bool HangulEngine::isComposing() const {
    return stage != HangulStage::IDLE;
}

const uint8_t* HangulEngine::getUtf8Bytes(size_t& outLen) const {
    outLen = utf8Len;
    return utf8Buf;
}

void HangulEngine::inputKey(uint8_t keyCode, bool shifted) {
    uint32_t jamo = 0;

    // Consonants (Two-Set Standard)
    switch (keyCode) {
        case 0x14: jamo = shifted ? 0x1112 : 0x1111; break;  // Q: ㅃ/ㅂ
        case 0x1A: jamo = shifted ? 0x1118 : 0x1117; break;  // W: ㅉ/ㅈ
        case 0x08: jamo = shifted ? 0x1107 : 0x1106; break;  // E: ㄸ/ㄷ
        case 0x15: jamo = shifted ? 0x1101 : 0x1100; break;  // R: ㄲ/ㄱ
        case 0x17: jamo = shifted ? 0x1115 : 0x1114; break;  // T: ㅆ/ㅅ
        case 0x04: jamo = 0x1110; break;  // A: ㅁ
        case 0x16: jamo = 0x1103; break;  // S: ㄴ
        case 0x07: jamo = 0x1116; break;  // D: ㅇ
        case 0x09: jamo = 0x1108; break;  // F: ㄹ
        case 0x0A: jamo = 0x111D; break;  // G: ㅎ
        case 0x1D: jamo = 0x111A; break;  // Z: ㅋ
        case 0x1B: jamo = 0x111B; break;  // X: ㅌ
        case 0x06: jamo = 0x1119; break;  // C: ㅊ
        case 0x19: jamo = 0x111C; break;  // V: ㅍ

        // Vowels
        case 0x1C: jamo = 0x116D; break;   // Y: ㅛ
        case 0x18: jamo = 0x1167; break;   // U: ㅕ
        case 0x0C: jamo = 0x1163; break;   // I: ㅑ
        case 0x12: jamo = shifted ? 0x1164 : 0x1162; break; // O: ㅒ/ㅐ
        case 0x13: jamo = shifted ? 0x1168 : 0x1166; break; // P: ㅖ/ㅔ
        case 0x0B: jamo = 0x1169; break;   // H: ㅗ
        case 0x0D: jamo = 0x1165; break;   // J: ㅓ
        case 0x0E: jamo = 0x1161; break;   // K: ㅏ
        case 0x0F: jamo = 0x1174; break;   // L: ㅣ
        case 0x05: jamo = 0x116B; break;   // B: ㅠ
        case 0x11: jamo = 0x116E; break;   // N: ㅜ
        case 0x10: jamo = 0x1172; break;   // M: ㅡ

        default: return;
    }

    if (jamo == 0) return;

    if (isVowel(jamo)) {
        if (stage == HangulStage::IDLE) {
            currentInitial = 0x1116;  // ㅇ
            currentMedial = jamo;
            currentFinal = 0;
            stage = HangulStage::MEDIAL;
        } else if (stage == HangulStage::INITIAL) {
            currentMedial = jamo;
            currentFinal = 0;
            stage = HangulStage::MEDIAL;
        } else if (stage == HangulStage::MEDIAL) {
            reset();
            currentInitial = 0x1116;  // ㅇ
            currentMedial = jamo;
            currentFinal = 0;
            stage = HangulStage::MEDIAL;
        } else if (stage == HangulStage::FINAL) {
            reset();
            currentInitial = 0x1116;  // ㅇ
            currentMedial = jamo;
            currentFinal = 0;
            stage = HangulStage::MEDIAL;
        }
    } else if (isInitialConsonant(jamo)) {
        if (stage == HangulStage::IDLE) {
            currentInitial = jamo;
            currentMedial = 0;
            currentFinal = 0;
            stage = HangulStage::INITIAL;
        } else if (stage == HangulStage::INITIAL) {
            currentInitial = jamo;
        } else if (stage == HangulStage::MEDIAL) {
            if (isFinalConsonant(jamo)) {
                currentFinal = jamo;
                stage = HangulStage::FINAL;
            } else {
                reset();
                currentInitial = jamo;
                currentMedial = 0;
                currentFinal = 0;
                stage = HangulStage::INITIAL;
            }
        } else if (stage == HangulStage::FINAL) {
            reset();
            currentInitial = jamo;
            currentMedial = 0;
            currentFinal = 0;
            stage = HangulStage::INITIAL;
        }
    }

    uint32_t cp = getComposedChar();
    encodeUtf8(cp, utf8Buf, utf8Len);
}

void HangulEngine::commit() {
    reset();
}

void HangulEngine::reset() {
    currentInitial = 0;
    currentMedial = 0;
    currentFinal = 0;
    stage = HangulStage::IDLE;
    utf8Len = 0;
}

void HangulEngine::backspace() {
    if (stage == HangulStage::FINAL) {
        currentFinal = 0;
        stage = HangulStage::MEDIAL;
    } else if (stage == HangulStage::MEDIAL) {
        currentMedial = 0;
        stage = HangulStage::INITIAL;
    } else if (stage == HangulStage::INITIAL) {
        reset();
        return;
    }

    uint32_t cp = getComposedChar();
    encodeUtf8(cp, utf8Buf, utf8Len);
}
