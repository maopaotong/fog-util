/*
 * SPDX-FileCopyrightText: 2025 Mao-Pao-Tong Workshop
 * SPDX-License-Identifier: MPL-2.0
 */
#pragma once

namespace fog
{
    // using namespace Ogre;

    template <typename T>
    struct Range2
    {

        T x1;
        T y1;
        T x2;
        T y2;

        Range2() = default;

        Range2(T w) : Range2(w, w)
        {
        }

        Range2(T w, T h) : Range2(0, 0, w, h)
        {
        }

        Range2(T x1, T y1, T x2, T y2) : x1(x1), y1(y1), x2(x2), y2(y2)
        {
        }

        T getWidth()
        {
            return x2 - x1;
        }

        T getHeight()
        {
            return y2 - y1;
        }
    };
}; // end of namespace
