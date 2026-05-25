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

#ifndef __CONFIG_FRAMEWORKSUPPORT_H
#define __CONFIG_FRAMEWORKSUPPORT_H

// This header file should be used to turn on and off non-functional code.
// Non-functional code is considered to be code that the application can
// do without. Examples are performance counter code, in the framework
// the number of request being processed are being counted. For the product
// this has no added value, other then gathering statistics on its use,
// mainly intersting for the developper to take the proper optimization
// decisions for the code.

#define THUNDER_RUNTIME_STATISTICS 0

#define THUNDER_RESTFULL_API 1

#endif
