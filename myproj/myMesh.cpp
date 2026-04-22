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
		if (!(myline >> t)) continue;
		if (t[0] == '#') continue;
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


void myMesh::generateSurfaceOfRevolution(vector<myPoint3D> &profile, int nSlices)
{
	clear();

	int nPts = profile.size();
	myVector3D axis(0, 1, 0);
	double angleStep = 2.0 * 3.14159265358979 / nSlices;

	for (int j = 0; j < nSlices; j++)
	{
		double angle = j * angleStep;
		for (int i = 0; i < nPts; i++)
		{
			myPoint3D p = profile[i];
			p.rotate(axis, angle);
			myVertex *v = new myVertex();
			v->point = new myPoint3D(p.X, p.Y, p.Z);
			v->index = vertices.size();
			vertices.push_back(v);
		}
	}

	map<pair<int, int>, myHalfedge *> twin_map;

	for (int j = 0; j < nSlices; j++)
	{
		int jnext = (j + 1) % nSlices;
		for (int i = 0; i < nPts - 1; i++)
		{
			int v0 = j * nPts + i;
			int v1 = j * nPts + i + 1;
			int v2 = jnext * nPts + i + 1;
			int v3 = jnext * nPts + i;

			int faceids[] = { v0, v1, v2, v3 };

			myHalfedge *he[4];
			for (int k = 0; k < 4; k++) he[k] = new myHalfedge();

			myFace *f = new myFace();
			f->adjacent_halfedge = he[0];

			for (int k = 0; k < 4; k++)
			{
				he[k]->next = he[(k + 1) % 4];
				he[k]->prev = he[(k + 3) % 4];
				he[k]->adjacent_face = f;
				he[k]->source = vertices[faceids[k]];

				if (vertices[faceids[k]]->originof == NULL)
					vertices[faceids[k]]->originof = he[k];

				int a = faceids[k], b = faceids[(k + 1) % 4];
				pair<int, int> rev = make_pair(b, a);
				map<pair<int, int>, myHalfedge *>::iterator it = twin_map.find(rev);
				if (it != twin_map.end())
				{
					he[k]->twin = it->second;
					it->second->twin = he[k];
				}
				else
					twin_map[make_pair(a, b)] = he[k];

				he[k]->index = halfedges.size();
				halfedges.push_back(he[k]);
			}

			f->index = faces.size();
			faces.push_back(f);
		}
	}

	myVertex *vTop = new myVertex();
	vTop->point = new myPoint3D(0, profile[nPts - 1].Y, 0);
	vTop->index = vertices.size();
	vertices.push_back(vTop);

	myVertex *vBot = new myVertex();
	vBot->point = new myPoint3D(0, profile[0].Y, 0);
	vBot->index = vertices.size();
	vertices.push_back(vBot);

	int topIdx = vTop->index;
	int botIdx = vBot->index;

	for (int j = 0; j < nSlices; j++)
	{
		int jnext = (j + 1) % nSlices;
		int a = j * nPts + (nPts - 1);
		int b = jnext * nPts + (nPts - 1);

		int fids[] = { a, b, topIdx };
		myHalfedge *he[3];
		for (int k = 0; k < 3; k++) he[k] = new myHalfedge();
		myFace *f = new myFace();
		f->adjacent_halfedge = he[0];
		for (int k = 0; k < 3; k++)
		{
			he[k]->next = he[(k + 1) % 3];
			he[k]->prev = he[(k + 2) % 3];
			he[k]->adjacent_face = f;
			he[k]->source = vertices[fids[k]];
			if (vertices[fids[k]]->originof == NULL)
				vertices[fids[k]]->originof = he[k];
			int aa = fids[k], bb = fids[(k + 1) % 3];
			pair<int, int> rev = make_pair(bb, aa);
			map<pair<int, int>, myHalfedge *>::iterator it = twin_map.find(rev);
			if (it != twin_map.end()) { he[k]->twin = it->second; it->second->twin = he[k]; }
			else twin_map[make_pair(aa, bb)] = he[k];
			he[k]->index = halfedges.size();
			halfedges.push_back(he[k]);
		}
		f->index = faces.size();
		faces.push_back(f);
	}

	for (int j = 0; j < nSlices; j++)
	{
		int jnext = (j + 1) % nSlices;
		int a = jnext * nPts;
		int b = j * nPts;

		int fids[] = { a, b, botIdx };
		myHalfedge *he[3];
		for (int k = 0; k < 3; k++) he[k] = new myHalfedge();
		myFace *f = new myFace();
		f->adjacent_halfedge = he[0];
		for (int k = 0; k < 3; k++)
		{
			he[k]->next = he[(k + 1) % 3];
			he[k]->prev = he[(k + 2) % 3];
			he[k]->adjacent_face = f;
			he[k]->source = vertices[fids[k]];
			if (vertices[fids[k]]->originof == NULL)
				vertices[fids[k]]->originof = he[k];
			int aa = fids[k], bb = fids[(k + 1) % 3];
			pair<int, int> rev = make_pair(bb, aa);
			map<pair<int, int>, myHalfedge *>::iterator it = twin_map.find(rev);
			if (it != twin_map.end()) { he[k]->twin = it->second; it->second->twin = he[k]; }
			else twin_map[make_pair(aa, bb)] = he[k];
			he[k]->index = halfedges.size();
			halfedges.push_back(he[k]);
		}
		f->index = faces.size();
		faces.push_back(f);
	}

	checkMesh();
	normalize();
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
	do { n++; e = e->next; } while (e != f->adjacent_halfedge);
	if (n == 3) return false;

	vector<myVertex *> verts(n);
	vector<myHalfedge *> hedges(n);
	e = f->adjacent_halfedge;
	for (int i = 0; i < n; i++) { verts[i] = e->source; hedges[i] = e; e = e->next; }

	myVector3D faceN(0, 0, 0);
	for (int i = 0; i < n; i++)
	{
		myPoint3D *a = verts[i]->point, *b = verts[(i + 1) % n]->point;
		faceN.dX += (a->Y - b->Y) * (a->Z + b->Z);
		faceN.dY += (a->Z - b->Z) * (a->X + b->X);
		faceN.dZ += (a->X - b->X) * (a->Y + b->Y);
	}

	vector<int> nxt(n), prv(n);
	for (int i = 0; i < n; i++) { nxt[i] = (i + 1) % n; prv[i] = (i - 1 + n) % n; }

	int remaining = n, cur = 0;
	while (remaining > 3)
	{
		bool found = false;
		int startIdx = cur;
		do {
			int p = prv[cur], nx = nxt[cur];
			myVector3D v1 = *verts[cur]->point - *verts[p]->point;
			myVector3D v2 = *verts[nx]->point - *verts[cur]->point;

			if (v1.crossproduct(v2) * faceN > 0)
			{
				bool isEar = true;
				int test = nxt[nx];
				while (test != p)
				{
					myVector3D c0 = (*verts[cur]->point - *verts[p]->point).crossproduct(*verts[test]->point - *verts[p]->point);
					myVector3D c1 = (*verts[nx]->point - *verts[cur]->point).crossproduct(*verts[test]->point - *verts[cur]->point);
					myVector3D c2 = (*verts[p]->point - *verts[nx]->point).crossproduct(*verts[test]->point - *verts[nx]->point);
					if (c0 * faceN > 0 && c1 * faceN > 0 && c2 * faceN > 0) { isEar = false; break; }
					test = nxt[test];
				}
				if (isEar)
				{
					myHalfedge *d_in = new myHalfedge(), *d_out = new myHalfedge();
					d_in->source = verts[nx]; d_out->source = verts[p];
					d_in->twin = d_out; d_out->twin = d_in;
					halfedges.push_back(d_in); halfedges.push_back(d_out);

					myFace *tri = new myFace();
					tri->adjacent_halfedge = hedges[p];
					faces.push_back(tri);

					hedges[p]->next = hedges[cur]; hedges[cur]->next = d_in; d_in->next = hedges[p];
					hedges[p]->prev = d_in; hedges[cur]->prev = hedges[p]; d_in->prev = hedges[cur];
					hedges[p]->adjacent_face = tri; hedges[cur]->adjacent_face = tri; d_in->adjacent_face = tri;

					hedges[p] = d_out;
					nxt[p] = nx; prv[nx] = p;
					remaining--; cur = nx;
					found = true; break;
				}
			}
			cur = nxt[cur];
		} while (cur != startIdx);
		if (!found) break;
	}

	int i0 = cur, i1 = nxt[i0], i2 = nxt[i1];
	f->adjacent_halfedge = hedges[i0];
	hedges[i0]->next = hedges[i1]; hedges[i1]->next = hedges[i2]; hedges[i2]->next = hedges[i0];
	hedges[i0]->prev = hedges[i2]; hedges[i1]->prev = hedges[i0]; hedges[i2]->prev = hedges[i1];
	hedges[i0]->adjacent_face = f; hedges[i1]->adjacent_face = f; hedges[i2]->adjacent_face = f;
	return true;
}

