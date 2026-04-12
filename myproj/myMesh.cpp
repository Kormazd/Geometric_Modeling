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
			while (myline >> u)
				faceids.push_back(atoi((u.substr(0, u.find("/"))).c_str()) - 1);
			if (faceids.size() < 3)
				continue;

			vector<myHalfedge*> hedges_vec;
			for (unsigned int i = 0; i < faceids.size(); i++)
				hedges_vec.push_back(new myHalfedge());

			myFace *f = new myFace();
			f->adjacent_halfedge = hedges_vec[0];

			for (unsigned int i = 0; i < faceids.size(); i++)
			{
				int iplusone = (i + 1) % faceids.size();
				int iminusone = (i - 1 + faceids.size()) % faceids.size();

				hedges_vec[i]->next = hedges_vec[iplusone];
				hedges_vec[i]->prev = hedges_vec[iminusone];
				hedges_vec[i]->adjacent_face = f;
				hedges_vec[i]->source = vertices[faceids[i]];

				if (vertices[faceids[i]]->originof == NULL)
					vertices[faceids[i]]->originof = hedges_vec[i];

				pair<int,int> rev_key = make_pair(faceids[iplusone], faceids[i]);
				it = twin_map.find(rev_key);
				if (it != twin_map.end())
				{
					hedges_vec[i]->twin = it->second;
					it->second->twin = hedges_vec[i];
				}
				else
				{
					pair<int,int> key = make_pair(faceids[i], faceids[iplusone]);
					twin_map[key] = hedges_vec[i];
				}

				hedges_vec[i]->index = halfedges.size();
				halfedges.push_back(hedges_vec[i]);
			}

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
		triangulate(faces_copy[i]);
}

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

	myHalfedge *v0 = f->adjacent_halfedge;
	myHalfedge *v1 = v0->next;

	for (int i = 1; i < n - 1; i++)
	{
		myFace *tri = new myFace();

		myHalfedge *he1 = new myHalfedge();
		myHalfedge *he2 = new myHalfedge();
		myHalfedge *he3 = new myHalfedge();

		he1->source = v0->source;
		he2->source = v1->source;
		he3->source = v1->next->source;

		he1->adjacent_face = tri;
		he2->adjacent_face = tri;
		he3->adjacent_face = tri;

		he1->next = he2;
		he2->next = he3;
		he3->next = he1;

		he1->prev = he3;
		he2->prev = he1;
		he3->prev = he2;

		he1->index = halfedges.size();
		halfedges.push_back(he1);
		he2->index = halfedges.size();
		halfedges.push_back(he2);
		he3->index = halfedges.size();
		halfedges.push_back(he3);

		tri->adjacent_halfedge = he1;
		tri->index = faces.size();
		faces.push_back(tri);

		v1 = v1->next;
	}

	faces.erase(find(faces.begin(), faces.end(), f));
	delete f;

	return true;
}

