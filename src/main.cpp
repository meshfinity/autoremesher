#include <iostream>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
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
    // TODO
}

int main(int argc, char *argv[])
{
    /* if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " [input filename] [output filename]\n";
        return -1;
    }

    std::string inFilename(argv[1]);
    std::string outFilename(argv[2]); */

    std::string inFilename = "/Users/nunya/dev/autoremesher/test-in.obj";
    std::string outFilename = "/Users/nunya/dev/autoremesher/test-out.obj";

    std::vector<AutoRemesher::Vector3> inVertices;
    std::vector<std::vector<size_t>> inTriangles;
    if (!loadObj(inFilename, inVertices, inTriangles))
    {
        std::cerr << "Unable to load " << inFilename << "\n";
        return -1;
    }

    std::cout << "inVertices " << inVertices.size() << " inTriangles " << inTriangles.size() << "\n";

    AutoRemesher::AutoRemesher remesher(inVertices, inTriangles);
    // remesher.setScaling(2.0);
    // remesher.setTargetTriangleCount(8192);
    remesher.setModelType(AutoRemesher::ModelType::Organic);
    remesher.setProgressHandler(reportProgressHandler);
    if (!remesher.remesh())
    {
        std::cerr << "Remesher failed\n";
        return -1;
    }

    std::cout << "Remesh finished!\n";

    return 0;
}