bool myMesh::testTriangulation()
{
	bool ok = true;
	cout << "=== Test Triangulation ===" << endl;

	for (unsigned int i = 0; i < faces.size(); i++)
	{
		myHalfedge *e = faces[i]->adjacent_halfedge;
		int count = 0;
		do { count++; e = e->next; } while (e != faces[i]->adjacent_halfedge && count <= 4);
		if (count != 3) { cout << "  FAIL: face " << i << " a " << count << " aretes" << endl; ok = false; }
	}

	int V = vertices.size(), E = halfedges.size() / 2, F = faces.size();
	cout << "  Euler: V=" << V << " E=" << E << " F=" << F << " => " << V - E + F << endl;

	cout << (ok ? "  PASSED" : "  FAILED") << endl;
	return ok;
}

bool myMesh::testNormals()
{
	bool ok = true;
	cout << "=== Test Normals ===" << endl;

	for (unsigned int i = 0; i < faces.size(); i++)
	{
		double len = faces[i]->normal->length();
		if (len < 0.9 || len > 1.1) { cout << "  FAIL: face " << i << " normal len=" << len << endl; ok = false; }
	}

	for (unsigned int i = 0; i < vertices.size(); i++)
	{
		if (vertices[i]->originof == NULL) continue;
		double len = vertices[i]->normal->length();
		if (len < 0.9 || len > 1.1) { cout << "  FAIL: vertex " << i << " normal len=" << len << endl; ok = false; }
	}

	cout << (ok ? "  PASSED" : "  FAILED") << endl;
	return ok;
}

bool myMesh::testHalfedgeConnectivity()
{
	bool ok = true;
	cout << "=== Test Halfedge Connectivity ===" << endl;

	for (unsigned int i = 0; i < halfedges.size(); i++)
	{
		myHalfedge *he = halfedges[i];
		if (he->next == NULL || he->prev == NULL) { cout << "  FAIL: he " << i << " NULL next/prev" << endl; ok = false; continue; }
		if (he->next->prev != he) { cout << "  FAIL: he " << i << " next->prev broken" << endl; ok = false; }
		if (he->prev->next != he) { cout << "  FAIL: he " << i << " prev->next broken" << endl; ok = false; }
		if (he->twin != NULL && he->twin->twin != he) { cout << "  FAIL: he " << i << " twin broken" << endl; ok = false; }
		if (he->source == NULL || he->adjacent_face == NULL) { cout << "  FAIL: he " << i << " NULL source/face" << endl; ok = false; }
	}

	cout << (ok ? "  PASSED" : "  FAILED") << endl;
	return ok;
}

