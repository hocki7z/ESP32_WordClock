My question:
how better organize display presentation:
- color for background
- present or not and color for
    - 'es ist'
    - additional minutes
- color for current time 
- later, colors for additional words

Color for one led: uint32_t (4 bytes): 1 byte per RGB component
Variant A: code each led with corresponding color. I need 16x16x4 = 1024 bytes
Varinat B: using layer structure with BitMatrix,
   one layer: 32 bytes for BitMask + 4 bytes for color = 36 bytes 

   layers 'es ist', time self and additional minutes need only 104 bytes. But for all additional words I should present them in separated layers and this maybe in niot very optimal

Variant C: layers with sets of words + color (can be very compact)

Could you analyze my proposal and select better variant or propose own one?

AI answer:

**Recommend Variant C for describing the presentation, rendered into the existing FastLED pixel buffer (Variant A).** These serve different purposes: a compact description of *what to display*, and a final buffer of *what each LED outputs*.

There is no need to choose only one representation for both.

## 1. Comparing your variants

| Variant | Advantages | Limitations | Best use |
|---|---|---|---|
| **A: color per LED** | Simple rendering; arbitrary colors, gradients and effects | Loses information about words and groups; larger snapshots | Final LED output |
| **B: mask + color per layer** | Supports arbitrary shapes; straightforward composition | Stores geometry already available in your word table; overhead grows with layers | Icons, effects, arbitrary pixel overlays |
| **C: words + color per group** | Compact; directly expresses your requirements; reuses existing layout | Limited to predefined words unless extended | Word-clock presentation |

Two memory corrections:

- RGB uses **3 bytes**. A `uint32_t` is convenient for `0x00RRGGBB`, but its fourth byte is unused unless you explicitly assign another meaning.
- Three layers of 36 bytes require **108 bytes**, not 104.

Also, 36 bytes describes a **packed mask payload plus color**, not your current `BitMatrix` object. Your BitMatrix implementation additionally contains dimensions, sizes, a pointer, a virtual-function pointer, and a separate heap allocation.

You already have the final pixel buffer as `mLeds` in src/Display.h. With FastLED’s three-byte `CRGB`, 256 LEDs require **768 bytes**. You do not need another 1024-byte buffer.

## 2. Suggested organization

Separate three concepts:

### Presentation settings — appearance

Persistent user preferences:

- Background color.
- “ES IST”: enabled and color.
- Main time: color.
- Additional minutes: enabled and color.
- Later: additional-word or notification styles.

**Visibility should be independent of color.** Disabled means “do not paint”; enabled with black means “paint black,” potentially covering something underneath.

### Display scene — current content

A small collection of ordered word groups:

```text
Scene
├── background color
├── time words          + time style
├── ES / IST            + prefix style
├── extra-minute words  + extra-minute style
└── additional words    + their styles
```

Your existing `tWord` and `mcWordDataArray` in `DisplayLayout.h` already provide the vocabulary and geometry needed for this.

### Renderer — output

The renderer:

1. Fills the LED buffer with the background color.
2. Draws enabled word groups in a defined order.
3. Maps logical coordinates to physical LED indices.
4. Applies overall brightness and calls `FastLED.show()`.

This keeps time calculation, appearance settings, and LED wiring separate.

## 3. A practical Variant C

For the current requirements, a bounded word list per group is sufficient:

```cpp
#include <array>
#include <cstdint>

struct WordGroup
{
    uint32_t mColor = 0;             // 0x00RRGGBB
    std::array<tWord, 5> mWords{};   // Main time needs at most 3 + 2
    uint8_t mWordCount = 0;
    bool mEnabled = true;
};

struct DisplayScene
{
    uint32_t mBackgroundColor = 0;

    WordGroup mTime;
    WordGroup mItIs;
    WordGroup mExtraMinutes;

    // Add a bounded collection of notification groups when needed.
};
```

With typical four-byte alignment, `WordGroup` would occupy **12 bytes**, and this scene **40 bytes**. Verify with `sizeof` for your build.

This deliberately sacrifices a few bytes in the smaller groups for simplicity. It is still substantially smaller than three pixel masks.

For future additional words:

- Multiple words sharing one appearance can belong to **one group**.
- An independently colored word can use its **own group**.
- Allocate capacity for the maximum number of **simultaneously displayed groups**, not necessarily every possible word.

If many independently colored single words eventually make group storage inefficient, switch that collection to a flat list of `{word, styleId}` entries plus a style table. There is no need for that complexity initially.

## 4. Render words directly; masks become optional

Your words are horizontal runs described by `mRow`, `mColumn`, and `mLength`. Consequently, the renderer can paint those runs directly:

```cpp
void Display::PaintWordLogical(tWord aWord, CRGB aColor)
{
    if (aWord <= WORD_END_OF_WORDS || aWord >= WORD_MAX_NUMBER)
    {
        return;
    }

    const auto& wWord = mcWordDataArray[aWord];

    for (uint16_t wOffset = 0; wOffset < wWord.mLength; ++wOffset)
    {
        const uint16_t wColumn = wWord.mColumn + wOffset;

        const uint16_t wPhysicalColumn =
            (wWord.mRow % 2 == 0)
                ? MATRIX_WIDTH - 1 - wColumn
                : wColumn;

        mLeds[wWord.mRow * MATRIX_WIDTH + wPhysicalColumn] = aColor;
    }
}
```

This is an illustrative new method, not an applied change.

Unlike the current `PaintWord()` and `PaintTime()` in src/Display.cpp, this approach does not need to create and flip a full mask for each group. Ideally, put the coordinate conversion in one reusable `LogicalToPhysicalIndex()` helper.

**Retain `BitMatrix` when you actually need arbitrary pixel shapes or mask operations**, rather than using it to represent every word.

## 5. Define overlap behavior explicitly

Some words share LEDs in your layout—for example, `WORD_FRISEUR` overlaps `WORD_CLOCK_MIN_5`.

No representation can display two independent colors on one physical LED simultaneously. Define a policy:

> Background first; normal clock content next; notifications last. Later groups overwrite earlier groups.

If a notification must remain fully readable, you may instead need to suppress a conflicting clock word or alternate between scenes. This is a presentation decision, not a memory-layout issue.

## 6. Use the same scene for the web preview

The selected `mDisplayPreviewMask.Copy(mLedMask)` preserves only **on/off state**. It cannot distinguish time, “ES IST,” and additional minutes once they have different colors.

Instead, publish a consistent **scene snapshot**, including its colors, to the web task:

- LED renderer: scene → physical LED colors.
- SVG renderer: scene → logical letter colors.
- Both use the same word layout and overlap order.

The current `BuildDisplayPreviewSvg()` in src/WebSite.cpp only tests one mask and uses hard-coded active/inactive CSS colors, so it would need this richer input.

Use a copied message payload or synchronized snapshot; a shared mutable scene should not be read while the display task is updating it.

### Final choice

**C for presentation + existing A for hardware output; B only where masks provide real value.**

This is compact, fits your existing word tables, supports independently styled groups, and avoids making the web preview depend on LED wiring.