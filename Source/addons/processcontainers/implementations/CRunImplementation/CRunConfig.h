#pragma once

#include <cstdlib>
#include <cstring>
#include <string>

namespace Thunder {
namespace ProcessContainers {
namespace Detail {

    // PUBLIC_INTERFACE
    /**
     * Validate a loaded OCI pointer graph and normalize its rootfs path.
     * container must expose container_def, root and process as libcrun does.
     * bundle prefixes relative paths; resize is realloc-compatible and can be
     * injected for allocation-failure tests. Returns false without replacing
     * the original allocation when validation or reallocation fails.
     */
    template <typename CONTAINER>
    bool PrepareRootfs(CONTAINER* container, const std::string& bundle,
        void* (*resize)(void*, size_t) = &std::realloc)
    {
        // Validate every level before accessing a loader-owned pointer.
        if ((container == nullptr) || (container->container_def == nullptr)
            || (container->container_def->root == nullptr)
            || (container->container_def->process == nullptr)
            || (container->container_def->root->path == nullptr)
            || (container->container_def->root->path[0] == '\0')
            || (resize == nullptr)) {
            return false;
        }

        auto* root = container->container_def->root;
        if (root->path[0] != '/') {
            const std::string path = bundle + "/" + root->path;
            // realloc failure leaves the old block valid; commit only success.
            char* replacement = static_cast<char*>(resize(root->path, path.size() + 1));
            if (replacement == nullptr) {
                return false;
            }
            root->path = replacement;
            std::memcpy(root->path, path.c_str(), path.size() + 1);
        }
        return true;
    }

} // namespace Detail
} // namespace ProcessContainers
} // namespace Thunder
