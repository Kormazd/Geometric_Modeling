#include "myMesh.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <utility>
#include <limits>
#include <algorithm>
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
	if (profile.size() < 2 || nSlices < 3) return;

	int nPts = (int)profile.size();
	myVector3D axis(0, 1, 0);
	double step = 2.0 * 3.14159265358979 / nSlices;

	for (int j = 0; j < nSlices; j++)
	{
		double a = j * step;
		for (int i = 0; i < nPts; i++)
		{
			myPoint3D p = profile[i];
			p.rotate(axis, a);
			myVertex *v = new myVertex();
			v->point = new myPoint3D(p.X, p.Y, p.Z);
			v->index = vertices.size();
			vertices.push_back(v);
		}
	}

	map<pair<int, int>, myHalfedge *> twin_map;

	for (int j = 0; j < nSlices; j++)
	{
		int j2 = (j + 1) % nSlices;
		for (int i = 0; i < nPts - 1; i++)
		{
			int ids[4] = { j * nPts + i, j * nPts + i + 1, j2 * nPts + i + 1, j2 * nPts + i };
			myHalfedge *h[4];
			for (int k = 0; k < 4; k++) h[k] = new myHalfedge();
			myFace *f = new myFace();
			f->adjacent_halfedge = h[0];

			for (int k = 0; k < 4; k++)
			{
				int kn = (k + 1) % 4, kp = (k + 3) % 4;
				h[k]->next = h[kn];
				h[k]->prev = h[kp];
				h[k]->adjacent_face = f;
				h[k]->source = vertices[ids[k]];
				if (vertices[ids[k]]->originof == NULL) vertices[ids[k]]->originof = h[k];

				pair<int, int> rev = make_pair(ids[kn], ids[k]);
				map<pair<int, int>, myHalfedge *>::iterator it = twin_map.find(rev);
				if (it != twin_map.end()) { h[k]->twin = it->second; it->second->twin = h[k]; }
				else twin_map[make_pair(ids[k], ids[kn])] = h[k];

				h[k]->index = halfedges.size();
				halfedges.push_back(h[k]);
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

	int topIdx = vTop->index, botIdx = vBot->index;
	for (int j = 0; j < nSlices; j++)
	{
		int j2 = (j + 1) % nSlices;
		int ids[3] = { j * nPts + (nPts - 1), j2 * nPts + (nPts - 1), topIdx };
		myHalfedge *h[3];
		for (int k = 0; k < 3; k++) h[k] = new myHalfedge();
		myFace *f = new myFace();
		f->adjacent_halfedge = h[0];
		for (int k = 0; k < 3; k++)
		{
			int kn = (k + 1) % 3, kp = (k + 2) % 3;
			h[k]->next = h[kn];
			h[k]->prev = h[kp];
			h[k]->adjacent_face = f;
			h[k]->source = vertices[ids[k]];
			if (vertices[ids[k]]->originof == NULL) vertices[ids[k]]->originof = h[k];
			pair<int, int> rev = make_pair(ids[kn], ids[k]);
			map<pair<int, int>, myHalfedge *>::iterator it = twin_map.find(rev);
			if (it != twin_map.end()) { h[k]->twin = it->second; it->second->twin = h[k]; }
			else twin_map[make_pair(ids[k], ids[kn])] = h[k];
			h[k]->index = halfedges.size();
			halfedges.push_back(h[k]);
		}
		f->index = faces.size();
		faces.push_back(f);
	}

	for (int j = 0; j < nSlices; j++)
	{
		int j2 = (j + 1) % nSlices;
		int ids[3] = { j2 * nPts, j * nPts, botIdx };
		myHalfedge *h[3];
		for (int k = 0; k < 3; k++) h[k] = new myHalfedge();
		myFace *f = new myFace();
		f->adjacent_halfedge = h[0];
		for (int k = 0; k < 3; k++)
		{
			int kn = (k + 1) % 3, kp = (k + 2) % 3;
			h[k]->next = h[kn];
			h[k]->prev = h[kp];
			h[k]->adjacent_face = f;
			h[k]->source = vertices[ids[k]];
			if (vertices[ids[k]]->originof == NULL) vertices[ids[k]]->originof = h[k];
			pair<int, int> rev = make_pair(ids[kn], ids[k]);
			map<pair<int, int>, myHalfedge *>::iterator it = twin_map.find(rev);
			if (it != twin_map.end()) { h[k]->twin = it->second; it->second->twin = h[k]; }
			else twin_map[make_pair(ids[k], ids[kn])] = h[k];
			h[k]->index = halfedges.size();
			halfedges.push_back(h[k]);
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
	// =====================================================================
	// APPROCHE : on crée un nouveau maillage vide, on le remplit étape par
	// étape, puis on remplace l'ancien maillage par le nouveau.
	// On utilise des std::map pour retrouver rapidement quel nouveau sommet
	// correspond à quelle ancienne face / ancienne arête / ancien sommet.
	// =====================================================================

	// Les nouveaux sommets seront stockés dans ces vecteurs temporaires.
	// On les indexe à partir de 0 dans le nouveau maillage.
	vector<myPoint3D> newPoints;   // toutes les positions des nouveaux sommets
	vector<vector<int>> newFaces;  // chaque face = liste d'indices dans newPoints

	// -----------------------------------------------------------------------
	// ETAPE 1 : FACE POINTS
	// Pour chaque ancienne face, on calcule la moyenne de ses sommets.
	// Ce point représente le "centre" de la face dans le nouveau maillage.
	// -----------------------------------------------------------------------
	// On stocke l'indice du face point de chaque face dans cette map.
	map<myFace*, int> facePointIndex;

	for (unsigned int i = 0; i < faces.size(); i++)
	{
		myFace *f = faces[i];
		myHalfedge *he = f->adjacent_halfedge;

		// Parcourir tous les sommets de la face et sommer leurs positions
		myPoint3D sum(0.0, 0.0, 0.0);
		int count = 0;
		do {
			sum.X += he->source->point->X;
			sum.Y += he->source->point->Y;
			sum.Z += he->source->point->Z;
			count++;
			he = he->next;
		} while (he != f->adjacent_halfedge);

		// Le face point = moyenne des sommets de la face
		myPoint3D fp(sum.X / count, sum.Y / count, sum.Z / count);

		// On enregistre l'indice de ce nouveau sommet
		facePointIndex[f] = (int)newPoints.size();
		newPoints.push_back(fp);
	}

	// -----------------------------------------------------------------------
	// ETAPE 2 : EDGE POINTS
	// Pour chaque arête physique (paire de half-edges jumeaux), on calcule :
	//   e = (v1 + v2 + f1 + f2) / 4
	// où v1, v2 sont les deux extrémités et f1, f2 sont les face points des
	// deux faces adjacentes à cette arête.
	// -----------------------------------------------------------------------
	// On stocke l'indice du edge point pour chaque half-edge (et son twin).
	// Pour éviter de traiter chaque arête deux fois, on utilise un set.
	map<myHalfedge*, int> edgePointIndex;
	set<myHalfedge*> visitedEdges;

	for (unsigned int i = 0; i < halfedges.size(); i++)
	{
		myHalfedge *he = halfedges[i];

		// Si on a déjà traité cette arête (via son twin), on passe
		if (visitedEdges.count(he)) continue;
		if (he->twin == NULL) continue; // arête de bord : on ignore pour l'instant

		// Marquer les deux demi-arêtes comme traitées
		visitedEdges.insert(he);
		visitedEdges.insert(he->twin);

		// Récupérer les deux extrémités de l'arête
		myPoint3D *v1 = he->source->point;
		myPoint3D *v2 = he->twin->source->point;

		// Récupérer les face points des deux faces adjacentes (étape 1)
		myPoint3D f1 = newPoints[facePointIndex[he->adjacent_face]];
		myPoint3D f2 = newPoints[facePointIndex[he->twin->adjacent_face]];

		// Formule du cours : e = (v1 + v2 + f1 + f2) / 4
		myPoint3D ep(
			(v1->X + v2->X + f1.X + f2.X) / 4.0,
			(v1->Y + v2->Y + f1.Y + f2.Y) / 4.0,
			(v1->Z + v2->Z + f1.Z + f2.Z) / 4.0
		);

		// Les deux demi-arêtes partagent le même edge point
		int idx = (int)newPoints.size();
		edgePointIndex[he]       = idx;
		edgePointIndex[he->twin] = idx;
		newPoints.push_back(ep);
	}

	// -----------------------------------------------------------------------
	// ETAPE 3 : VERTEX POINTS
	// Pour chaque ancien sommet p de valence n, on calcule sa nouvelle position
	// lissée avec la formule du cours :
	//   v = (F_avg + 2 * M_avg + (n-3) * p) / n
	// où :
	//   F_avg = moyenne des face points des faces adjacentes au sommet
	//   M_avg = moyenne des milieux des anciennes arêtes issues du sommet
	//   n     = valence (nombre d'arêtes issues du sommet)
	// -----------------------------------------------------------------------
	map<myVertex*, int> vertexPointIndex;

	for (unsigned int i = 0; i < vertices.size(); i++)
	{
		myVertex *v = vertices[i];
		if (v->originof == NULL) continue; // sommet isolé : on ignore

		// Parcourir le 1-ring du sommet via les half-edges sortants
		// (on tourne autour du sommet en suivant : he -> he->twin->next)
		myPoint3D F_avg(0.0, 0.0, 0.0); // somme des face points adjacents
		myPoint3D M_avg(0.0, 0.0, 0.0); // somme des milieux des arêtes adjacentes
		int n = 0;                        // valence

		myHalfedge *start = v->originof;
		myHalfedge *he    = start;
		do {
			// Face point de la face adjacente à ce half-edge
			myPoint3D fp = newPoints[facePointIndex[he->adjacent_face]];
			F_avg.X += fp.X;
			F_avg.Y += fp.Y;
			F_avg.Z += fp.Z;

			// Milieu de l'arête (v + voisin) / 2
			myPoint3D *neighbor = he->next->source->point;
			M_avg.X += (v->point->X + neighbor->X) / 2.0;
			M_avg.Y += (v->point->Y + neighbor->Y) / 2.0;
			M_avg.Z += (v->point->Z + neighbor->Z) / 2.0;

			n++;
			// Passer à l'arête suivante autour du sommet
			if (he->twin == NULL) break; // bord : on arrête
			he = he->twin->next;
		} while (he != start);

		if (n == 0) continue;

		// Terminer le calcul des moyennes
		F_avg.X /= n; F_avg.Y /= n; F_avg.Z /= n;
		M_avg.X /= n; M_avg.Y /= n; M_avg.Z /= n;

		// Formule du cours : v = (F_avg + 2*M_avg + (n-3)*p) / n
		myPoint3D *p = v->point;
		myPoint3D vp(
			(F_avg.X + 2.0 * M_avg.X + (n - 3) * p->X) / n,
			(F_avg.Y + 2.0 * M_avg.Y + (n - 3) * p->Y) / n,
			(F_avg.Z + 2.0 * M_avg.Z + (n - 3) * p->Z) / n
		);

		vertexPointIndex[v] = (int)newPoints.size();
		newPoints.push_back(vp);
	}

	// -----------------------------------------------------------------------
	// ETAPE 4 : CREATION DES QUADRILATERES
	// Pour chaque ancienne face, et pour chaque coin (sommet) de cette face,
	// on crée un quad qui relie dans l'ordre :
	//   [vertex point] -> [edge point de l'arête courante]
	//   -> [face point] -> [edge point de l'arête précédente]
	//
	//       v_point --- e_point_prev
	//          |              |
	//       e_point  --- f_point
	// -----------------------------------------------------------------------
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		myFace *f = faces[i];
		int fp_idx = facePointIndex[f]; // face point de cette face

		// Collecter les half-edges de la face dans l'ordre
		vector<myHalfedge*> faceHE;
		myHalfedge *he = f->adjacent_halfedge;
		do {
			faceHE.push_back(he);
			he = he->next;
		} while (he != f->adjacent_halfedge);

		int n = (int)faceHE.size();
		for (int k = 0; k < n; k++)
		{
			myHalfedge *heCur  = faceHE[k];
			myHalfedge *hePrev = faceHE[(k + n - 1) % n];

			int vp_idx   = vertexPointIndex[heCur->source]; // vertex point du coin
			int ep_cur   = edgePointIndex[heCur];           // edge point arête courante
			int ep_prev  = edgePointIndex[hePrev];          // edge point arête précédente

			// Le quad dans le sens trigonométrique :
			// vertex_point -> edge_point_courant -> face_point -> edge_point_précédent
			vector<int> quad = { vp_idx, ep_cur, fp_idx, ep_prev };
			newFaces.push_back(quad);
		}
	}

	// -----------------------------------------------------------------------
	// RECONSTRUCTION : remplacer l'ancien maillage par le nouveau
	// On utilise le même pattern twin_map que dans readFile.
	// -----------------------------------------------------------------------
	clear();

	// Créer les nouveaux sommets
	for (unsigned int i = 0; i < newPoints.size(); i++)
	{
		myVertex *v    = new myVertex();
		v->point       = new myPoint3D(newPoints[i].X, newPoints[i].Y, newPoints[i].Z);
		v->index       = (int)vertices.size();
		vertices.push_back(v);
	}

	// Créer les half-edges et les faces avec le pattern twin_map
	map<pair<int,int>, myHalfedge*> twin_map;
	for (unsigned int i = 0; i < newFaces.size(); i++)
	{
		int sz = (int)newFaces[i].size();
		vector<myHalfedge*> h(sz);
		for (int k = 0; k < sz; k++) h[k] = new myHalfedge();

		myFace *f = new myFace();
		f->adjacent_halfedge = h[0];

		for (int k = 0; k < sz; k++)
		{
			int kn = (k + 1) % sz;
			int kp = (k + sz - 1) % sz;

			h[k]->next         = h[kn];
			h[k]->prev         = h[kp];
			h[k]->adjacent_face = f;
			h[k]->source       = vertices[newFaces[i][k]];

			if (vertices[newFaces[i][k]]->originof == NULL)
				vertices[newFaces[i][k]]->originof = h[k];

			// Chercher le twin : l'arête inverse (kn -> k) doit déjà être dans la map
			pair<int,int> rev = make_pair(newFaces[i][kn], newFaces[i][k]);
			map<pair<int,int>, myHalfedge*>::iterator it = twin_map.find(rev);
			if (it != twin_map.end())
			{
				h[k]->twin      = it->second;
				it->second->twin = h[k];
			}
			else
			{
				twin_map[make_pair(newFaces[i][k], newFaces[i][kn])] = h[k];
			}

			h[k]->index = (int)halfedges.size();
			halfedges.push_back(h[k]);
		}

		f->index = (int)faces.size();
		faces.push_back(f);
	}

	normalize();
	computeNormals();
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

void myMesh::simplifyShortestEdgeCollapse(int n)
{
	for (int step = 0; step < n; step++)
	{
		if (halfedges.empty()) return;

		// Construire la liste des arêtes intérieures triées par longueur croissante
		// On évite les doublons (chaque arête = 1 halfedge sur 2) via un set
		map<myVertex*, int> vMap;
		for (unsigned int i = 0; i < vertices.size(); i++) vMap[vertices[i]] = i;

		vector<pair<double, myHalfedge*>> sortedEdges;
		set<pair<int,int>> seen;
		for (unsigned int i = 0; i < halfedges.size(); i++)
		{
			myHalfedge *e = halfedges[i];
			if (e->twin == NULL) continue; // ignorer les arêtes de bord
			int a = vMap[e->source], b = vMap[e->twin->source];
			if (a > b) std::swap(a, b); // clé canonique pour éviter les doublons
			if (seen.count(make_pair(a, b))) continue;
			seen.insert(make_pair(a, b));

			double dx = e->source->point->X - e->twin->source->point->X;
			double dy = e->source->point->Y - e->twin->source->point->Y;
			double dz = e->source->point->Z - e->twin->source->point->Z;
			sortedEdges.push_back(make_pair(sqrt(dx*dx + dy*dy + dz*dz), e));
		}
		sort(sortedEdges.begin(), sortedEdges.end());

		// Essayer les arêtes de la plus courte à la plus longue
		// Si la plus courte échoue (link condition), on essaie la suivante
		bool collapsed = false;
		for (unsigned int i = 0; i < sortedEdges.size(); i++)
		{
			myHalfedge *e = sortedEdges[i].second;
			// Règle 2 du cours : nouveau sommet = milieu des deux extrémités
			myPoint3D midpoint(
				(e->source->point->X + e->twin->source->point->X) * 0.5,
				(e->source->point->Y + e->twin->source->point->Y) * 0.5,
				(e->source->point->Z + e->twin->source->point->Z) * 0.5
			);
			if (simplifyShortestEdgeCollapse(e, midpoint))
			{
				collapsed = true;
				break; // collapse réussi, passer à l'étape suivante
			}
			// sinon : link condition refusée pour cette arête, on essaie la suivante
		}

		if (!collapsed) break; // aucune arête du maillage ne peut être collapsée
	}
	normalize();
	computeNormals();
}

bool myMesh::simplifyShortestEdgeCollapse(myHalfedge *best, myPoint3D pNew)
{
	map<myVertex*, int> vMap;
	for (unsigned int i = 0; i < vertices.size(); i++) vMap[vertices[i]] = i;

	int iKeep = vMap[best->source];
	int iKill = vMap[best->twin->source];

	// Build 1-ring neighbor sets for link condition check
	set<int> neighKeep, neighKill;
	for (unsigned int i = 0; i < halfedges.size(); i++)
	{
		int src = vMap[halfedges[i]->source];
		int dst = vMap[halfedges[i]->next->source];
		if (src == iKeep && dst != iKill) neighKeep.insert(dst);
		if (dst == iKeep && src != iKill) neighKeep.insert(src);
		if (src == iKill && dst != iKeep) neighKill.insert(dst);
		if (dst == iKill && src != iKeep) neighKill.insert(src);
	}
	vector<int> common;
	for (int v : neighKeep) if (neighKill.count(v)) common.push_back(v);
	if (common.size() != 2) return false;

	vector<vector<int>> newFaces;
	set<set<int>> seenFaces;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		vector<int> ids;
		myHalfedge *e = faces[i]->adjacent_halfedge;
		do { int idx = vMap[e->source]; if (idx == iKill) idx = iKeep; ids.push_back(idx); e = e->next; } while (e != faces[i]->adjacent_halfedge);
		set<int> u(ids.begin(), ids.end());
		if ((int)u.size() < 3) continue;
		if (seenFaces.count(u)) continue;
		seenFaces.insert(u);
		newFaces.push_back(ids);
	}

	map<int, int> remap;
	vector<myPoint3D> newPts;
	for (unsigned int i = 0; i < vertices.size(); i++)
	{
		if ((int)i == iKill) continue;
		remap[i] = (int)newPts.size();
		newPts.push_back((int)i == iKeep ? pNew : *(vertices[i]->point));
	}
	for (unsigned int i = 0; i < newFaces.size(); i++)
		for (unsigned int j = 0; j < newFaces[i].size(); j++)
			newFaces[i][j] = remap[newFaces[i][j]];

	clear();
	for (unsigned int i = 0; i < newPts.size(); i++)
	{
		myVertex *v = new myVertex();
		v->point = new myPoint3D(newPts[i].X, newPts[i].Y, newPts[i].Z);
		v->index = (int)vertices.size();
		vertices.push_back(v);
	}

	map<pair<int, int>, myHalfedge *> twin_map;
	for (unsigned int i = 0; i < newFaces.size(); i++)
	{
		int n = (int)newFaces[i].size();
		vector<myHalfedge *> h(n);
		for (int k = 0; k < n; k++) h[k] = new myHalfedge();
		myFace *f = new myFace();
		f->adjacent_halfedge = h[0];
		for (int k = 0; k < n; k++)
		{
			int kn = (k + 1) % n, kp = (k + n - 1) % n;
			h[k]->next = h[kn]; h[k]->prev = h[kp];
			h[k]->adjacent_face = f;
			h[k]->source = vertices[newFaces[i][k]];
			if (vertices[newFaces[i][k]]->originof == NULL) vertices[newFaces[i][k]]->originof = h[k];
			pair<int, int> rev = make_pair(newFaces[i][kn], newFaces[i][k]);
			map<pair<int, int>, myHalfedge *>::iterator it = twin_map.find(rev);
			if (it != twin_map.end()) { h[k]->twin = it->second; it->second->twin = h[k]; }
			else twin_map[make_pair(newFaces[i][k], newFaces[i][kn])] = h[k];
			h[k]->index = (int)halfedges.size();
			halfedges.push_back(h[k]);
		}
		f->index = (int)faces.size();
		faces.push_back(f);
	}

	return true;
}

