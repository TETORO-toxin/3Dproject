#pragma once
#include "DxLib.h"
#include <string>

class AssetsMgr
{
public:
    AssetsMgr();
    ~AssetsMgr();

    void Init();
    int LoadModel(const std::string& path);

private:
};
