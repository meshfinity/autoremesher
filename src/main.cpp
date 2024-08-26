#include <iostream>
#include <fstream>
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

    std::cout << "AutoRemesher\n===\nInput filename: " << inFilename << "\nOutput filename: " << outFilename << "\nEdge scaling: " << edgeScaling << "\nTarget triangle count: " << targetTriangleCount << "\n===\n\n";

    GEO::initialize();

    std::vector<AutoRemesher::Vector3> inVertices;
    std::vector<std::vector<size_t>> inTriangles;
    if (!loadObj(inFilename, inVertices, inTriangles))
    {
        std::cerr << "Unable to load " << inFilename << "\n";
        return -1;
    }

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

    std::ofstream outFile(outFilename, std::ios::out);
    for (std::vector<AutoRemesher::Vector3>::const_iterator it = remeshedVertices.begin(); it != remeshedVertices.end(); ++it)
    {
        outFile << "v " << (*it).x() << " " << (*it).y() << " " << (*it).z() << "\n";
    }
    for (std::vector<std::vector<size_t>>::const_iterator it = remeshedQuads.begin(); it != remeshedQuads.end(); ++it)
    {
        outFile << "f";
        for (std::vector<size_t>::const_iterator subIt = (*it).begin(); subIt != (*it).end(); ++subIt)
        {
            outFile << " " << (1 + *subIt);
        }
        outFile << "\n";
    }
    outFile.close();

    return 0;
}
