#include "NavMesh.h"
#include <queue>
#include <limits>
#include <cmath>

void NavMesh::Initialize(int width, int height, float cellSize, VECTOR origin)
{
    width_ = width;
    height_ = height;
    cellSize_ = cellSize;
    origin_ = origin;
    cells_.clear();
    cells_.resize(width_ * height_);
}

void NavMesh::Clear(bool walkable)
{
    for (auto &c : cells_) c.walkable = walkable;
}

void NavMesh::SetBlocked(int x, int z, bool blocked)
{
    if (!InRange(x, z)) return;
    cells_[Index(x,z)].walkable = !blocked;
}

bool NavMesh::IsWalkable(int x, int z) const
{
    if (!InRange(x, z)) return false;
    return cells_[Index(x,z)].walkable;
}

bool NavMesh::WorldToCell(const VECTOR& pos, int& x, int& z) const
{
    float lx = pos.x - origin_.x;
    float lz = pos.z - origin_.z;
    int cx = (int)floor(lx / cellSize_);
    int cz = (int)floor(lz / cellSize_);
    if (!InRange(cx, cz)) return false;
    x = cx; z = cz;
    return true;
}

VECTOR NavMesh::CellToWorld(int x, int z) const
{
    // return center of cell
    float wx = origin_.x + (x + 0.5f) * cellSize_;
    float wy = origin_.y;
    float wz = origin_.z + (z + 0.5f) * cellSize_;
    return VGet(wx, wy, wz);
}

std::vector<VECTOR> NavMesh::FindPath(const VECTOR& start, const VECTOR& goal) const
{
    std::vector<VECTOR> out;
    int sx, sz, gx, gz;
    if (!WorldToCell(start, sx, sz)) return out;
    if (!WorldToCell(goal, gx, gz)) return out;
    if (!IsWalkable(sx, sz) || !IsWalkable(gx, gz)) return out;

    const int n = width_ * height_;
    const float INF = std::numeric_limits<float>::infinity();
    std::vector<float> g(n, INF);
    std::vector<int> parent(n, -1);
    std::vector<char> closed(n, 0);
    std::vector<char> open(n, 0);

    auto idx = [&](int x, int z){ return z * width_ + x; };
    int startIdx = idx(sx, sz);
    int goalIdx = idx(gx, gz);
    g[startIdx] = 0.0f;
    open[startIdx] = 1;
    std::vector<int> openList;
    openList.push_back(startIdx);

    auto heuristic = [&](int x, int z){ return (float)(abs(x - gx) + abs(z - gz)); };

    const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    while (!openList.empty()) {
        // find best f = g + h
        int bestIdx = -1;
        float bestF = INF;
        for (int i = 0; i < (int)openList.size(); ++i) {
            int node = openList[i];
            int nx = node % width_;
            int nz = node / width_;
            float f = g[node] + heuristic(nx,nz);
            if (f < bestF) { bestF = f; bestIdx = i; }
        }
        int u = openList[bestIdx];
        // remove from openList
        openList.erase(openList.begin() + bestIdx);
        open[u] = 0;
        if (u == goalIdx) break;
        closed[u] = 1;

        int ux = u % width_;
        int uz = u / width_;
        for (int di = 0; di < 4; ++di) {
            int vx = ux + dirs[di][0];
            int vz = uz + dirs[di][1];
            if (!InRange(vx, vz)) continue;
            int v = idx(vx, vz);
            if (closed[v]) continue;
            if (!cells_[v].walkable) continue;
            float tentative = g[u] + 1.0f;
            if (tentative < g[v]) {
                g[v] = tentative;
                parent[v] = u;
                if (!open[v]) {
                    open[v] = 1;
                    openList.push_back(v);
                }
            }
        }
    }

    if (parent[goalIdx] == -1 && startIdx != goalIdx) {
        // no path
        return out;
    }

    // reconstruct
    std::vector<int> rev;
    int cur = goalIdx;
    rev.push_back(cur);
    while (cur != startIdx) {
        cur = parent[cur];
        if (cur < 0) break;
        rev.push_back(cur);
    }
    // reverse to get start->goal
    for (int i = (int)rev.size() - 1; i >= 0; --i) {
        int ci = rev[i];
        int cx = ci % width_;
        int cz = ci / width_;
        out.push_back(CellToWorld(cx, cz));
    }
    return out;
}

void NavMesh::DebugDraw() const
{
    const float h = 0.05f;
    for (int z = 0; z < height_; ++z) {
        for (int x = 0; x < width_; ++x) {
            VECTOR c = CellToWorld(x,z);
            float half = cellSize_ * 0.5f;
            VECTOR a = VGet(c.x - half, c.y, c.z - half);
            VECTOR b = VGet(c.x + half, c.y + h, c.z + half);
            unsigned int col = cells_[Index(x,z)].walkable ? GetColor(0,160,0) : GetColor(160,0,0);
            // draw a thin wire cube (top face and vertical edges) since DrawBox3D may not be available
            VECTOR corners[8];
            // bottom face
            corners[0] = VGet(c.x - half, c.y, c.z - half);
            corners[1] = VGet(c.x + half, c.y, c.z - half);
            corners[2] = VGet(c.x + half, c.y, c.z + half);
            corners[3] = VGet(c.x - half, c.y, c.z + half);
            // top face
            corners[4] = VGet(c.x - half, c.y + h, c.z - half);
            corners[5] = VGet(c.x + half, c.y + h, c.z - half);
            corners[6] = VGet(c.x + half, c.y + h, c.z + half);
            corners[7] = VGet(c.x - half, c.y + h, c.z + half);
            // draw top rectangle
            DrawLine3D(corners[4], corners[5], col);
            DrawLine3D(corners[5], corners[6], col);
            DrawLine3D(corners[6], corners[7], col);
            DrawLine3D(corners[7], corners[4], col);
            // draw vertical edges
            DrawLine3D(corners[0], corners[4], col);
            DrawLine3D(corners[1], corners[5], col);
            DrawLine3D(corners[2], corners[6], col);
            DrawLine3D(corners[3], corners[7], col);
        }
    }
}
