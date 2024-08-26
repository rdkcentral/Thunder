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

#ifndef __WEBTRANSFORM_H
#define __WEBTRANSFORM_H

#include "Module.h"

namespace Thunder {
namespace Web {
    class NoTransform {
    public:
        NoTransform(const NoTransform&) = delete;
        NoTransform& operator=(const NoTransform&) = delete;

    public:
        inline NoTransform()
        {
        }
        inline ~NoTransform()
        {
        }
    };
}
}

#endif // __WEBTRANSFORM_H
