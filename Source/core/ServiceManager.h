/*
 * Thunder ServiceManager - Singleton for service registration and lookup
 * Inspired by Android Binder's ServiceManager
 */
#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

namespace Thunder {
namespace IPC {

class ServiceManager {
public:
    // Get the singleton instance
    static ServiceManager& Instance();

    // Register a service by name
    bool RegisterService(const std::string& name, void* serviceHandle);

    // Query a service by name
    void* GetService(const std::string& name);

    // List all registered services
    std::unordered_map<std::string, void*> ListServices() const;

private:
    ServiceManager();
    ~ServiceManager();
    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;

    mutable std::mutex _lock;
    std::unordered_map<std::string, void*> _services;
};

} // namespace IPC
} // namespace Thunder
