#ifndef _MESHFIX_H
#define _MESHFIX_H

#include <AutoRemesher/Vector3>

int runMeshfix(const char* infilename, std::vector<AutoRemesher::Vector3> &outVertices, std::vector<std::vector<size_t>> &outTriangles);

#endif
