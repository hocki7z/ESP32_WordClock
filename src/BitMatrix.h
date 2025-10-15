/*
 * BitMatrix.h
 *
 *  Created on: 14.10.2025
 *      Author: hocki
 */
#ifndef SRC_BITMATRIX_H_
#define SRC_BITMATRIX_H_

#include <Arduino.h>

class BitMatrix
{
public:
    BitMatrix(uint16_t aWidth, const uint16_t aHeight)
        : mWidth(aWidth), mHeight(aHeight)
    {
        /* Calculate number of bits and array size */
        mNumbOfBits = mWidth * mHeight;
        mArraySize = (mNumbOfBits + mBitsInUint8 - 1) / mBitsInUint8;

        /* Create array of uint8_t elements */
        mpArray = new uint8_t[mArraySize];
    }

    virtual ~BitMatrix()
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
    }

    /** @brief Clear all (sets every bit in the bit array to 0) */
    void ClearAll(void)
    {
        for (uint32_t wI = 0; wI < mArraySize; wI++)
        {
            mpArray[wI] = 0;
        }
    }

    /** @brief Checks if the bit at the specified index is set (1) */
    bool IsBitSet(const uint32_t aIndex)
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

    /** @brief Check if a bit is set in the matrix at specified row and column */
    bool IsBitSet(const uint16_t aRow, const uint16_t aCol)
    {
        bool wBitIsSet = false;

        /* Check input parameters */
        if ((aRow < mHeight) &&
            (aCol < mWidth))
        {
            /* Calculate bit index */
            uint32_t wBitIndex = (aRow * mHeight) + aCol;
            /* Get bit state */
            wBitIsSet = IsBitSet(wBitIndex);
        }

        return wBitIsSet;
    }

    /** @brief Clear bit at specific position in bit array */
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

    /** @brief Set a bit in the matrix at specified row and column */
    void SetBit(const uint16_t aRow, const uint16_t aCol)
    {
        /* Check input parameters */
        if ((aRow < mHeight) &&
            (aCol < mWidth))
        {
            /* Calculate bit index */
            uint32_t wBitIndex = (aRow * mHeight) + aCol;
            /* Set bit */
            SetBit(wBitIndex);
        }
    }

    /** @brief Set a line of bits in the matrix */
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

    /** @brief Set a rectangular area of bits in the matrix */
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

    /** @brief Flip a specific row horizontally (left-to-right) */
    void FlipRow(const uint16_t aRow)
    {
        for (uint16_t wCol = 0; wCol < mWidth / 2; wCol++)
        {
            uint32_t leftIndex  = aRow * mWidth + wCol;
            uint32_t rightIndex = aRow * mWidth + (mWidth - 1 - wCol);

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

    /** @brief Flip a specific column vertically (top-to-bottom) */
    void FlipColumn(const uint16_t aColumn)
    {
        for (uint16_t wRow = 0; wRow < mHeight / 2; wRow++)
        {
            uint32_t topIndex    = wRow * mWidth + aColumn;
            uint32_t bottomIndex = (mHeight - 1 - wRow) * mWidth + aColumn;

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

    void Copy(const BitMatrix& aOther)
    {
        if ((mWidth  == aOther.mWidth) &&
            (mHeight == aOther.mHeight))
        {
            /* Copy array using memcpy function */
            memcpy(mpArray, aOther.mpArray, mArraySize);
        }
    }

    void Union(const BitMatrix& aOther)
    {
        if ((mWidth == aOther.mWidth) &&
            (mHeight == aOther.mHeight))
        {
            /* Set bits that are set in either matrix */
            for (uint32_t wI = 0; wI < mArraySize; wI++)
            {
                mpArray[wI] |= aOther.mpArray[wI];
            }
        }
    }
    void Intersect(const BitMatrix& aOther)
    {
        if ((mWidth == aOther.mWidth) &&
            (mHeight == aOther.mHeight))
        {
            /* Keep only bits that are set in both matrices */
            for (uint32_t wI = 0; wI < mArraySize; wI++)
            {
                mpArray[wI] &= aOther.mpArray[wI];
            }
        }
    }


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

    /* Operator == overload */
    bool operator==(const BitMatrix& aOther) const
    {
        return ((mWidth  == aOther.mWidth)  &&
                (mHeight == aOther.mHeight) &&
                (memcmp(mpArray, aOther.mpArray, mArraySize) == 0));
    }

    /* Operator != overload */
    bool operator!=(const BitMatrix& aOther) const
    {
        return !(*this == aOther);
    }

private:

    /* Number of bits in uint8_t type */
    static constexpr uint8_t mBitsInUint8 = 8U;

    uint16_t mWidth;
    uint16_t mHeight;

    /* Number of bits in the array */
    uint32_t mNumbOfBits;

    /* Size of byte array */
    uint32_t mArraySize;
    /* Pointer to the byte array */
	uint8_t* mpArray;
};

#endif /* SRC_BITARRAY_H_ */