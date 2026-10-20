/*
 * BitMatrix.h
 *
 *  Created on: 14.10.2025
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>


class BitMatrix
{
public:
    BitMatrix(uint16_t aWidth, const uint16_t aHeight)
        : mWidth(aWidth), mHeight(aHeight)
    {
        /* Calculate number of bits and array size */
        mNumbOfBits = static_cast<uint32_t>(mWidth) * mHeight;
        mArraySize  = (mNumbOfBits / mBitsInUint8) + ((mNumbOfBits % mBitsInUint8) != 0);

        /* Create array of uint8_t elements */
        mpArray = new uint8_t[mArraySize];

        /* Clear all bits in the array */
        ClearAll();
    }

    /**
     * @brief Disable copy construction.
     * @details BitMatrix owns its bit array, so copying the pointer would cause
     *          multiple objects to manage the same memory.
     */
    BitMatrix(const BitMatrix&) = delete;

    /**
     * @brief Disable copy assignment.
     * @details Prevents shallow-copying the owned bit array.
     */
    BitMatrix& operator=(const BitMatrix&) = delete;

    ~BitMatrix()
    {
        delete[] mpArray;
        mpArray = nullptr;
    }

    /** @brief Get number of emenents */
    uint32_t GetSize(void) const
    {
        return mNumbOfBits;
    };

    /** @brief Get matrix width */
    uint16_t GetWidth(void) const
    {
        return mWidth;
    };

    /** @brief Get matrix height */
    uint16_t GetHeight(void) const
    {
        return mHeight;
    };

    /** @brief Set all (sets every bit in the bit array to 1) */
    void SetAll(void)
    {
        for (uint32_t wI = 0; wI < mArraySize; wI++)
        {
            mpArray[wI] = 0xFF;
        }

        /* Clear unused padding bits in the last byte */
        const uint8_t wRemainder = mNumbOfBits % mBitsInUint8;
        if (wRemainder != 0)
        {
            mpArray[mArraySize - 1] = static_cast<uint8_t>((1U << wRemainder) - 1U);
        }
    }

    /** @brief Clear all (sets every bit in the bit array to 0) */
    void ClearAll(void)
    {
        for (uint32_t wI = 0; wI < mArraySize; wI++)
        {
            mpArray[wI] = 0;
        }
    }

    /** @brief Checks if the bit at the specified index is set (1)
     *
     * @param aIndex The index of the bit to check
     * @return true if the bit is set (1), false if the bit is clear (0)
     */
    bool IsBitSet(const uint32_t aIndex) const
    {
        uint32_t wByteOffset;
        uint32_t wBitIndex;
        bool     wBitIsSet = false;

        if (aIndex < mNumbOfBits)
        {
            wByteOffset = (aIndex / mBitsInUint8);
            wBitIndex   = (aIndex % mBitsInUint8);

            wBitIsSet = (*(static_cast<uint8_t*>(mpArray + wByteOffset)) & static_cast<uint8_t>(1 << wBitIndex)) != 0;
        }

        return wBitIsSet;
    }

    /** @brief Check if a bit is set in the matrix at specified row and column
     *
     * @param aRow The row of the bit to check
     * @param aCol The column of the bit to check
     * @return true if the bit is set (1), false if the bit is clear (0)
     */
    bool IsBitSet(const uint16_t aRow, const uint16_t aCol) const
    {
        bool wBitIsSet = false;

        /* Check input parameters */
        if ((aRow < mHeight) &&
            (aCol < mWidth))
        {
            /* Calculate bit index */
            uint32_t wBitIndex = (static_cast<uint32_t>(aRow) * mWidth) + aCol;
            /* Get bit state */
            wBitIsSet = IsBitSet(wBitIndex);
        }

        return wBitIsSet;
    }

    /** @brief Clear bit at specific position in bit array
     *
     * @param aIndex The index of the bit to clear
     */
    void ClearBit(const uint32_t aIndex)
    {
        uint32_t wByteOffset;
        uint32_t wBitIndex;

        if (aIndex < mNumbOfBits)
        {
            wByteOffset = (aIndex / mBitsInUint8);
            wBitIndex   = (aIndex % mBitsInUint8);

            *(static_cast<uint8_t*>(mpArray + wByteOffset)) &= ~(static_cast<uint8_t>(1 << wBitIndex));
        }
    }

    /** @brief Set the bit at a specific position in bit array */
    void SetBit(const uint32_t aIndex)
    {
        uint32_t wByteOffset;
        uint32_t wBitIndex;

        if (aIndex < mNumbOfBits)
        {
            wByteOffset = (aIndex / mBitsInUint8);
            wBitIndex   = (aIndex % mBitsInUint8);

            *(static_cast<uint8_t*>(mpArray + wByteOffset)) |= static_cast<uint8_t>(1 << wBitIndex);
        }
    }

    /** @brief Set a bit in the matrix at specified row and column
     *
     * @param aRow The row of the bit to set
     * @param aCol The column of the bit to set
    */
    void SetBit(const uint16_t aRow, const uint16_t aCol)
    {
        /* Check input parameters */
        if ((aRow < mHeight) &&
            (aCol < mWidth))
        {
            /* Calculate bit index */
            uint32_t wBitIndex = (static_cast<uint32_t>(aRow) * mWidth) + aCol;
            /* Set bit */
            SetBit(wBitIndex);
        }
    }

    /** @brief Set a line of bits in the matrix
     *
     * @param aRow The row of the line to set
     * @param aCol The starting column of the line to set
     * @param aLength The length of the line to set
     */
    void SetLine(const uint16_t aRow, const uint16_t aCol, const uint16_t aLength)
    {
        /* Check input parameters */
        if ((aLength > 0) && (aRow < mHeight) &&
            (aCol < mWidth)  && ((aCol + aLength) <= mWidth))
        {
            for (uint16_t wI = 0; wI < aLength; wI++)
            {
                SetBit(aRow, (aCol + wI));
            }
        }
    }

    /** @brief Set a rectangular area of bits in the matrix
     *
     * @param aRow The starting row of the area to set
     * @param aCol The starting column of the area to set
     * @param aWidth The width of the area to set
     * @param aHeight The height of the area to set
     */
    void SetArea(const uint16_t aRow, const uint16_t aCol, const uint16_t aWidth, const uint16_t aHeight)
    {
        /* Check input parameters */
        if ((aRow < mHeight) && ((aRow + aHeight) <= mHeight) &&
            (aCol < mWidth)  && ((aCol + aWidth)  <= mWidth))
        {
            for (uint16_t wI = 0; wI < aHeight; wI++)
            {
                SetLine((aRow + wI), aCol, aWidth);
            }
        }
    }

    /** @brief Flip a specific row horizontally (left-to-right)
     *
     * @param aRow The row to flip
     */
    void FlipRow(const uint16_t aRow)
    {
        if (aRow < mHeight)
        {
            for (uint16_t wCol = 0; wCol < mWidth / 2; wCol++)
            {
                uint32_t leftIndex  = static_cast<uint32_t>(aRow) * mWidth + wCol;
                uint32_t rightIndex = static_cast<uint32_t>(aRow) * mWidth + (mWidth - 1 - wCol);

                bool leftBit  = IsBitSet(leftIndex);
                bool rightBit = IsBitSet(rightIndex);

                /* Swap bits if they are different */
                if (leftBit != rightBit)
                {
                    if (leftBit)
                    {
                        SetBit(rightIndex);
                        ClearBit(leftIndex);
                    }
                    else
                    {
                        SetBit(leftIndex);
                        ClearBit(rightIndex);
                    }
                }
            }
        }
    }

    /** @brief Flip a specific column vertically (top-to-bottom)
     *
     * @param aColumn The column to flip
     */
    void FlipColumn(const uint16_t aColumn)
    {
        if (aColumn < mWidth)
        {
            for (uint16_t wRow = 0; wRow < mHeight / 2; wRow++)
            {
                uint32_t topIndex    = static_cast<uint32_t>(wRow) * mWidth + aColumn;
                uint32_t bottomIndex = static_cast<uint32_t>(mHeight - 1 - wRow) * mWidth + aColumn;

                bool topBit    = IsBitSet(topIndex);
                bool bottomBit = IsBitSet(bottomIndex);

                /* Swap bits if they are different */
                if (topBit != bottomBit)
                {
                    if (topBit)
                    {
                        SetBit(bottomIndex);
                        ClearBit(topIndex);
                    }
                    else
                    {
                        SetBit(topIndex);
                        ClearBit(bottomIndex);
                    }
                }
            }
        }
    }

    /** @brief Flip the matrix horizontally (left-to-right) */
    void FlipHorizontal(void)
    {
        for (uint16_t wRow = 0; wRow < mHeight; wRow++)
        {
            FlipRow(wRow);
        }
    }

    /** @brief Flip the matrix vertically (top-to-bottom) */
    void FlipVertical(void)
    {
        for (uint16_t col = 0; col < mWidth; ++col)
        {
            FlipColumn(col);
        }
    }

    /** @brief Set the matrix to zigzag order
     *
     * @param aFlipEvenRows If true, flip even rows; otherwise, flip odd rows
     */
    void SetZigzagOrder(bool aFlipEvenRows = true)
    {
        for (uint16_t wRow = 0; wRow < mHeight; wRow++)
        {
            /* Flip even rows when requested; otherwise flip odd rows */
            if ((wRow % 2) == (aFlipEvenRows ? 0 : 1))
            {
                FlipRow(wRow);
            }
        }
    }

    /** @brief Copy the contents of another BitMatrix into this one
     *
     * @param aOther The other BitMatrix to copy from
     */
    void Copy(const BitMatrix& aOther)
    {
        if ((this != &aOther) &&                // Check for self-assignment
            (mWidth  == aOther.mWidth) &&
            (mHeight == aOther.mHeight))
        {
            /* Copy array using memcpy function */
            memcpy(mpArray, aOther.mpArray, mArraySize);
        }
    }

    /** @brief Perform a union operation with another BitMatrix
     *
     * @param aOther The other BitMatrix to union with
     */
    void Union(const BitMatrix& aOther)
    {
        if ((this != &aOther) &&                // Check for self-assignment
            (mWidth == aOther.mWidth) &&
            (mHeight == aOther.mHeight))
        {
            /* Set bits that are set in either matrix */
            for (uint32_t wI = 0; wI < mArraySize; wI++)
            {
                mpArray[wI] |= aOther.mpArray[wI];
            }
        }
    }
    /** @brief Perform an intersection operation with another BitMatrix
     *
     * @param aOther The other BitMatrix to intersect with
     */
    void Intersect(const BitMatrix& aOther)
    {
        if ((this != &aOther) &&                // Check for self-assignment
            (mWidth == aOther.mWidth) &&
            (mHeight == aOther.mHeight))
        {
            /* Keep only bits that are set in both matrices */
            for (uint32_t wI = 0; wI < mArraySize; wI++)
            {
                mpArray[wI] &= aOther.mpArray[wI];
            }
        }
    }


    /** @brief Perform a difference operation with another BitMatrix
     *
     * @details This operation will clear bits in this matrix that are set in the other matrix.
     *          When aOther is the same matrix, this sets every byte to zero, which is the expected
     *          result of a matrix minus itself.
     *
     * @param aOther The other BitMatrix to differ with
     */
    void Difference(const BitMatrix& aOther)
    {
        if ((mWidth  == aOther.mWidth) &&
            (mHeight == aOther.mHeight))
        {
            /* Set bits that are set in aOther to 0 in this matrix */
            for (uint32_t wI = 0; wI < mArraySize; wI++)
            {
                mpArray[wI] &= ~(aOther.mpArray[wI]);
            }
        }
    }

    /** @brief Equality operator overload
     *
     * @param aOther The other BitMatrix to compare with
     * @return true if the matrices are equal, false otherwise
     */
    bool operator==(const BitMatrix& aOther) const
    {
        return ((mWidth  == aOther.mWidth)  &&
                (mHeight == aOther.mHeight) &&
                (memcmp(mpArray, aOther.mpArray, mArraySize) == 0));
    }

    /** @brief Inequality operator overload
     *
     * @param aOther The other BitMatrix to compare with
     * @return true if the matrices are not equal, false otherwise
     */
    bool operator!=(const BitMatrix& aOther) const
    {
        return !(*this == aOther);
    }

private:

    /* Number of bits in uint8_t type */
    static constexpr uint8_t mBitsInUint8 = 8U;

    /** @brief Width and heightof the BitMatrix */
    uint16_t mWidth;
    uint16_t mHeight;

    /* Number of bits in the array */
    uint32_t mNumbOfBits;

    /* Size of byte array */
    uint32_t mArraySize;
    /* Pointer to the byte array */
	uint8_t* mpArray;
};
