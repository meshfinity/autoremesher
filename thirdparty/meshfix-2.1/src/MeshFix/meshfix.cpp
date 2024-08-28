#include "tmesh.h"
#include "meshfix.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

using namespace T_MESH;

double closestPair(List *bl1, List *bl2, Vertex **closest_on_bl1, Vertex **closest_on_bl2)
{
	Node *n, *m;
	Vertex *v, *w;
	double adist, mindist = DBL_MAX;

	FOREACHVVVERTEX(bl1, v, n)
		FOREACHVVVERTEX(bl2, w, m)
	if ((adist = w->squaredDistance(v))<mindist)
	{
		mindist = adist;
		*closest_on_bl1 = v;
		*closest_on_bl2 = w;
	}

	return mindist;
}

bool joinClosestComponents(Basic_TMesh *tin)
{
	Vertex *v, *w, *gv, *gw;
	Triangle *t, *s;
	Node *n;
	List triList, boundary_loops, *one_loop;
	List **bloops_array;
	int i, j, numloops;

	// Mark triangles with connected component's unique ID
	i = 0;
	FOREACHVTTRIANGLE((&(tin->T)), t, n) t->info = NULL;
	FOREACHVTTRIANGLE((&(tin->T)), t, n) if (t->info == NULL)
	{
		i++;
		triList.appendHead(t);
		t->info = (void *)(intptr_t)i;

		while (triList.numels())
		{
			t = (Triangle *)triList.popHead();
			if ((s = t->t1()) != NULL && s->info == NULL) { triList.appendHead(s); s->info = (void *)(intptr_t)i; }
			if ((s = t->t2()) != NULL && s->info == NULL) { triList.appendHead(s); s->info = (void *)(intptr_t)i; }
			if ((s = t->t3()) != NULL && s->info == NULL) { triList.appendHead(s); s->info = (void *)(intptr_t)i; }
		}
	}

	if (i<2)
	{
		FOREACHVTTRIANGLE((&(tin->T)), t, n) t->info = NULL;
		//   JMesh::info("Mesh is a single component. Nothing done.");
		return false;
	}

	FOREACHVTTRIANGLE((&(tin->T)), t, n)
	{
		t->v1()->info = t->v2()->info = t->v3()->info = t->info;
	}

	FOREACHVVVERTEX((&(tin->V)), v, n) if (!IS_VISITED2(v) && v->isOnBoundary())
	{
		w = v;
		one_loop = new List;
		do
		{
			one_loop->appendHead(w); MARK_VISIT2(w);
			w = w->nextOnBoundary();
		} while (w != v);
		boundary_loops.appendHead(one_loop);
	}
	FOREACHVVVERTEX((&(tin->V)), v, n) UNMARK_VISIT2(v);

	bloops_array = (List **)boundary_loops.toArray();
	numloops = boundary_loops.numels();

	int numtris = tin->T.numels();
	double adist, mindist = DBL_MAX;

	gv = NULL;
	for (i = 0; i<numloops; i++)
	for (j = 0; j<numloops; j++)
	if (((Vertex *)bloops_array[i]->head()->data)->info != ((Vertex *)bloops_array[j]->head()->data)->info)
	{
		adist = closestPair(bloops_array[i], bloops_array[j], &v, &w);
		if (adist<mindist) { mindist = adist; gv = v; gw = w; }
	}

	if (gv != NULL) tin->joinBoundaryLoops(gv, gw, 1, 0);

	FOREACHVTTRIANGLE((&(tin->T)), t, n) t->info = NULL;
	FOREACHVVVERTEX((&(tin->V)), v, n) v->info = NULL;

	free(bloops_array);
	while ((one_loop = (List *)boundary_loops.popHead()) != NULL) delete one_loop;

	return (gv != NULL);
}

int runMeshfix(const char* infilename, std::vector<AutoRemesher::Vector3> &outVertices, std::vector<std::vector<size_t>> &outTriangles)
{
 TMesh::init(); // This is mandatory
 TMesh::app_name = "MeshFix";
 TMesh::app_version = "2.0";
 TMesh::app_year = "2016";
 TMesh::app_authors = "Marco Attene";
 TMesh::app_maillist = "attene@ge.imati.cnr.it";
 
 clock_t beginning = clock();

 Basic_TMesh tin;
 bool skip_if_fixed = false;
 bool join_multiple_components = false;

 // The loader automatically reconstructs a manifold triangle connectivity
 if (tin.load(infilename) != 0) TMesh::error("Can't open file.\n");

 if (join_multiple_components)
 {
	 TMesh::info("\nJoining input components ...\n");
	 TMesh::begin_progress();
	 while (joinClosestComponents(&tin)) TMesh::report_progress("Num. components: %d       ", tin.shells());
	 TMesh::end_progress();
	 tin.deselectTriangles();
 }

	   // Keep only the largest component (i.e. with most triangles)
	   int sc = tin.removeSmallestComponents();
	   if (sc) TMesh::warning("Removed %d small components\n",sc);

	   // Fill holes
	   if (tin.boundaries())
	   {
		TMesh::warning("Patching holes\n");
		tin.fillSmallBoundaries(0, true);
	   }

	   // Run geometry correction
	   if (!tin.boundaries()) TMesh::warning("Fixing degeneracies and intersections...\n");
	   if (tin.boundaries() || !tin.meshclean()) TMesh::warning("MeshFix could not fix everything.\n", sc);


 TMesh::info("Copying output mesh ...\n");
 tin.save(outVertices, outTriangles);

 // Normally, CLOCKS_PER_SEC >= 1000, but this is not technically guaranteed,
 // and if it were not true, dividing by zero would invoke undefined behavior.
 if (CLOCKS_PER_SEC >= 1000)
	 printf("MeshFix elapsed time: %d ms\n", (clock() - beginning) / (CLOCKS_PER_SEC / 1000));

 return 0;
}
