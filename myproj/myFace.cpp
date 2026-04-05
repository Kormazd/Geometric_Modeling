#include "myFace.h"
#include "myvector3d.h"
#include "myHalfedge.h"
#include "myVertex.h"
#include <GL/glew.h>

myFace::myFace(void)
{
	adjacent_halfedge = NULL;
	normal = new myVector3D(1.0, 1.0, 1.0);
}

myFace::~myFace(void)
{
	if (normal) delete normal;
}

void myFace::computeNormal()
{
	myHalfedge *e1 = adjacent_halfedge;
	myHalfedge *e2 = e1->next;

	myVector3D v1 = *(e1->source->point) - *(e2->source->point);
	myVector3D v2 = *(e2->source->point) - *(e2->next->source->point);

	*normal = v1.crossproduct(v2);
	normal->normalize();
}
