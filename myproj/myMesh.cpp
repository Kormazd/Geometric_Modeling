#include "myMesh.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <GL/glew.h>
#include "myvector3d.h"

using namespace std;

myMesh::myMesh(void)
{
	/**** TODO ****/
}


myMesh::~myMesh(void)
{
	/**** TODO ****/
}

void myMesh::clear()
{
	for (unsigned int i = 0; i < vertices.size(); i++) if (vertices[i]) delete vertices[i];
	for (unsigned int i = 0; i < halfedges.size(); i++) if (halfedges[i]) delete halfedges[i];
	for (unsigned int i = 0; i < faces.size(); i++) if (faces[i]) delete faces[i];

	vector<myVertex *> empty_vertices;    vertices.swap(empty_vertices);
	vector<myHalfedge *> empty_halfedges; halfedges.swap(empty_halfedges);
	vector<myFace *> empty_faces;         faces.swap(empty_faces);
}

void myMesh::checkMesh()
{
	vector<myHalfedge *>::iterator it;
	for (it = halfedges.begin(); it != halfedges.end(); it++)
	{
		if ((*it)->twin == NULL)
			break;
	}
	if (it != halfedges.end())
		cout << "Error! Not all edges have their twins!\n";
	else cout << "Each edge has a twin!\n";
}


bool myMesh::readFile(std::string filename)
{
	string s, t, u;
	vector<int> faceids;
	myHalfedge **hedges;

	ifstream fin(filename);
	if (!fin.is_open()) {
		cout << "Unable to open file!\n";
		return false;
	}
	name = filename;

	map<pair<int, int>, myHalfedge *> twin_map;
	map<pair<int, int>, myHalfedge *>::iterator it;

	while (getline(fin, s))
	{
		stringstream myline(s);
		myline >> t;
		if (t == "g") {}
		else if (t == "v")
		{
			float x, y, z;
			myline >> x >> y >> z;

			myPoint3D* p = new myPoint3D(x, y, z);
			myVertex* v = new myVertex();
			v->point = p;
			v->index = vertices.size();
			vertices.push_back(v);
		}
		else if (t == "mtllib") {}
		else if (t == "usemtl") {}
		else if (t == "s") {}
		else if (t == "f")
		{
			faceids.clear();
			while (myline >> u) // read indices of vertices from a face into a container - it helps to access them later 
				faceids.push_back(atoi((u.substr(0, u.find("/"))).c_str()) - 1);
			if (faceids.size() < 3) // ignore degenerate faces
				continue;

			hedges = new myHalfedge * [faceids.size()]; // allocate the array for storing pointers to half-edges
			for (unsigned int i = 0; i < faceids.size(); i++)
				hedges[i] = new myHalfedge(); // pre-allocate new half-edges

			myFace* f = new myFace(); // allocate the new face
			f->adjacent_halfedge = hedges[0]; // connect the face with incident edge
			for (unsigned int i = 0; i < faceids.size(); i++)
			{
				int iplusone = (i + 1) % faceids.size();
				int iminusone = (i - 1 + faceids.size()) % faceids.size();

				// connect prevs, and next
				hedges[i]->next = hedges[iplusone];
				hedges[i]->prev = hedges[iminusone];

				// connect halfedge to face
				hedges[i]->adjacent_face = f;

				// set source (vertex origin) of halfedge
				hedges[i]->source = vertices[faceids[i]];

				// set originof for vertex if not already set
				if (vertices[faceids[i]]->originof == NULL)
					vertices[faceids[i]]->originof = hedges[i];

				// search for the twins using twin_map
				pair<int,int> rev_key = make_pair(faceids[iplusone], faceids[i]);
				it = twin_map.find(rev_key);
				if (it != twin_map.end())
				{
					hedges[i]->twin = it->second;
					it->second->twin = hedges[i];
				}
				else
				{
					pair<int,int> key = make_pair(faceids[i], faceids[iplusone]);
					twin_map[key] = hedges[i];
				}

				// push edges to halfedges in myMesh
				hedges[i]->index = halfedges.size();
				halfedges.push_back(hedges[i]);
			}
			delete[] hedges;
			// push faces to faces in myMesh
			f->index = faces.size();
			faces.push_back(f);
		}
	}

	checkMesh();
	normalize();

	return true;
}


void myMesh::computeNormals()
{
	for (unsigned int i = 0; i < faces.size(); i++)
		faces[i]->computeNormal();

	for (unsigned int i = 0; i < vertices.size(); i++)
		vertices[i]->computeNormal();
}

