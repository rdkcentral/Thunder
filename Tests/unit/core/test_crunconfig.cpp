#include <gtest/gtest.h>

#include "../../../Source/addons/processcontainers/implementations/CRunImplementation/CRunConfig.h"

namespace Thunder {
namespace Tests {
namespace {

    // Minimal OCI pointer graph: these are only the fields accessed by the
    // templated helper used by the actual libcrun startup implementation.
    struct Root {
        char* path;
    };
    struct Process { };
    struct Definition {
        Root* root;
        Process* process;
    };
    struct Container {
        Definition* container_def;
    };

    // PUBLIC_INTERFACE
    /** Own a test rootfs allocation independently of pointer-graph mutations. */
    struct Config {
        Root root { nullptr };
        Process process;
        Definition definition { &root, &process };
        Container container { &definition };

        // PUBLIC_INTERFACE
        /** Allocate a rootfs path compatible with the helper's realloc contract. */
        explicit Config(const char* path)
        {
            root.path = static_cast<char*>(std::malloc(std::strlen(path) + 1));
            if (root.path != nullptr) {
                std::strcpy(root.path, path);
            }
        }
        // PUBLIC_INTERFACE
        /** Release the original or successfully replaced rootfs allocation. */
        ~Config()
        {
            std::free(root.path);
        }
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;
    };

    // PUBLIC_INTERFACE
    /** Simulate realloc failure while leaving the original block allocated. */
    void* FailResize(void*, size_t)
    {
        return nullptr;
    }

} // namespace

TEST(Core_CRunConfig, RejectsIncompleteLoaderOutput)
{
    using ProcessContainers::Detail::PrepareRootfs;
    EXPECT_FALSE(PrepareRootfs(static_cast<Container*>(nullptr), "/bundle"));
    Container missing { nullptr };
    EXPECT_FALSE(PrepareRootfs(&missing, "/bundle"));

    Config config("rootfs");
    ASSERT_NE(nullptr, config.root.path);
    config.definition.root = nullptr;
    EXPECT_FALSE(PrepareRootfs(&config.container, "/bundle"));
    config.definition.root = &config.root;
    config.definition.process = nullptr;
    EXPECT_FALSE(PrepareRootfs(&config.container, "/bundle"));
    config.definition.process = &config.process;

    char* original = config.root.path;
    config.root.path = nullptr;
    EXPECT_FALSE(PrepareRootfs(&config.container, "/bundle"));
    config.root.path = original;
    EXPECT_STREQ("rootfs", config.root.path);

    Config empty("");
    ASSERT_NE(nullptr, empty.root.path);
    EXPECT_FALSE(PrepareRootfs(&empty.container, "/bundle"));
}

TEST(Core_CRunConfig, NormalizesRelativeAndPreservesAbsolutePath)
{
    Config relative("rootfs");
    ASSERT_NE(nullptr, relative.root.path);
    ASSERT_TRUE(ProcessContainers::Detail::PrepareRootfs(&relative.container, "/bundle"));
    EXPECT_STREQ("/bundle/rootfs", relative.root.path);

    Config absolute("/existing/rootfs");
    ASSERT_NE(nullptr, absolute.root.path);
    char* original = absolute.root.path;
    // Absolute paths must not attempt reallocation.
    EXPECT_TRUE(ProcessContainers::Detail::PrepareRootfs(&absolute.container, "/bundle", FailResize));
    EXPECT_EQ(original, absolute.root.path);
    EXPECT_STREQ("/existing/rootfs", absolute.root.path);
}

TEST(Core_CRunConfig, AllocationFailurePreservesOriginalBlock)
{
    Config config("rootfs");
    ASSERT_NE(nullptr, config.root.path);
    char* original = config.root.path;
    EXPECT_FALSE(ProcessContainers::Detail::PrepareRootfs(&config.container, "/bundle", FailResize));
    EXPECT_EQ(original, config.root.path);
    EXPECT_STREQ("rootfs", config.root.path);
    // Retrying after failure remains safe because the old pointer was retained.
    ASSERT_TRUE(ProcessContainers::Detail::PrepareRootfs(&config.container, "/bundle"));
    EXPECT_STREQ("/bundle/rootfs", config.root.path);
}

} // namespace Tests
} // namespace Thunder
