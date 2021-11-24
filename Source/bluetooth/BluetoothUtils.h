/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Bluetooth {

EXTERNAL int BtUtilsHciDevba(int dev_id, bdaddr_t *bdaddr);
EXTERNAL int BtUtilsHciGetRoute(bdaddr_t *bdaddr);
EXTERNAL int BtUtilsBa2Str(const bdaddr_t *ba, char *str);
EXTERNAL int BtUtilsStr2Ba(const char *str, bdaddr_t *ba);
EXTERNAL int BtUtilsBa2Oui(const bdaddr_t *ba, char *str);

inline void BtUtilsHciFilterClear(struct hci_filter *f)
{
	memset(f, 0, sizeof(*f));
}

inline void BtUtilsHciSetBit(int nr, void *addr)
{
	*((uint32_t *) addr + (nr >> 5)) |= (1 << (nr & 31));
}

inline int BtUtilsHciTestBit(int nr, void *addr)
{
	return *((uint32_t *) addr + (nr >> 5)) & (1 << (nr & 31));
}

inline void BtUtilsHciFilterSetEvent(int e, struct hci_filter *f)
{
	BtUtilsHciSetBit((e & HCI_FLT_EVENT_BITS), &f->event_mask);
}

inline void BtUtilsHciFilterSetPtype(int t, struct hci_filter *f)
{
	BtUtilsHciSetBit((t == HCI_VENDOR_PKT) ? 0 : (t & HCI_FLT_TYPE_BITS), &f->type_mask);
}

} // namespace Bluetooth

} // namespace WPEFramework
