#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <meshfix.h>
#include <xatlas.h>
#include "tiny_obj_loader.h"
#include <geogram/basic/common.h>
#include <AutoRemesher/AutoRemesher>
#include <AutoRemesher/Vector3>

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
    std::vector<std::uint32_t> outIndices;
    std::vector<std::uint8_t> outFacesNumVertices;
    for (std::vector<std::vector<size_t>>::const_iterator it = remeshedQuads.begin(); it != remeshedQuads.end(); ++it)
    {
        std::uint8_t faceNumVertices = 0;
        for (std::vector<size_t>::const_iterator subIt = (*it).begin(); subIt != (*it).end(); ++subIt)
        {
            outIndices.emplace_back(remeshed2OutIdx[*subIt]);
            ++faceNumVertices;
        }
        outFacesNumVertices.emplace_back(faceNumVertices);
    }

    // xatlas
    xatlas::Atlas *atlas = xatlas::Create();
    xatlas::MeshDecl meshDecl;
    meshDecl.vertexCount = outVerticesCount;
    meshDecl.vertexPositionData = outVertices.data();
    meshDecl.vertexPositionStride = sizeof(float) * 3;
    meshDecl.indexCount = outIndices.size();
    meshDecl.indexData = outIndices.data();
    meshDecl.indexFormat = xatlas::IndexFormat::UInt32;
    meshDecl.faceVertexCount = outFacesNumVertices.data();
    meshDecl.faceCount = outFacesNumVertices.size();
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

    std::ofstream outFile(outFilename, std::ios::out);
    for (std::size_t i = 0; i < atlas->meshes[0].vertexCount; ++i)
    {
        const xatlas::Vertex &vertex = atlas->meshes[0].vertexArray[i];
        const float *pos = &outVertices[vertex.xref * 3];
        outFile << "v " << pos[0] << " " << pos[1] << " " << pos[2] << "\n";
        outFile << "vt " << vertex.uv[0] / static_cast<float>(atlas->width) << " " << vertex.uv[1] / static_cast<float>(atlas->height) << "\n";
    }
    std::size_t currentFaceIndex = 0;
    for (std::size_t i = 0; i < outFacesNumVertices.size(); ++i)
    {
        outFile << "f";
        const std::size_t index = atlas->meshes[0].indexArray[currentFaceIndex];
        for (std::size_t j = 0; j < outFacesNumVertices[i]; ++j)
        {
            outFile << " " << index << "/" << index;
        }
        outFile << "\n";
        ++currentFaceIndex;
    }

    std::cout << "AutoRemesher done!\n";

    return 0;
}
