 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef __RECTANGLE_H
#define __RECTANGLE_H

#include <algorithm>
#ifdef WIN32
#include <xutility>
#endif

#include "Module.h"

namespace Thunder {
namespace Core {

    class EXTERNAL Rectangle {
    public:
        Rectangle()
            : x(0)
            , y(0)
            , w(0)
            , h(0)
        {
        }
        Rectangle(int _x, int _y, int _w, int _h)
            : x(_x)
            , y(_y)
            , w(_w)
            , h(_h)
        {
        }

        int x, y, w, h;

        bool operator==(const Rectangle& rhs) const
        {
            return x == rhs.x && y == rhs.y && w == rhs.w && h == rhs.h;
        }
        bool operator!=(const Rectangle& rhs) const
        {
            return x != rhs.x || y != rhs.y || w != rhs.w || h != rhs.h;
        }

        // Combine 2 rects to form the total enclosed area.
        Rectangle& combine(const Rectangle& r)
        {
            int left = std::min(x, r.x);
            int top = std::min(y, r.y);
            int right = std::max(x + w, r.x + r.w);
            int bottom = std::max(y + h, r.y + r.h);
            x = left;
            y = top;
            w = right - left;
            h = bottom - top;
            return *this;
        }

        Rectangle operator&(const Rectangle& r)
        {
            Rectangle retVal;
            // Either in Y direction OR in X direction they must completely not overlap
            const int minx1 = x;
            const int minx2 = r.x;
            const int maxx1 = x + w - 1;
            const int maxx2 = r.x + r.w - 1;
            if (maxx1 < minx2 || maxx2 < minx1) {
                return retVal;
            }

            const int miny1 = y;
            const int miny2 = r.y;
            const int maxy1 = y + h - 1;
            const int maxy2 = r.y + r.h - 1;
            if (maxy1 < miny2 || maxy2 < miny1) {
                return retVal;
            }

            retVal.x = std::max(minx1, minx2);
            retVal.w = 1 + std::min(maxx1, maxx2) - retVal.x;
            retVal.y = std::max(miny1, miny2);
            retVal.h = 1 + std::min(maxy1, maxy2) - retVal.y;
            return retVal;
        }

        bool Overlaps(const Rectangle& r)
        {
            // Either in Y direction OR in X direction they must completely not overlap
            const int minx1 = x;
            const int minx2 = r.x;
            const int maxx1 = x + w - 1;
            const int maxx2 = r.x + r.w - 1;
            if (maxx1 < minx2 || maxx2 < minx1) {
                return false;
            }

            const int miny1 = y;
            const int miny2 = r.y;
            const int maxy1 = y + h - 1;
            const int maxy2 = r.y + r.h - 1;
            if (maxy1 < miny2 || maxy2 < miny1) {
                return false;
            }

            return true;
        }

        // Alternative combine
        Rectangle operator|(const Rectangle& rhs)
        {
            Rectangle tmp(*this);
            return tmp.combine(rhs);
        }

        bool Contains(int tx, int ty) const
        {
            int dx = tx - x;
            int dy = ty - y;
            return dx >= 0 && dx < w && dy >= 0 && dy < h;
        }
    };
}
} // namespace Core

#endif // __RECTANGLE_H
