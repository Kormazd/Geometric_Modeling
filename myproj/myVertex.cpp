#include "myVertex.h"
#include "myvector3d.h"
#include "myHalfedge.h"
#include "myFace.h"

myVertex::myVertex(void)
{
	point = NULL;
	originof = NULL;
	normal = new myVector3D(1.0,1.0,1.0);
}

myVertex::~myVertex(void)
{
	if (normal) delete normal;
}

void myVertex::computeNormal()
{
	normal->dX = 0.0;
	normal->dY = 0.0;
	normal->dZ = 0.0;

	myHalfedge *e = originof;
	do {
		*normal = *normal + *(e->adjacent_face->normal);
		if (e->twin == NULL) break;
		e = e->twin->next;
	} while (e != originof);

	normal->normalize();
}
