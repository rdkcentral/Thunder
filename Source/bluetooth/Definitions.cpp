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

#include "Module.h"

namespace WPEFramework {

namespace Bluetooth {

    /* LMP features mapping */
    struct ConversionTable {
        uint16_t id;
        const TCHAR* text;
    };

    static ConversionTable lmp_features_map[] = {
        /* Byte 0 */
        { LMP_3SLOT | 0x0000, _T("3-slot packets") }, /* Bit 0 */
        { LMP_5SLOT | 0x0000, _T("5-slot packets") }, /* Bit 1 */
        { LMP_ENCRYPT | 0x0000, _T("encryption") }, /* Bit 2 */
        { LMP_SOFFSET | 0x0000, _T("slot offset") }, /* Bit 3 */
        { LMP_TACCURACY | 0x0000, _T("timing accuracy") }, /* Bit 4 */
        { LMP_RSWITCH | 0x0000, _T("role switch") }, /* Bit 5 */
        { LMP_HOLD | 0x0000, _T("hold mode") }, /* Bit 6 */
        { LMP_SNIFF | 0x0000, _T("sniff mode") }, /* Bit 7 */

        /* Byte 1 */
        { LMP_PARK | 0x0100, _T("park state") }, /* Bit 0 */
        { LMP_RSSI | 0x0100, _T("RSSI") }, /* Bit 1 */
        { LMP_QUALITY | 0x0100, _T("channel quality") }, /* Bit 2 */
        { LMP_SCO | 0x0100, _T("SCO link") }, /* Bit 3 */
        { LMP_HV2 | 0x0100, _T("HV2 packets") }, /* Bit 4 */
        { LMP_HV3 | 0x0100, _T("HV3 packets") }, /* Bit 5 */
        { LMP_ULAW | 0x0100, _T("u-law log") }, /* Bit 6 */
        { LMP_ALAW | 0x0100, _T("A-law log") }, /* Bit 7 */

        /* Byte 2 */
        { LMP_CVSD | 0x0200, _T("CVSD") }, /* Bit 0 */
        { LMP_PSCHEME | 0x0200, _T("paging scheme") }, /* Bit 1 */
        { LMP_PCONTROL | 0x0200, _T("power control") }, /* Bit 2 */
        { LMP_TRSP_SCO | 0x0200, _T("transparent SCO") }, /* Bit 3 */
        { LMP_BCAST_ENC | 0x0200, _T("broadcast encrypt") }, /* Bit 7 */

        /* Byte 3 */
        { LMP_EDR_ACL_2M | 0x0300, _T("EDR ACL 2 Mbps") }, /* Bit 1 */
        { LMP_EDR_ACL_3M | 0x0300, _T("EDR ACL 3 Mbps") }, /* Bit 2 */
        { LMP_ENH_ISCAN | 0x0300, _T("enhanced iscan") }, /* Bit 3 */
        { LMP_ILACE_ISCAN | 0x0300, _T("interlaced iscan") }, /* Bit 4 */
        { LMP_ILACE_PSCAN | 0x0300, _T("interlaced pscan") }, /* Bit 5 */
        { LMP_RSSI_INQ | 0x0300, _T("inquiry with RSSI") }, /* Bit 6 */
        { LMP_ESCO | 0x0300, _T("extended SCO") }, /* Bit 7 */

        /* Byte 4 */
        { LMP_EV4 | 0x0400, _T("EV4 packets") }, /* Bit 0 */
        { LMP_EV5 | 0x0400, _T("EV5 packets") }, /* Bit 1 */
        { LMP_AFH_CAP_SLV | 0x0400, _T("AFH cap. slave") }, /* Bit 3 */
        { LMP_AFH_CLS_SLV | 0x0400, _T("AFH class. slave") }, /* Bit 4 */
        { LMP_NO_BREDR | 0x0400, _T("BR/EDR not supp.") }, /* Bit 5 */
        { LMP_LE | 0x0400, _T("LE support") }, /* Bit 6 */
        { LMP_EDR_3SLOT | 0x0400, _T("3-slot EDR ACL") }, /* Bit 7 */

        /* Byte 5 */
        { LMP_EDR_5SLOT | 0x0500, _T("5-slot EDR ACL") }, /* Bit 0 */
        { LMP_SNIFF_SUBR | 0x0500, _T("sniff subrating") }, /* Bit 1 */
        { LMP_PAUSE_ENC | 0x0500, _T("pause encryption") }, /* Bit 2 */
        { LMP_AFH_CAP_MST | 0x0500, _T("AFH cap. master") }, /* Bit 3 */
        { LMP_AFH_CLS_MST | 0x0500, _T("AFH class. master") }, /* Bit 4 */
        { LMP_EDR_ESCO_2M | 0x0500, _T("EDR eSCO 2 Mbps") }, /* Bit 5 */
        { LMP_EDR_ESCO_3M | 0x0500, _T("EDR eSCO 3 Mbps") }, /* Bit 6 */
        { LMP_EDR_3S_ESCO | 0x0500, _T("3-slot EDR eSCO") }, /* Bit 7 */

        /* Byte 6 */
        { LMP_EXT_INQ | 0x0600, _T("extended inquiry") }, /* Bit 0 */
        { LMP_LE_BREDR | 0x0600, _T("LE and BR/EDR") }, /* Bit 1 */
        { LMP_SIMPLE_PAIR | 0x0600, _T("simple pairing") }, /* Bit 3 */
        { LMP_ENCAPS_PDU | 0x0600, _T("encapsulated PDU") }, /* Bit 4 */
        { LMP_ERR_DAT_REP | 0x0600, _T("err. data report") }, /* Bit 5 */
        { LMP_NFLUSH_PKTS | 0x0600, _T("non-flush flag") }, /* Bit 6 */

        /* Byte 7 */
        { LMP_LSTO | 0x0700, _T("LSTO") }, /* Bit 1 */
        { LMP_INQ_TX_PWR | 0x0700, _T("inquiry TX power") }, /* Bit 2 */
        { LMP_EPC | 0x0700, _T("EPC") }, /* Bit 2 */
        { LMP_EXT_FEAT | 0x0700, _T("extended features") }, /* Bit 7 */

        { 0x0000, nullptr }
    };

    const TCHAR* FeatureToText(const uint16_t index)
    {
        ConversionTable* pos = lmp_features_map;

        while ((pos->text != nullptr) && (pos->id != index)) {
            pos++;
        }

        return (pos->text != nullptr ? pos->text : _T("reserved"));
    }

} } // namespace WPEFramework::Bluetooth

