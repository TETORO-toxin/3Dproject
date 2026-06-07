#pragma once
#include "DxLib.h"
#include <vector>

class NavMesh
{
public:
    struct Cell
    {
        bool walkable = true;
    };

    void Initialize(int width, int height, float cellSize, VECTOR origin);
    void Clear(bool walkable = true);

    void SetBlocked(int x, int z, bool blocked);
    bool IsWalkable(int x, int z) const;

    bool WorldToCell(const VECTOR& pos, int& x, int& z) const;
    VECTOR CellToWorld(int x, int z) const;

    std::vector<VECTOR> FindPath(const VECTOR& start, const VECTOR& goal) const;

    void DebugDraw() const;

private:
    int width_ = 0;
    int height_ = 0;
    float cellSize_ = 1.0f;
    VECTOR origin_ = VGet(0, 0, 0);
    std::vector<Cell> cells_;

    int Index(int x, int z) const { return z * width_ + x; }
    bool InRange(int x, int z) const { return x >= 0 && z >= 0 && x < width_ && z < height_; }
};
