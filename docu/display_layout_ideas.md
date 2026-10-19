My question:
Following: I want to have an universal definition of dislay layouts, like 16x16 for 12 hours or 18x16 for 24 hours. In this case I can have different definitions for each clock variant, e.g. eum #sym:tWord . My question: how I can organize such definitions, may be use classes insteandof typedef: abstract class word, 12h variant have oun set of words (e.g. 20 words), 24 h - 35 words. There are some better alternatives?

AI answer:
**Use a separate layout definition for each variant, with its own `enum class` and constant tables. An abstract class for individual words is unnecessary.**

A word is data—an identifier and its position—not an object requiring inheritance. Classes become useful for grouping an entire layout or implementing different time-formatting behavior.

## 1. Separate three responsibilities

Your current `DisplayLayout.h` combines:

| Responsibility | Examples |
|---|---|
| Panel layout | Width, height, word positions, available words |
| Time phrasing | 12/24-hour cycle, “viertel nach”, hour offset |
| Rendering | Converting word positions into LED pixels |

Keep these conceptually separate. A larger panel does not necessarily imply a different time-formatting algorithm, and the same time format can be used on different panels.

## 2. Recommended: layout types with their own enums

For one layout selected **at build time**, use ordinary structs and, where useful, templates.

Your platformio.ini already enables C++17, so `inline static constexpr` tables are available.

Here is a small example; the word lists and coordinates are illustrative:

```cpp
#include <array>
#include <cstddef>
#include <cstdint>

struct WordData
{
    uint8_t row;
    uint8_t column;
    uint8_t length;
};

struct Layout12h
{
    static constexpr uint16_t width = 16;
    static constexpr uint16_t height = 16;
    static constexpr uint8_t hourCycle = 12;

    enum class Word : uint8_t
    {
        None,
        Es,
        Ist,
        Eins,
        Uhr,
        Count
    };

    inline static constexpr std::array<
        WordData, static_cast<std::size_t>(Word::Count)> words{{
        {  0,  0, 0 }, // None
        {  3, 14, 2 }, // Es
        {  4,  7, 3 }, // Ist
        { 13,  0, 4 }, // Eins
        { 14, 13, 3 }, // Uhr
    }};
};

struct Layout24h
{
    static constexpr uint16_t width = 18;
    static constexpr uint16_t height = 16;
    static constexpr uint8_t hourCycle = 24;

    enum class Word : uint8_t
    {
        None,
        Es,
        Ist,
        Dreizehn,
        Vierzehn,
        Uhr,
        Count
    };

    inline static constexpr std::array<
        WordData, static_cast<std::size_t>(Word::Count)> words{{
        {  0,  0, 0 }, // None
        {  0,  0, 2 }, // Es
        {  0,  3, 3 }, // Ist
        {  1,  0, 8 }, // Dreizehn
        {  2,  0, 8 }, // Vierzehn
        { 15, 15, 3 }, // Uhr
    }};
};
```

Each layout can contain 20, 35, or any other number of words. No shared enum or inheritance is required.

Importantly, `Layout12h::Word` and `Layout24h::Word` are **different types**, preventing accidental mixing.

### Generic rendering

The rendering logic only needs to know the selected layout’s type:

```cpp
template<class Layout>
class WordRenderer
{
public:
    using Word = typename Layout::Word;

    void AddWord(BitMatrix& mask, Word word) const
    {
        const auto index = static_cast<std::size_t>(word);

        if (index == 0 || index >= Layout::words.size())
            return;

        const auto& data = Layout::words[index];
        mask.SetLine(data.row, data.column, data.length);
    }
};
```

Select the variant in one place:

```cpp
using ActiveLayout = Layout12h;
// using ActiveLayout = Layout24h;

using ActiveWord = ActiveLayout::Word;

WordRenderer<ActiveLayout> renderer;
BitMatrix mask(ActiveLayout::width, ActiveLayout::height);

mask.ClearAll();
renderer.AddWord(mask, ActiveWord::Eins);
```

This provides compile-time type checking without virtual dispatch or dynamically allocated word objects.

**You do not have to template the entire application.** If only one layout exists in each firmware build, other code can simply use `ActiveLayout` and `ActiveWord`.

## 3. Keep time-formatting tables with the appropriate variant

Your current `Display::PaintTime()` directly uses global tables and explicitly normalizes to 12 hours. Therefore, replacing only the dimensions and enum would not be sufficient.

Move these variant-dependent definitions together:

- Hour-word table.
- Minute-word table.
- Extra-minute table.
- Hour cycle.
- Supported phrasing modes.

If both variants use the same table-driven algorithm, the formatter can also be templated:

```cpp
template<class Layout>
class TimeFormatter
{
public:
    using Word = typename Layout::Word;

    // Read Layout's tables and produce a sequence of Word values.
};
```

If 12-hour and 24-hour phrasing are substantially different, use separate formatter implementations rather than filling one algorithm with conditions.

For hour indexing, when the table is ordered from hour zero, normalization becomes:

```cpp
const auto hourIndex =
    (hour + hourOffset) % Layout::hourCycle;
```

This handles midnight rollover for either cycle.

## 4. When would an abstract class make sense?

Use an abstract **clock face/formatter**, not an abstract word, if the user must switch variants **at runtime** within the same firmware.

For example:

```cpp
class IClockFace
{
public:
    virtual ~IClockFace() = default;

    virtual uint16_t Width() const = 0;
    virtual uint16_t Height() const = 0;

    virtual void BuildTimeMask(
        uint8_t hour,
        uint8_t minute,
        BitMatrix& mask) const = 0;
};
```

Each concrete implementation can keep its own enum internally. The common interface exchanges a pixel mask, so it does not need a universal word enum.

| Requirement | Suitable approach |
|---|---|
| One layout per firmware build | Layout structs + selected type alias |
| Reusable algorithms for different layouts | Templates |
| Runtime layout selection | Abstract clock-face interface |
| Only coordinates differ; vocabulary is identical | Shared enum + separate data tables |

## Important for your 18×16 variant

There is already a rectangular-matrix issue in `BitMatrix.h`: coordinate indexing in `IsBitSet(row, col)` and `SetBit(row, col)` uses:

```cpp
aRow * mHeight + aCol
```

For row-major storage, it should use:

```cpp
aRow * mWidth + aCol
```

The problem is hidden with a 16×16 matrix but becomes visible with 18×16.

**For this project, start with layout structs, per-layout `enum class Word`, and an `ActiveLayout` alias. Add templates for shared rendering/formatting algorithms; introduce virtual interfaces only if runtime switching is actually needed.**