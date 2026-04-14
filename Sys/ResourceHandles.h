#pragma once
#include "DxLib.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>

// Centralized resource handle manager for MV1 models, textures and sound
class ResourceHandles
{
public:
    ResourceHandles();
    ~ResourceHandles();

    // Models (.mv1)
    int LoadModel(const std::string& path);
    int GetModel(const std::string& path) const;

    // Textures / 2D images
    int LoadTexture(const std::string& path);
    int GetTexture(const std::string& path) const;

    // Sound effects / BGM
    int LoadSound(const std::string& path);
    int GetSound(const std::string& path) const;

    // Unload everything (called from destructor as well)
    void UnloadAll();

    // Async helper: starts loading model in background and registers it when ready.
    // Note: MV1LoadModel may not be thread-safe on all platforms; use with caution.
    void AsyncLoadModel(const std::string& path);

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string,int> models_;
    std::unordered_map<std::string,int> textures_;
    std::unordered_map<std::string,int> sounds_;
    // worker threads for async loading. Joined in destructor to ensure safe shutdown.
    std::vector<std::thread> workers_;
    mutable std::mutex workersMutex_;
};
