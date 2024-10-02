#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <meshfix.h>
#include <xatlas.h>
#include <mapbox/earcut.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <geogram/basic/common.h>
#include <AutoRemesher/AutoRemesher>
#include <AutoRemesher/Vector3>

struct PolygonPoolItem
{
    std::size_t beginIdx = 0, count = 0;
    std::vector<std::uint32_t> original;
};

// Ripped from https://github.com/tinyobjloader/tinyobjloader/blob/release/tiny_obj_loader.h#L1482
std::vector<std::uint32_t> triangulate(std::vector<float> const &v, std::vector<std::uint32_t> const &polygonIndices)
{
    std::vector<std::uint32_t> result;

    using namespace tinyobj;

    face_t face;
    face.smoothing_group_id = 0;
    for (std::size_t idx : polygonIndices)
    {
        face.vertex_indices.emplace_back(vertex_index_t(idx));
    }
    size_t npolys = face.vertex_indices.size();

    if (npolys == 4)
    {
        vertex_index_t i0 = face.vertex_indices[0];
        vertex_index_t i1 = face.vertex_indices[1];
        vertex_index_t i2 = face.vertex_indices[2];
        vertex_index_t i3 = face.vertex_indices[3];

        size_t vi0 = size_t(i0.v_idx);
        size_t vi1 = size_t(i1.v_idx);
        size_t vi2 = size_t(i2.v_idx);
        size_t vi3 = size_t(i3.v_idx);

        if (((3 * vi0 + 2) >= v.size()) || ((3 * vi1 + 2) >= v.size()) ||
            ((3 * vi2 + 2) >= v.size()) || ((3 * vi3 + 2) >= v.size()))
        {
            // Invalid triangle.
            // FIXME(syoyo): Is it ok to simply skip this invalid triangle?
            return std::vector<std::uint32_t>();
        }

        real_t v0x = v[vi0 * 3 + 0];
        real_t v0y = v[vi0 * 3 + 1];
        real_t v0z = v[vi0 * 3 + 2];
        real_t v1x = v[vi1 * 3 + 0];
        real_t v1y = v[vi1 * 3 + 1];
        real_t v1z = v[vi1 * 3 + 2];
        real_t v2x = v[vi2 * 3 + 0];
        real_t v2y = v[vi2 * 3 + 1];
        real_t v2z = v[vi2 * 3 + 2];
        real_t v3x = v[vi3 * 3 + 0];
        real_t v3y = v[vi3 * 3 + 1];
        real_t v3z = v[vi3 * 3 + 2];

        // There are two candidates to split the quad into two triangles.
        //
        // Choose the shortest edge.
        // TODO: Is it better to determine the edge to split by calculating
        // the area of each triangle?
        //
        // +---+
        // |\  |
        // | \ |
        // |  \|
        // +---+
        //
        // +---+
        // |  /|
        // | / |
        // |/  |
        // +---+

        real_t e02x = v2x - v0x;
        real_t e02y = v2y - v0y;
        real_t e02z = v2z - v0z;
        real_t e13x = v3x - v1x;
        real_t e13y = v3y - v1y;
        real_t e13z = v3z - v1z;

        real_t sqr02 = e02x * e02x + e02y * e02y + e02z * e02z;
        real_t sqr13 = e13x * e13x + e13y * e13y + e13z * e13z;

        index_t idx0, idx1, idx2, idx3;

        idx0.vertex_index = i0.v_idx;
        idx0.normal_index = i0.vn_idx;
        idx0.texcoord_index = i0.vt_idx;
        idx1.vertex_index = i1.v_idx;
        idx1.normal_index = i1.vn_idx;
        idx1.texcoord_index = i1.vt_idx;
        idx2.vertex_index = i2.v_idx;
        idx2.normal_index = i2.vn_idx;
        idx2.texcoord_index = i2.vt_idx;
        idx3.vertex_index = i3.v_idx;
        idx3.normal_index = i3.vn_idx;
        idx3.texcoord_index = i3.vt_idx;

        if (sqr02 < sqr13)
        {
            // [0, 1, 2], [0, 2, 3]
            result.emplace_back(idx0.vertex_index);
            result.emplace_back(idx1.vertex_index);
            result.emplace_back(idx2.vertex_index);

            result.emplace_back(idx0.vertex_index);
            result.emplace_back(idx2.vertex_index);
            result.emplace_back(idx3.vertex_index);
        }
        else
        {
            // [0, 1, 3], [1, 2, 3]
            result.emplace_back(idx0.vertex_index);
            result.emplace_back(idx1.vertex_index);
            result.emplace_back(idx3.vertex_index);

            result.emplace_back(idx1.vertex_index);
            result.emplace_back(idx2.vertex_index);
            result.emplace_back(idx3.vertex_index);
        }
    }
    else
    {
        vertex_index_t i0 = face.vertex_indices[0];
        vertex_index_t i0_2 = i0;

        // TMW change: Find the normal axis of the polygon using Newell's
        // method
        TinyObjPoint n;
        for (size_t k = 0; k < npolys; ++k)
        {
            i0 = face.vertex_indices[k % npolys];
            size_t vi0 = size_t(i0.v_idx);

            size_t j = (k + 1) % npolys;
            i0_2 = face.vertex_indices[j];
            size_t vi0_2 = size_t(i0_2.v_idx);

            real_t v0x = v[vi0 * 3 + 0];
            real_t v0y = v[vi0 * 3 + 1];
            real_t v0z = v[vi0 * 3 + 2];

            real_t v0x_2 = v[vi0_2 * 3 + 0];
            real_t v0y_2 = v[vi0_2 * 3 + 1];
            real_t v0z_2 = v[vi0_2 * 3 + 2];

            const TinyObjPoint point1(v0x, v0y, v0z);
            const TinyObjPoint point2(v0x_2, v0y_2, v0z_2);

            TinyObjPoint a(point1.x - point2.x, point1.y - point2.y,
                           point1.z - point2.z);
            TinyObjPoint b(point1.x + point2.x, point1.y + point2.y,
                           point1.z + point2.z);

            n.x += (a.y * b.z);
            n.y += (a.z * b.x);
            n.z += (a.x * b.y);
        }
        real_t length_n = GetLength(n);
        // Check if zero length normal
        if (length_n <= 0)
        {
            // Failure - ignore face
            return std::vector<std::uint32_t>();
        }
        // Negative is to flip the normal to the correct direction
        real_t inv_length = -real_t(1.0) / length_n;
        n.x *= inv_length;
        n.y *= inv_length;
        n.z *= inv_length;

        TinyObjPoint axis_w, axis_v, axis_u;
        axis_w = n;
        TinyObjPoint a;
        if (std::fabs(axis_w.x) > real_t(0.9999999))
        {
            a = TinyObjPoint(0, 1, 0);
        }
        else
        {
            a = TinyObjPoint(1, 0, 0);
        }
        axis_v = Normalize(cross(axis_w, a));
        axis_u = cross(axis_w, axis_v);
        using Point = std::array<real_t, 2>;

        // first polyline define the main polygon.
        // following polylines define holes(not used in tinyobj).
        std::vector<std::vector<Point>> polygon;

        std::vector<Point> polyline;

        // TMW change: Find best normal and project v0x and v0y to those
        // coordinates, instead of picking a plane aligned with an axis (which
        // can flip polygons).

        // Fill polygon data(facevarying vertices).
        for (size_t k = 0; k < npolys; k++)
        {
            i0 = face.vertex_indices[k];
            size_t vi0 = size_t(i0.v_idx);

            assert(((3 * vi0 + 2) < v.size()));

            real_t v0x = v[vi0 * 3 + 0];
            real_t v0y = v[vi0 * 3 + 1];
            real_t v0z = v[vi0 * 3 + 2];

            TinyObjPoint polypoint(v0x, v0y, v0z);
            TinyObjPoint loc = WorldToLocal(polypoint, axis_u, axis_v, axis_w);

            polyline.push_back({loc.x, loc.y});
        }

        polygon.push_back(polyline);
        std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(polygon);
        // => result = 3 * faces, clockwise

        assert(indices.size() % 3 == 0);

        // Reconstruct vertex_index_t
        for (size_t k = 0; k < indices.size() / 3; k++)
        {
            {
                index_t idx0, idx1, idx2;
                idx0.vertex_index = face.vertex_indices[indices[3 * k + 0]].v_idx;
                idx1.vertex_index = face.vertex_indices[indices[3 * k + 1]].v_idx;
                idx2.vertex_index = face.vertex_indices[indices[3 * k + 2]].v_idx;

                result.emplace_back(idx0.vertex_index);
                result.emplace_back(idx1.vertex_index);
                result.emplace_back(idx2.vertex_index);
            }
        }
    }

    return result;
}