void myMesh::normalize()
{
	if (vertices.size() < 1) return;

	int tmpxmin = 0, tmpymin = 0, tmpzmin = 0, tmpxmax = 0, tmpymax = 0, tmpzmax = 0;

	for (unsigned int i = 0; i < vertices.size(); i++) {
		if (vertices[i]->point->X < vertices[tmpxmin]->point->X) tmpxmin = i;
		if (vertices[i]->point->X > vertices[tmpxmax]->point->X) tmpxmax = i;

		if (vertices[i]->point->Y < vertices[tmpymin]->point->Y) tmpymin = i;
		if (vertices[i]->point->Y > vertices[tmpymax]->point->Y) tmpymax = i;

		if (vertices[i]->point->Z < vertices[tmpzmin]->point->Z) tmpzmin = i;
		if (vertices[i]->point->Z > vertices[tmpzmax]->point->Z) tmpzmax = i;
	}

	double xmin = vertices[tmpxmin]->point->X, xmax = vertices[tmpxmax]->point->X,
		ymin = vertices[tmpymin]->point->Y, ymax = vertices[tmpymax]->point->Y,
		zmin = vertices[tmpzmin]->point->Z, zmax = vertices[tmpzmax]->point->Z;

	double scale = (xmax - xmin) > (ymax - ymin) ? (xmax - xmin) : (ymax - ymin);
	scale = scale > (zmax - zmin) ? scale : (zmax - zmin);

	for (unsigned int i = 0; i < vertices.size(); i++) {
		vertices[i]->point->X -= (xmax + xmin) / 2;
		vertices[i]->point->Y -= (ymax + ymin) / 2;
		vertices[i]->point->Z -= (zmax + zmin) / 2;

		vertices[i]->point->X /= scale;
		vertices[i]->point->Y /= scale;
		vertices[i]->point->Z /= scale;
	}
}


void myMesh::splitFaceTRIS(myFace *f, myPoint3D *p)
{
	/**** TODO ****/
}

void myMesh::splitEdge(myHalfedge *e1, myPoint3D *p)
{

	/**** TODO ****/
}

void myMesh::splitFaceQUADS(myFace *f, myPoint3D *p)
{
	/**** TODO ****/
}


void myMesh::subdivisionCatmullClark()
{
	/**** TODO ****/
}


void myMesh::triangulate()
{
	vector<myFace *> faces_copy = faces;
	for (unsigned int i = 0; i < faces_copy.size(); i++)
	{
		triangulate(faces_copy[i]);
	}
}

//return false if already triangle, true othewise.
bool myMesh::triangulate(myFace *f)
{
	myHalfedge *e = f->adjacent_halfedge;
	int n = 0;
	do {
		n++;
		e = e->next;
	} while (e != f->adjacent_halfedge);

	if (n == 3)
		return false;

	myPoint3D center(0, 0, 0);
	e = f->adjacent_halfedge;
	do {
		center.X += e->source->point->X;
		center.Y += e->source->point->Y;
		center.Z += e->source->point->Z;
		e = e->next;
	} while (e != f->adjacent_halfedge);

	center.X /= n;
	center.Y /= n;
	center.Z /= n;

	myVertex *center_vertex = new myVertex();
	center_vertex->point = new myPoint3D(center.X, center.Y, center.Z);
	vertices.push_back(center_vertex);

	e = f->adjacent_halfedge;
	for (int i = 0; i < n; i++)
	{
		myFace *new_face = new myFace();

		myHalfedge *he1 = new myHalfedge();
		myHalfedge *he2 = new myHalfedge();
		myHalfedge *he3 = new myHalfedge();

		he1->source = e->source;
		he2->source = e->next->source;
		he3->source = center_vertex;

		he1->adjacent_face = new_face;
		he2->adjacent_face = new_face;
		he3->adjacent_face = new_face;

		he1->next = he2;
		he2->next = he3;
		he3->next = he1;

		he1->prev = he3;
		he2->prev = he1;
		he3->prev = he2;

		halfedges.push_back(he1);
		halfedges.push_back(he2);
		halfedges.push_back(he3);

		new_face->adjacent_halfedge = he1;
		faces.push_back(new_face);

		e = e->next;
	}

	faces.erase(find(faces.begin(), faces.end(), f));
	delete f;

	return true;
}

