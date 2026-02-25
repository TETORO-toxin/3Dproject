#include "Assets.h"
#include <cstdio>

AssetsMgr::AssetsMgr()
{
}

AssetsMgr::~AssetsMgr()
{
}

void AssetsMgr::Init()
{
    // nothing special for now
}

int AssetsMgr::LoadModel(const std::string& path)
{
    int handle = MV1LoadModel(path.c_str());
    if (handle == -1) {
        char buf[512];
        sprintf_s(buf, "Failed to load model: %s", path.c_str());
        DrawString(10, 60, buf, GetColor(255, 0, 0));
    }
    return handle;
}
