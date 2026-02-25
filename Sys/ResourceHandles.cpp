#include "ResourceHandles.h"
#include <cstdio>
#include <thread>

ResourceHandles::ResourceHandles()
{
}

ResourceHandles::~ResourceHandles()
{
    UnloadAll();
}

int ResourceHandles::LoadModel(const std::string& path)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = models_.find(path);
    if (it != models_.end()) return it->second;
    int h = MV1LoadModel(path.c_str());
    if (h == -1) {
        char buf[512];
        sprintf_s(buf, "ResourceHandles: failed to load model '%s'", path.c_str());
        DrawString(10, 100, buf, GetColor(255, 0, 0));
        return -1;
    }
    models_.emplace(path, h);
    return h;
}

int ResourceHandles::GetModel(const std::string& path) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = models_.find(path);
    return it != models_.end() ? it->second : -1;
}

int ResourceHandles::LoadTexture(const std::string& path)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = textures_.find(path);
    if (it != textures_.end()) return it->second;
    int h = LoadGraph(path.c_str());
    if (h == -1) {
        char buf[512];
        sprintf_s(buf, "ResourceHandles: failed to load texture '%s'", path.c_str());
        DrawString(10, 120, buf, GetColor(255, 0, 0));
        return -1;
    }
    textures_.emplace(path, h);
    return h;
}

int ResourceHandles::GetTexture(const std::string& path) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = textures_.find(path);
    return it != textures_.end() ? it->second : -1;
}

int ResourceHandles::LoadSound(const std::string& path)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = sounds_.find(path);
    if (it != sounds_.end()) return it->second;
    int h = LoadSoundMem(path.c_str());
    if (h == -1) {
        char buf[512];
        sprintf_s(buf, "ResourceHandles: failed to load sound '%s'", path.c_str());
        DrawString(10, 140, buf, GetColor(255, 0, 0));
        return -1;
    }
    sounds_.emplace(path, h);
    return h;
}

int ResourceHandles::GetSound(const std::string& path) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = sounds_.find(path);
    return it != sounds_.end() ? it->second : -1;
}

void ResourceHandles::UnloadAll()
{
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto &p : models_) {
        if (p.second != -1) MV1DeleteModel(p.second);
    }
    models_.clear();

    for (auto &p : textures_) {
        if (p.second != -1) DeleteGraph(p.second);
    }
    textures_.clear();

    for (auto &p : sounds_) {
        if (p.second != -1) DeleteSoundMem(p.second);
    }
    sounds_.clear();
}

void ResourceHandles::AsyncLoadModel(const std::string& path)
{
    // Launch a detached thread to load the model and register it when complete.
    // Caution: MV1LoadModel may not be thread-safe for DXLib; test in your environment.
    std::thread([this, path]() {
        int h = MV1LoadModel(path.c_str());
        std::lock_guard<std::mutex> lk(mutex_);
        if (h != -1) models_.emplace(path, h);
    }).detach();
}
