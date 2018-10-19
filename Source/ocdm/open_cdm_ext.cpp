#include "open_cdm_ext.h"

#include "open_cdm_impl.h"

OpenCDMError opencdm_system_get_drm_time(struct OpenCDMAccessor* system, time_t * time) {
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        *time = static_cast<OpenCDMError>(system->GetDrmSystemTime());
        result = ERROR_NONE;
    }
    return (result);
}