bool loadObj(std::string const &filename, std::vector<AutoRemesher::Vector3> &vertices, std::vector<std::vector<size_t>> &triangles)
{
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool loadSuccess = tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, filename.c_str());
    if (!warn.empty())
    {
        std::cout << "WARN:" << warn.c_str();
    }
    if (!err.empty())
    {
        std::cerr << err.c_str();
    }
    if (!loadSuccess)
    {
        return false;
    }

    vertices.clear();
    vertices.resize(attributes.vertices.size() / 3);
    for (size_t i = 0, j = 0; i < vertices.size(); ++i)
    {
        auto &dest = vertices[i];
        dest.setX(attributes.vertices[j++]);
        dest.setY(attributes.vertices[j++]);
        dest.setZ(attributes.vertices[j++]);
    }

    triangles.clear();
    for (const auto &shape : shapes)
    {
        for (size_t i = 0; i < shape.mesh.indices.size(); i += 3)
        {
            triangles.push_back(std::vector<size_t>{
                (size_t)shape.mesh.indices[i + 0].vertex_index,
                (size_t)shape.mesh.indices[i + 1].vertex_index,
                (size_t)shape.mesh.indices[i + 2].vertex_index});
        }
    }

    return true;
}

static void reportProgressHandler(void *tag, float progress)
{
    std::cout << "Progress: " << progress << "\n";
}

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        std::cout << "Usage: " << argv[0] << " [input filename] [output filename] [edge scaling (suggested = 4)] [target triangle count (suggested = 65536)]\n";
        return -1;
    }

    std::string inFilename(argv[1]);
    std::string outFilename(argv[2]);
    double edgeScaling = std::stof(argv[3]);
    int targetTriangleCount = std::stoi(argv[4]);

    std::cout << "MeshFix + AutoRemesher (Meshfinity CLI Edition)\n===\nInput filename: " << inFilename << "\nOutput filename: " << outFilename << "\nEdge scaling: " << edgeScaling << "\nTarget triangle count: " << targetTriangleCount << "\n===\n\n";

    std::cout << "Running MeshFix...\n";
    std::vector<AutoRemesher::Vector3> inVertices;
    std::vector<std::vector<size_t>> inTriangles;
    if (runMeshfix(inFilename.c_str(), inVertices, inTriangles) != 0)
    {
        std::cerr << "MeshFix failed\n";
        return -1;
    }
    std::cout << "MeshFix done! Now running AutoRemesher...\n";

    GEO::initialize();

    AutoRemesher::AutoRemesher remesher(inVertices, inTriangles);
    remesher.setScaling(edgeScaling);
    remesher.setTargetTriangleCount(targetTriangleCount);
    remesher.setModelType(AutoRemesher::ModelType::Organic);
    remesher.setProgressHandler(reportProgressHandler);
    if (!remesher.remesh())
    {
        std::cerr << "Remesh failed\n";
        return -1;
    }

    std::vector<AutoRemesher::Vector3> const &remeshedVertices = remesher.remeshedVertices();
    std::vector<std::vector<size_t>> const &remeshedQuads = remesher.remeshedQuads();

    // remeshedVertices contains lots of unreferenced vertices which don't belong to any face - filter those out here
    std::vector<float> outVertices;
    std::size_t outVerticesCount = 0;
    std::unordered_map<size_t, size_t> remeshed2OutIdx;
    for (std::vector<std::vector<size_t>>::const_iterator it = remeshedQuads.begin(); it != remeshedQuads.end(); ++it)
    {
        for (std::vector<size_t>::const_iterator subIt = (*it).begin(); subIt != (*it).end(); ++subIt)
        {
            if (remeshed2OutIdx.find(*subIt) == remeshed2OutIdx.end())
            {
                outVertices.emplace_back(remeshedVertices[*subIt].x());
                outVertices.emplace_back(remeshedVertices[*subIt].y());
                outVertices.emplace_back(remeshedVertices[*subIt].z());
                remeshed2OutIdx[*subIt] = outVerticesCount;
                ++outVerticesCount;
            }
        }
    }
    std::vector<PolygonPoolItem> polygonPool;
    std::vector<std::uint32_t> xatlasInputTris;
    std::vector<bool> xatlasInputTrisKeep;
    for (std::vector<std::vector<size_t>>::const_iterator it = remeshedQuads.begin(); it != remeshedQuads.end(); ++it)
    {
        if (it->size() == 3)
        {
            xatlasInputTris.emplace_back(remeshed2OutIdx[(*it)[0]]);
            xatlasInputTrisKeep.emplace_back(true);
            xatlasInputTris.emplace_back(remeshed2OutIdx[(*it)[1]]);
            xatlasInputTrisKeep.emplace_back(true);
            xatlasInputTris.emplace_back(remeshed2OutIdx[(*it)[2]]);
            xatlasInputTrisKeep.emplace_back(true);
        }
        else if (it->size() > 3)
        {
            std::vector<std::uint32_t> polygonIndices;
            for (std::vector<size_t>::const_iterator subIt = it->begin(); subIt != it->end(); ++subIt)
            {
                polygonIndices.emplace_back(remeshed2OutIdx[*subIt]);
            }

            std::vector<std::uint32_t> triangulatedIndices = triangulate(outVertices, polygonIndices);
            xatlasInputTris.insert(xatlasInputTris.end(), triangulatedIndices.begin(), triangulatedIndices.end());
            for (std::size_t i = 0; i < triangulatedIndices.size(); ++i)
            {
                xatlasInputTrisKeep.emplace_back(true);
            }

            // Only support quads in polygon pool for now
            if (it->size() == 4)
            {
                PolygonPoolItem item;
                item.beginIdx = xatlasInputTris.size() - triangulatedIndices.size();
                item.count = triangulatedIndices.size();
                item.original = polygonIndices;
                polygonPool.emplace_back(item);
            }
        }
        else
        {
            // Ignore degenerate faces
        }
    }

    xatlas::Atlas *atlas = xatlas::Create();
    xatlas::MeshDecl meshDecl;
    meshDecl.vertexCount = outVerticesCount;
    meshDecl.vertexPositionData = outVertices.data();
    meshDecl.vertexPositionStride = sizeof(float) * 3;
    meshDecl.indexCount = xatlasInputTris.size();
    meshDecl.indexData = xatlasInputTris.data();
    meshDecl.indexFormat = xatlas::IndexFormat::UInt32;
    xatlas::AddMeshError error = xatlas::AddMesh(atlas, meshDecl, 1);
    if (error != xatlas::AddMeshError::Success)
    {
        std::cerr << "xatlas failed\n";
        return -1;
    }
    xatlas::Generate(atlas);
    for (std::size_t i = 0; i < atlas->atlasCount; ++i)
    {
        std::cerr << "Atlas " << i << ": " << std::roundf(atlas->utilization[i] * 100.0f) << "\% utilization\n";
    }
    std::cout << "Atlas resolution: " << atlas->width << "x" << atlas->height << "\n";
    std::cout << "Total vertices after atlasing: " << atlas->meshes[0].vertexCount << "\n";

    // OBJ Output: Vertices
    std::ofstream outFile(outFilename, std::ios::out);
    for (std::size_t i = 0; i < outVerticesCount; ++i)
    {
        outFile << "v " << outVertices[i * 3 + 0] << " " << outVertices[i * 3 + 1] << " " << outVertices[i * 3 + 2] << "\n";
    }
    for (std::size_t i = 0; i < atlas->meshes[0].vertexCount; ++i)
    {
        const xatlas::Vertex &vertex = atlas->meshes[0].vertexArray[i];
        outFile << "vt " << vertex.uv[0] / static_cast<float>(atlas->width) << " " << vertex.uv[1] / static_cast<float>(atlas->height) << "\n";
    }

    // OBJ Output: Polygons
    for (PolygonPoolItem const &item : polygonPool)
    {
        bool isValid = true;

        std::unordered_map<std::uint32_t, std::uint32_t> old2New;
        for (std::size_t i = item.beginIdx; i < item.beginIdx + item.count; ++i)
        {
            const std::size_t newIdx = atlas->meshes[0].indexArray[i];
            const xatlas::Vertex &xatlasVertex = atlas->meshes[0].vertexArray[newIdx];
            std::size_t oldIdx = xatlasVertex.xref;
            if (old2New.find(oldIdx) == old2New.end())
            {
                old2New[oldIdx] = newIdx;
            }
            else if (old2New[oldIdx] != newIdx)
            {
                isValid = false;
                break;
            }
        }

        if (isValid)
        {
            outFile << "f";
            for (std::size_t i = 0; i < item.original.size(); ++i)
            {
                std::size_t oldIdx = item.original[i];
                const std::size_t newIdx = old2New[item.original[i]];
                outFile << " " << (oldIdx + 1) << "/" << (newIdx + 1);
            }
            outFile << "\n";

            for (std::size_t i = item.beginIdx; i < item.beginIdx + item.count; ++i)
            {
                xatlasInputTrisKeep[i] = false;
            }
        }
    }

    // OBJ Output: Triangles
    for (std::size_t i = 0; i < xatlasInputTris.size(); i += 3)
    {
        bool wrote = false;
        for (std::size_t j = 0; j < 3; ++j)
        {
            if (xatlasInputTrisKeep[i + j])
            {
                if (!wrote)
                {
                    outFile << "f";
                    wrote = true;
                }

                const std::size_t texIdx = atlas->meshes[0].indexArray[i + j];
                const xatlas::Vertex &xatlasVertex = atlas->meshes[0].vertexArray[texIdx];
                std::size_t posIdx = xatlasVertex.xref;
                outFile << " " << (posIdx + 1) << "/" << (texIdx + 1);
            }
        }
        if (wrote)
        {
            outFile << "\n";
        }
    }

    std::cout << "AutoRemesher done!\n";

    return 0;
}
