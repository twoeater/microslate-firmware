#pragma once

#include <cstdint>
#include <cstddef>

enum class HangulStage {
    IDLE,
    INITIAL,
    MEDIAL,
    FINAL
};

class HangulEngine {
public:
    HangulEngine();

    void inputKey(uint8_t keyCode, bool shifted);
    uint32_t getComposedChar() const;
    HangulStage getStage() const;
    bool isComposing() const;

    void commit();
    void reset();
    void backspace();

    const uint8_t* getUtf8Bytes(size_t& outLen) const;

private:
    uint32_t initialIndex(uint32_t jamo) const;
    uint32_t medialIndex(uint32_t jamo) const;
    uint32_t finalIndex(uint32_t jamo) const;
    bool isInitialConsonant(uint32_t jamo) const;
    bool isFinalConsonant(uint32_t jamo) const;
    bool isVowel(uint32_t jamo) const;

    uint32_t composeSyllable(uint32_t initIdx, uint32_t medIdx, uint32_t finIdx) const;
    void encodeUtf8(uint32_t cp, uint8_t buf[4], size_t& len) const;

    uint32_t currentInitial;
    uint32_t currentMedial;
    uint32_t currentFinal;
    HangulStage stage;

    uint8_t utf8Buf[4];
    size_t utf8Len;
};
