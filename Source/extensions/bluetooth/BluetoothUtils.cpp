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

#include "BluetoothUtils.h"

namespace Thunder {

namespace Bluetooth {

int BtUtilsHciForEachDevice(int flag, int (*func)(int dd, int dev_id, long arg),
	long arg)
{
	struct hci_dev_list_req* dl;
	struct hci_dev_req* dr;
	int dev_id = -1;
	int i, sk, err = 0;

	sk = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
	if (sk < 0)
		return -1;

	dl = static_cast<hci_dev_list_req*>(malloc(HCI_MAX_DEV * sizeof(*dr) + sizeof(*dl)));
	if (!dl) {
		err = errno;
		goto done;
	}

	memset(dl, 0, HCI_MAX_DEV * sizeof(*dr) + sizeof(*dl));

	dl->dev_num = HCI_MAX_DEV;
	dr = dl->dev_req;

	if (ioctl(sk, HCIGETDEVLIST, (void*)dl) < 0) {
		err = errno;
		goto free;
	}

	for (i = 0; i < dl->dev_num; i++, dr++) {
		if (BtUtilsHciTestBit(flag, &dr->dev_opt))
			if (!func || func(sk, dr->dev_id, arg)) {
				dev_id = dr->dev_id;
				break;
			}
	}

	if (dev_id < 0)
		err = ENODEV;

free:
	free(dl);

done:
	close(sk);
	errno = err;

	return dev_id;
}

int BtUtilsHciDevInfo(int dev_id, struct hci_dev_info* di)
{
	int dd, err, ret;

	dd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
	if (dd < 0)
		return dd;

	memset(di, 0, sizeof(struct hci_dev_info));

	di->dev_id = dev_id;
	ret = ioctl(dd, HCIGETDEVINFO, (void*)di);

	err = errno;
	close(dd);
	errno = err;

	return ret;
}

int BtUtilsHciDevba(int dev_id, bdaddr_t* bdaddr)
{
	struct hci_dev_info di;

	memset(&di, 0, sizeof(di));

	if (BtUtilsHciDevInfo(dev_id, &di))
		return -1;

	if (!BtUtilsHciTestBit(HCI_UP, &di.flags)) {
		errno = ENETDOWN;
		return -1;
	}

	bacpy(bdaddr, &di.bdaddr);

	return 0;
}

int BtUtilsOtherBdaddr(int dd, int dev_id, long arg)
{
	struct hci_dev_info di;
	di.dev_id = static_cast<uint16_t>(dev_id);

	if (ioctl(dd, HCIGETDEVINFO, (void*)&di))
		return 0;

	if (BtUtilsHciTestBit(HCI_RAW, &di.flags))
		return 0;

	return bacmp((bdaddr_t*)arg, &di.bdaddr);
}

int BtUtilsSameBdaddr(int dd, int dev_id, long arg)
{
	struct hci_dev_info di;
	di.dev_id = static_cast<uint16_t>(dev_id);

	if (ioctl(dd, HCIGETDEVINFO, (void*)&di))
		return 0;

	return !bacmp((bdaddr_t*)arg, &di.bdaddr);
}

int BtUtilsHciGetRoute(bdaddr_t* bdaddr)
{
	int dev_id;
	bdaddr_t* bdaddr_any = static_cast<bdaddr_t*>(calloc(1, sizeof(*bdaddr)));

	dev_id = BtUtilsHciForEachDevice(HCI_UP, BtUtilsOtherBdaddr,
		(long)(bdaddr ? bdaddr : bdaddr_any));
	if (dev_id < 0)
		dev_id = BtUtilsHciForEachDevice(HCI_UP, BtUtilsSameBdaddr,
			(long)(bdaddr ? bdaddr : bdaddr_any));

	free(bdaddr_any);

	return dev_id;
}

int BtUtilsBa2Str(const bdaddr_t* ba, char* str)
{
	return sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
		ba->b[5], ba->b[4], ba->b[3], ba->b[2], ba->b[1], ba->b[0]);
}

int BtUtilsBachk(const char* str)
{
	if (!str)
		return -1;

	if (strlen(str) != 17)
		return -1;

	while (*str) {
		if (!isxdigit(*str++))
			return -1;

		if (!isxdigit(*str++))
			return -1;

		if (*str == 0)
			break;

		if (*str++ != ':')
			return -1;
	}

	return 0;
}

int BtUtilsStr2Ba(const char* str, bdaddr_t* ba)
{
	int i;

	if (BtUtilsBachk(str) < 0) {
		memset(ba, 0, sizeof(*ba));
		return -1;
	}

	for (i = 5; i >= 0; i--, str += 3)
		ba->b[i] = strtol(str, NULL, 16);

	return 0;
}

int BtUtilsBa2Oui(const bdaddr_t* ba, char* str)
{
	return sprintf(str, "%2.2X-%2.2X-%2.2X", ba->b[5], ba->b[4], ba->b[3]);
}

void BtUtilsBaswap(bdaddr_t* dst, const bdaddr_t* src)
{
	register unsigned char* d = (unsigned char*)dst;
	register const unsigned char* s = (const unsigned char*)src;
	register int i;

	for (i = 0; i < 6; i++)
		d[i] = s[5 - i];
}

} // namespace Bluetooth

} // namespace Thunder
