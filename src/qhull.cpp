/***************************************************************************
                          qhull.cpp  -  GDL library function
                             -------------------
    begin                : Jun 09 2021
    copyright            : (C) 2021 by Eloi R.de Linage

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "includefirst.hpp"
#include "datatypes.hpp"
#include "envt.hpp"
#include "dinterpreter.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <array>

//Qhull libraries

#include <libqhullcpp/PointCoordinates.h>
#include <libqhullcpp/RboxPoints.h>
#include <libqhullcpp/QhullError.h>
#include <libqhullcpp/QhullQh.h>
#include <libqhullcpp/QhullFacet.h>
#include <libqhullcpp/QhullFacetList.h>
#include <libqhullcpp/QhullFacetSet.h>
#include <libqhullcpp/QhullLinkedList.h>
#include <libqhullcpp/QhullPoint.h>
#include <libqhullcpp/QhullRidge.h>
// #include <libqhullcpp/QhullUser.h>
#include <libqhullcpp/QhullVertex.h>
#include <libqhullcpp/QhullVertexSet.h>
#include <libqhullcpp/Qhull.h>

//debugging only
#include <chrono>

 //DEBUGGING
          using std::chrono::high_resolution_clock;
          using std::chrono::duration_cast;
          using std::chrono::duration;
          using std::chrono::milliseconds;

using namespace std;

using orgQhull::Qhull;
using orgQhull::QhullError;
using orgQhull::QhullFacet;
using orgQhull::QhullFacetList;
using orgQhull::QhullFacetListIterator;
using orgQhull::QhullFacetSet;
using orgQhull::QhullFacetSetIterator;
using orgQhull::QhullPoint;
using orgQhull::QhullPoints;
using orgQhull::QhullPointsIterator;
using orgQhull::QhullQh;
using orgQhull::QhullRidge;
using orgQhull::QhullRidgeSet;
//using orgQhull::QhullUser;
using orgQhull::QhullVertex;
using orgQhull::QhullVertexList;
using orgQhull::QhullVertexListIterator;
using orgQhull::QhullVertexSet;
using orgQhull::QhullVertexSetIterator;
using orgQhull::RboxPoints;
using orgQhull::PointCoordinates;

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;

namespace lib {

  void qhull ( EnvT* e)
  {
    // There might be a simpler/cleaner way to do this...
    static int connIx=e->KeywordIx("CONNECTIVITY");
    bool isConn=e->KeywordPresent(connIx);

    static int boundsIx=e->KeywordIx("BOUNDS");
    bool isBounds=e->KeywordPresent(boundsIx);

    static int vnormIx=e->KeywordIx("VNORMALS");
    bool isVnorm=e->KeywordPresent(vnormIx);

    static int vdiagIx=e->KeywordIx("VDIAGRAM");
    bool isVdiag=e->KeywordPresent(vdiagIx);

    static int vvertIx=e->KeywordIx("VVERTICES");
    bool isVvert=e->KeywordPresent(vvertIx);

    bool isVoronoi= isVnorm || isVdiag || isVvert;

    static int sphereIx=e->KeywordIx("SPHERE");
    bool isSphere=e->KeywordPresent(sphereIx);

    static int delaunayIx=e->KeywordIx("DELAUNAY");
    bool isDelaunay= ( e->KeywordSet(delaunayIx) || isVoronoi); //switch to delaunay if a voronoi keyword is set

    if(isConn & !isDelaunay) e->Throw("Keyword CONNECTIVITY requires the presence of keyword DELAUNAY/SPHERE.\nSPHERE is not implemented yet.");        
    if(isSphere) e->Throw("SPHERE is not implemented yet.");

    int nParam = e->NParam(2);
    int outIx = nParam-1;
  
    DDoubleGDL* p0;

    if(nParam == 2){
        p0 = e->GetParAs<DDoubleGDL>(0);
    } else {
        int inDim = e->GetParAs<DDoubleGDL>(0)->Dim(0);
        p0 = new DDoubleGDL( *(new dimension(nParam-1, inDim)), BaseGDL::ZERO );

        for(int i=0; i<nParam-1; i++)
        {
          DDoubleGDL* par=e->GetParAs<DDoubleGDL>(i);
          if(par->Dim(0) != inDim || par->Dim(1) != 0 )
          {
            e->Throw("qhull input error: separated input arrays must have same length and be 1 dimensional");
          }
          for(int j=0; j<inDim; j++) (*p0)[i+j*(nParam-1)] = (*par)[j];
        }
    }

    Qhull qhull;

    int nd=p0->Dim(0);
    int np=p0->Dim(1);

    if(nd == 0 || np == 0) e->Throw("qhull input error: array must have 2 dimensions");

    //if not enough input points for qhull, throw error
    if( np <= nd){
      stringstream ssp, ss_minp;
      ssp << np;
      ss_minp << nd+1;
      string err_string="qhull input error: not enough points ("+ ssp.str() +") to construct initial simplex (need "+ ss_minp.str() +")";
      e->Throw(err_string);
    } else if ((np <= nd+1) && isDelaunay) {
      e->Throw("qhull input error: not enough points available");
    }

    PointCoordinates *mPoints = new PointCoordinates();
    mPoints->setDimension(nd);
    
    vector <double> allPoints;
    for (int i =0; i < p0->N_Elements(); i++){
      allPoints.push_back((*p0)[i]);
      if(!isfinite((*p0)[i])) e->Throw("Infinite or invalid (NaN) operands not allowed.");
    }
          
    mPoints->append(allPoints);
    int ndRes;

    if(isVoronoi)
    {
        qhull.runQhull( mPoints->comment().c_str(), nd, np, mPoints->coordinates(), "QJ v Qbb Pp");
        ndRes=nd+1;
    } else if(isDelaunay) {
        // QJ option prevents non-simplicial facets, it seems to be the option IDL uses
        // While Qt option triangulates facets afterwards, which gives somewhat different results in some cases

        // Delaunay triangulation has issues with integers (ex:
        //                                              a=[[0,0,0],[1,0,0],[0,1,0],[0,0,1],[1,1,1]] & qhull, a, b, /DELAUNAY
        //                                                    crashes...)
        qhull.runQhull( mPoints->comment().c_str(), nd, np, mPoints->coordinates(), "QJ d Pp");
        ndRes=nd+1;
    } else {
        qhull.runQhull( mPoints->comment().c_str(), nd, np, mPoints->coordinates(), "QJ Pp");
        ndRes=nd;
    }

    QhullFacetList facets = qhull.facetList();
    // count bad facets
    int bad_facets=0;
    for (QhullFacetList::iterator it = facets.begin(); it != facets.end(); ++it)
        if (!(*it).isGood())
          bad_facets++;

    DLongGDL* res = new DLongGDL( *(new dimension(ndRes, qhull.facetCount()-bad_facets)), BaseGDL::ZERO);
    
    int ix=0;

    for (QhullFacetList::iterator it = facets.begin(); it != facets.end(); ++it)
    {
        if (!(*it).isGood()) continue;
        QhullFacet f = *it;
        QhullVertexSet vSet = f.vertices();
        for (QhullVertexSet::iterator vIt = vSet.begin(); vIt != vSet.end(); ++vIt)
        {
          (*res)[ix] = (*vIt).point().id();
          ix++;
        }
    }

    /*if(bad_facets > 0){
        if(isDelaunay){
          res->SetDim(*(new dimension(nd+1, qhull.facetCount()-bad_facets)));
        } else {
          res->SetDim(*(new dimension(nd, qhull.facetCount()-bad_facets)));
        }
    }*/

    if(isBounds)
    {
      vector<long> boundsV;
      for(int i=0; i<res->Dim(0)*res->Dim(1); ++i)
          if( (find(boundsV.begin(), boundsV.end(), (*res)[i]) == boundsV.end()))
            boundsV.push_back( (*res)[i] );
      DLongGDL* bounds=new DLongGDL( *(new dimension(boundsV.size())), BaseGDL::ZERO );
      for(int i=0; i <boundsV.size(); ++i){
          (*bounds)[i] = boundsV.at(i);
      }
      e->SetKW(boundsIx, bounds);
    }

    if(isConn)
    {
      vector<vector<long>>connV(qhull.vertexCount());
      qhull.defineVertexNeighborFacets();
      QhullVertexList vertices = qhull.vertexList();

      long connCount=0;

      for(QhullVertexList::iterator vIt = vertices.begin(); vIt != vertices.end(); vIt++)
      {
        QhullVertex vertex = *vIt;
        long vertexId = vertex.point().id();
        QhullFacetSet neighbors = vertex.neighborFacets();
        vector<long> neighborsV;

        for(QhullFacetSet::iterator nIt = neighbors.begin(); nIt != neighbors.end(); ++nIt)
        {
          if(!(*nIt).isGood()) continue;
          QhullFacet neighbor = *nIt;
          QhullVertexSet neighborVSet = neighbor.vertices();
          for (QhullVertexSet::iterator nVIt = neighborVSet.begin(); nVIt != neighborVSet.end(); ++nVIt)
          {
              QhullVertex v = *nVIt;
              QhullPoint p = v.point();
              if( (find(neighborsV.begin(), neighborsV.end(), (long) p.id()) == neighborsV.end()) && p.id() != vertexId)
              {
                neighborsV.push_back(p.id());
                connCount++;
              }
          }
        }
        connV.at(vertexId) = neighborsV;
      }

      DLongGDL* conn = new DLongGDL(*(new dimension( np + 1 + connCount )), BaseGDL::ZERO);
      
      int writeIx = np+1;
      (*conn)[np]=np+1+connCount;
      for(int i=0; i < connV.size(); i++)
      {
          vector<long> neighborsV = connV.at(i);
          for(int j=0; j<neighborsV.size(); j++) (*conn)[ writeIx+j ] = neighborsV.at(j);
          (*conn)[i] = writeIx;
          writeIx += neighborsV.size();
      }
      e->SetKW(connIx, conn);
    }

    if(isVoronoi)
    {
      // prepare voronoi

      // bool isLower;            //not used
      // int voronoiVertexCount;  //not used
      // qhull.prepareVoronoi(&isLower, &voronoiVertexCount); // useful ?

      // retrieve voronoi vertices

      vector<vector<double> > vVertices;
      for(QhullFacetList::iterator it = facets.begin(); it != facets.end(); ++it)
      {
        if (!(*it).isGood()) continue;
        QhullFacet f = *it;
        vVertices.push_back(f.getCenter().toStdVector());
      }

      if(isVvert)
      {
        DDoubleGDL* vvert=new DDoubleGDL( *(new dimension(nd, vVertices.size())), BaseGDL::ZERO);
        int nvVert=vVertices.size();
        for(int i=0; i < nvVert; ++i)
          for(int j=0; j<nd; ++j)
            (*vvert)[nd*i+j]=vVertices.at(i).at(j);
        e->SetKW(vvertIx, vvert);
      }

      // retrieve voronoi normals
      
      // QhullUser only appeared in latest versions of qhull, so we avoid using it

      // QhullUser results((&qhull)->qh());
      // (&qhull)->outputQhull("Fo");
      // vector<vector<double>> normsPars= results.doublesVector();
      // vector<vector<int>> normsVertId = results.intsVector();
      

      stringbuf strbuf;
      ostream os(&strbuf);
      qhull.setOutputStream( &os );
      qhull.outputQhull("Fo");
      qhull.clearQhullMessage(); // to prevent qhull printing in console
      stringstream ss;
      ss << os.rdbuf();

      int n_lines;
      ss >> n_lines;

      vector<vector<double>> normsPars;
      vector<vector<int>> normsVertId;

      // Temp variables
      vector<double> normsPars_line(nd+1);
      double n_coeffs, vert1_id, vert2_id;

      for(int l = 0; l < n_lines; ++l)
      {
        ss >> n_coeffs;

        ss >> vert1_id;
        ss >> vert2_id;
        normsVertId.push_back({(int) vert1_id, (int) vert2_id});

        for(int i=0; i<nd+1; ++i) ss >> normsPars_line[i];
        normsPars.push_back(normsPars_line);
      }

      if(isVnorm)
      {
        int nVnorm = normsPars.size();
        DDoubleGDL* vnorm=new DDoubleGDL(*(new dimension(nd+1, nVnorm)), BaseGDL::ZERO);
        for(int i=0; i < nVnorm; ++i)
          for(int j=0; j<nd+1; ++j)
            (*vnorm)[i*(nd+1) + j] = normsPars[i][j];

        e->SetKW(vnormIx, vnorm);
      }

      // make voronoi diagrams array (see IDL QHULL doc)

      if(isVdiag){

        // stringbuf strbuf;
        // ostream os(&strbuf);
        os.clear();
        ss.clear();

        qhull.setOutputStream( &os );
        qhull.outputQhull("Fv");
        qhull.clearQhullMessage(); // to prevent qhull printing in console

        // stringstream ss;
        ss << os.rdbuf();

        int useless_first_int;
        ss >> useless_first_int;
        
        if(nd==2) // 2D case
        {
          // Format the data
          int current_int, nVdiag, n_indices;
          int line_ix=0;

          vector<vector<int>>q_vdiag;
          vector<int> line;
          while ( ss >> current_int )
          {
              if(line_ix == 0){
                n_indices = current_int;
              } else {
                line.push_back(current_int);
              }
              line_ix++;
              if(line_ix == n_indices+1){
                q_vdiag.push_back(line);
                line.clear();
                line_ix=0;
              }
          }
          nVdiag = q_vdiag.size();

          // Handle unbounded half-spaces
          for(int i=0; i<nVdiag; i++){
            for(int j=2; j<4; ++j){
              if(q_vdiag[i][j] == 0){
                for(int k=0; k<normsVertId.size(); k++){
                  if(q_vdiag[i][0] == normsVertId[k][1] && q_vdiag[i][1] == normsVertId[k][2]){
                    q_vdiag[i][j] = -k;
                    break;
                  }
                }
                break;
              }
            }
          }
          
          DLongGDL* vdiag=new DLongGDL(*(new dimension(nd*2, nVdiag)), BaseGDL::ZERO);
          for(int i=0; i < nVdiag; ++i)
          {
              for(int j=0; j<nd; ++j)
              {
                (*vdiag)[i*2*nd]=q_vdiag[i][0];
                (*vdiag)[i*2*nd+1]=q_vdiag[i][1];
              }
              for(int k=0; k< nd; k++)
              {
                (*vdiag)[i*2*nd+2]=q_vdiag[i][2]-1;
                (*vdiag)[i*2*nd+3]=q_vdiag[i][3]-1;
              }
          }
          e->SetKW(vdiagIx, vdiag);
        }
        else // For NDimension > 2
        {
          // Format the data, a bit differently from the 2D case since we keep, for each line, the number of indices
          int current_int, nVdiag, n_indices;
          int line_ix=0;
          int vdiag_length=0;

          vector<vector<int>>q_vdiag;
          vector<int>line;
          while ( ss >> current_int )
          {
              if(line_ix == 0) n_indices = current_int;
              line.push_back(current_int);
              line_ix++;
              if(line_ix == n_indices+1){
                q_vdiag.push_back(line);
                vdiag_length += n_indices+1;
                line.clear();
                line_ix=0;
              }
          }
          nVdiag = q_vdiag.size();

          // Handle unbounded half-spaces
          for(int i=0; i<nVdiag; i++){
            for(int j=2; j<q_vdiag.size(); ++j){
              if(q_vdiag[i][j] == 0){
                for(int k=0; k<normsVertId.size(); k++){
                  if(q_vdiag[i][1] == normsVertId[k][1] && q_vdiag[i][2] == normsVertId[k][2]){
                    q_vdiag[i][j] = -k;
                    break;
                  }
                }
                break;
              }
            }
          }
          
          DLongGDL* vdiag=new DLongGDL(*(new dimension(vdiag_length)), BaseGDL::ZERO);
          int writeIx=0;
          for(int i=0; i < nVdiag; ++i)
          {
              for(int j=0; j<3; ++j)
              {
                (*vdiag)[writeIx]=q_vdiag[i][j];
                writeIx++;
              }
              for(int j=3; j< q_vdiag[i].size(); j++)
              {
                (*vdiag)[writeIx]=q_vdiag[i][j]-1;
                writeIx++;
              }
          }
          e->SetKW(vdiagIx, vdiag);
        }
      }
    }

    e->SetPar(outIx, res);
  }

  // sub_functions for qgrid3

  // 3D vector struct

  struct Vec3{ double x, y, z; };

  inline Vec3 cross_prod(Vec3 a, Vec3 b)
  {
    return {a.y*b.z - a.z*b.y, b.x*a.z - a.x*b.z ,  a.x*b.y - a.y*b.x };
  }

  inline double dot_prod3d(Vec3 a, Vec3 b)
  {
    return (a.x*b.x + a.y*b.y + a.z*b.z);
  }

  inline double scalar_triple_prod(Vec3 a, Vec3 b, Vec3 c)
  {
    return dot_prod3d(a, cross_prod(b,c));
  }

  inline Vec3 vec3_substract(Vec3 a, Vec3 b)
  {
    return {a.x-b.x, a.y-b.y, a.z-b.z};
  }

  Vec3 box_upper_limit(vector<Vec3> *points)
  {
    Vec3 max_coord = (*points)[0];
    for(int i=0; i<(*points).size(); i++){
      max_coord.x = max(max_coord.x, (*points)[i].x);
      max_coord.y = max(max_coord.y, (*points)[i].y);
      max_coord.z = max(max_coord.z, (*points)[i].z);
    }
    return max_coord;
  }

  Vec3 box_lower_limit(vector<Vec3> *points)
  {
    Vec3 min_coord = (*points)[0];
    for(int i=0; i<(*points).size(); i++){
      min_coord.x = min(min_coord.x, (*points)[i].x);
      min_coord.y = min(min_coord.y, (*points)[i].y);
      min_coord.z = min(min_coord.z, (*points)[i].z);
    }
    return min_coord;
  }

  // Tetrahedron Struct

  struct Tetra{
    Vec3 a,b,c,d;
    array<int,4> vertices;   //ids of vertices in points vector
    array<int,4> neighbours; //neighbour[0] holds the id of the simplex opposite a (sharing bcd facet)
  };
/*
  Tetra TetraConstr(vector<double> a, vector<double> b, vector<double> c, vector<double> d, vector<int> vertices)
  {
    vector<double>vab = vector_substract(b, a),
    vac = vector_substract(c, a),
    vad = vector_substract(d, a),
    vbc = vector_substract(c, b),
    vbd = vector_substract(d, b);
    double v6 = 1/scalar_triple_prod(vab, vac, vad);
    Tetra tet = {a, b, c, d, vab, vac, vad, vbc, vbd, v6, vertices};
    return tet;
  }
   */

    // tet struct methods

  int inside_outside_tet(Tetra *tet, Vec3* p)
  {
    Vec3 vap = vec3_substract(*p, tet->a);
    Vec3 vbp = vec3_substract(*p, tet->b);

    Vec3 vab = vec3_substract(tet->b, tet->a);
    Vec3 vac = vec3_substract(tet->c, tet->a);
    Vec3 vad = vec3_substract(tet->d, tet->a);

    Vec3 vbc = vec3_substract(tet->c, tet->b);
    Vec3 vbd = vec3_substract(tet->d, tet->b);

    double v6 = 1/scalar_triple_prod(vab, vac, vad);

    double va6 = scalar_triple_prod(vbp, vbd, vbc);
    if(va6*v6 < 0) return 1;
    double vb6 = scalar_triple_prod(vap, vac, vad);
    if(vb6*v6 < 0) return 2;
    double vc6 = scalar_triple_prod(vap, vad, vab);
    if(vc6*v6 < 0) return 3;
    double vd6 = scalar_triple_prod(vap, vab, vac);
    if(vd6*v6 < 0) return 4;
    return 0;
  }
 

  inline array<double, 4> bary_tet(Tetra *tet, Vec3* p)
  {
    Vec3 vap = vec3_substract(*p, tet->a);
    Vec3 vbp = vec3_substract(*p, tet->b);

    Vec3 vab = vec3_substract(tet->b, tet->a);
    Vec3 vac = vec3_substract(tet->c, tet->a);
    Vec3 vad = vec3_substract(tet->d, tet->a);

    Vec3 vbc = vec3_substract(tet->c, tet->b);
    Vec3 vbd = vec3_substract(tet->d, tet->b);

    double va6 = scalar_triple_prod(vbp, vbd, vbc);
    double vb6 = scalar_triple_prod(vap, vac, vad);
    double vc6 = scalar_triple_prod(vap, vad, vab);
    double vd6 = scalar_triple_prod(vap, vab, vac);

    double v6 = 1/scalar_triple_prod(vab, vac, vad);

    return array<double,4> {va6*v6, vb6*v6, vc6*v6, vd6*v6};
  }

  BaseGDL* qgrid3_fun ( EnvT* e)
  {
    //check and get parameters...
    int nParam = e->NParam(3);
  
    DDoubleGDL* p0;
    DDoubleGDL * func;
    DLongGDL * tetra_list;

    if(nParam == 3)
    {
        p0 = e->GetParAs<DDoubleGDL>(0);         //input points
        func = e->GetParAs<DDoubleGDL>(1);       //input function
        tetra_list = e->GetParAs<DLongGDL>(2);   //indices of tetrahedra vertices from qhull
    } else { // if input coordinates are in separate arrays
        e->NParam(5);
        if (nParam>5) e->Throw("Incorrect number of arguments.");

        func = e->GetParAs<DDoubleGDL>(3);     //input function
        tetra_list = e->GetParAs<DLongGDL>(4); //indices of tetrahedra vertices from qhull

        int inDim = e->GetParAs<DDoubleGDL>(0)->Dim(0);
        p0 = new DDoubleGDL( *(new dimension(3, inDim)), BaseGDL::ZERO ); //concatenation of the 3 separate inputs arrays

        for(int i=0; i<3; i++)
        {
          DDoubleGDL* par=e->GetParAs<DDoubleGDL>(i);
          if(par->Dim(0) != inDim || par->Dim(1) != 0 )
          {
            e->Throw("separated input arrays must have same length and be 1 dimensional");
          }
          for(int j=0; j<inDim; j++) (*p0)[i+j*3] = (*par)[j];
        }
    }

    int n_tetra = tetra_list->Dim(1);
    int nd=p0->Dim(0);
    size_t np=p0->Dim(1);

    // x,y,z dimensions of grid
    dimension* res_dim;
    array<int, 3> res_dim_vec = {25,25,25}; // array caring dims
    static int dimensionIx=e->KeywordIx("DIMENSION");
    if(e->KeywordPresent(dimensionIx)){
      DLongGDL * dimPar = e->GetKWAs<DLongGDL>(dimensionIx);
      int dimNelem = dimPar->N_Elements();
      if(dimNelem <= 3 && dimNelem >0){
        res_dim_vec={(*dimPar)[0], (*dimPar)[(dimNelem>1)], (*dimPar)[2*(dimNelem>1)]};
      } else {
        e->Throw("Keyword array parameter DIMENSION must have from 1 to 3 elements.");
      }
    }
    res_dim = new dimension(res_dim_vec[0], res_dim_vec[1], res_dim_vec[2]);

    // putting input points in a vector...
    vector<Vec3> points;
    points.reserve(np);

    for (int i =0; i < np; i++)
    {
      points[i] = { (*p0)[3*i], (*p0)[3*i+1], (*p0)[3*i+2] };
      //TODO handle not finite values
    }

    // vector holding all necessary info on the triangulation
    vector<Tetra> tetra_data;
    tetra_data.reserve(n_tetra);
    
    // directly available info
    array<int,4> empty_neighbours {-1,-1,-1,-1};
    for(int i=0; i<n_tetra; ++i){
      //TODO handle out of bounds vertices
      array<int,4> vertices = {(*tetra_list)[4*i],(*tetra_list)[4*i+1], (*tetra_list)[4*i+2], (*tetra_list)[4*i+3]};
      sort(vertices.begin(), vertices.end());
      tetra_data[i] = { points[vertices[0]], points[vertices[1]],points[vertices[2]],points[vertices[3]], vertices, empty_neighbours};
    }

     //start debug - time section 1
    auto t3 = high_resolution_clock::now();
    //end debug - time section 1

    // find the neighbours of each simplex
    // TOO SLOW, FIND ANOTHER METHOD

    vector<int> tetra_neigh_count(n_tetra, 0);
    tetra_neigh_count.reserve(n_tetra);
    
    for(int i=0; i<n_tetra; ++i)
    {
      if(tetra_neigh_count[i] == 4) continue;
      for(int j=i+1; j<n_tetra; ++j)
      {
        int diff[4] = {0,0,0,0};
        set_difference(tetra_data[i].vertices.begin(), tetra_data[i].vertices.end(), tetra_data[j].vertices.begin(), tetra_data[j].vertices.end(), diff);
        if(diff[1] == 0)
        {          
          int diff_neigh[4] = {0,0,0,0};
          set_difference(tetra_data[j].vertices.begin(), tetra_data[j].vertices.end(), tetra_data[i].vertices.begin(), tetra_data[i].vertices.end(), diff_neigh);

          int neigh_vert_Id=find(tetra_data[j].vertices.begin(), tetra_data[j].vertices.end(), diff_neigh[0]) - tetra_data[j].vertices.begin();
          tetra_data[j].neighbours[neigh_vert_Id] = i;

          int tet_vert_Id=find(tetra_data[i].vertices.begin(), tetra_data[i].vertices.end(), diff[0]) - tetra_data[i].vertices.begin();
          tetra_data[i].neighbours[tet_vert_Id] = j;

          tetra_neigh_count[i]++;
          tetra_neigh_count[j]++;

          if(tetra_neigh_count[i] == 4) break;
        }
      }
    }
    
    /*
    vector<short> neighbours_count(n_tetra, 0);

    for(size_t i=0; i<n_tetra; ++i)
    {
      for(size_t j=i+1; j<n_tetra; ++j)
      {
          int commonVertCount=0;
          int tetVertId=6;
          int neighVertId=6;
          for(int k=0; k<4; ++k)
          {
            for(int l=0; l<4; ++l)
            {
              if(tetra_data[j].vertices[l] == tetra_data[i].vertices[k])
              {
                neighVertId = neighVertId-l;
                tetVertId = tetVertId-k;
                commonVertCount++;
                break;
              }
            }
          }
          if(commonVertCount == 3)
          {
            tetra_data[i].neighbours[tetVertId] = j;
            tetra_data[j].neighbours[neighVertId] = i;
            neighbours_count[i]++;
          }
          if(neighbours_count[i] == 4) break;
      }
    }
    */

    // find the neighbours of each simplex (not optimal at all)
    /*
    for(int i=0; i<n_tetra; ++i)
    {
      for(int j=0; j<4; ++j)
      {
        vector<int> common_vertices;
        for(int k=0; k<4; k++) if(k != j) common_vertices.push_back(tetra_data[i].vertices[k]);

        for(int k=0; k<n_tetra;k++)
        {
          if(k != i)
          {
            bool find_a=( find(tetra_data[k].vertices.begin(), tetra_data[k].vertices.end(), common_vertices[0]) !=  tetra_data[k].vertices.end());
            bool find_b=( find(tetra_data[k].vertices.begin(), tetra_data[k].vertices.end(), common_vertices[1]) !=  tetra_data[k].vertices.end());
            bool find_c=( find(tetra_data[k].vertices.begin(), tetra_data[k].vertices.end(), common_vertices[2]) !=  tetra_data[k].vertices.end());
            if(find_a && find_b && find_c)
            {
              tetra_data[i].neighbours[j] = k;
              break;
            }
          }
        }
      }
    }
    */

    //start debug - time section 2
    auto t4 = high_resolution_clock::now();
    duration<double, std::milli> ms_double2 = t4 - t3;
    double tot_function_time2 = ms_double2.count();
    // end debug - time section 2

    // we need to get max and min x, y and z...
    Vec3 min_coord = box_lower_limit(&points);
    Vec3 max_coord = box_upper_limit(&points);

    // start of grid
    Vec3 start_coord = min_coord;
    static int startIx=e->KeywordIx("START");
    if(e->KeywordPresent(startIx)){
      DDoubleGDL * startPar = e->GetKWAs<DDoubleGDL>(startIx);
      int startNelem = startPar->N_Elements();
      if(startNelem <= 3 && startNelem >0){
        start_coord = {(*startPar)[0], (*startPar)[ (startNelem>1) ], (*startPar)[ 2*(startNelem>2) ]};
      } else {
        e->Throw("Keyword array parameter START must have from 1 to 3 elements.");
      }
    }

    // grid spacing
    Vec3 delta;
    // default grid spacing is determined using max values of point set
    delta.x = (max_coord.x - start_coord.x) / double(res_dim_vec[0]);
    delta.y = (max_coord.y - start_coord.y) / double(res_dim_vec[1]);
    delta.z = (max_coord.z - start_coord.z) / double(res_dim_vec[2]);

    static int deltaIx=e->KeywordIx("DELTA");
    if(e->KeywordPresent(deltaIx)){
      DDoubleGDL * deltaPar = e->GetKWAs<DDoubleGDL>(deltaIx);
      int deltaNelem = deltaPar->N_Elements();
      if(deltaNelem <= 3 && deltaNelem >0){
        delta = {(*deltaPar)[0], (*deltaPar)[(deltaNelem>1)], (*deltaPar)[2*(deltaNelem>2)]};
      } else {
        e->Throw("Keyword array parameter DELTA must have from 1 to 3 elements.");
      }
    }

    double missing = 0;
    static int missingIx=e->KeywordIx("MISSING");
    if(e->KeywordPresent(missingIx)) missing = (*e->GetKWAs<DDoubleGDL>(missingIx))[0];

    DDoubleGDL * res = new DDoubleGDL(*res_dim, BaseGDL::ZERO);
    Vec3 coord;

    size_t last_tetraId=0;        // index of tetrahedron where the last point was found
    int tot_walk_count=0;

    //DEBUG
    double tot_function_time=0;

    //start debug - time section 1
    auto t1 = high_resolution_clock::now();
    //end debug - time section 1

    // Loop to find container simplex for each point, and interpolate if it is found

    for(size_t i=0; i < res_dim_vec[2]; i++ ){
      coord.z = start_coord.z+i*delta.z;     // Z coord
      for(size_t j=0; j<res_dim_vec[1]; j++){
        coord.y = start_coord.y + j*delta.y; // Y coord
        for(size_t k=0; k<res_dim_vec[0]; k++){
          coord.x=start_coord.x+k*delta.x;   // X coord

          size_t res_index = k + j*res_dim_vec[0] + i*res_dim_vec[0]*res_dim_vec[1];
          double interp_value=missing;
          int step_count = 0;
          size_t tetraId = last_tetraId;

          // cout << "Point: " << res_index << " "<< coord.x << " " <<coord.y << " " << coord.z <<endl;

          // walk through simplices smarlty, starting from the simplex the last point was found in
          for(size_t l=0; l<n_tetra; l++)
          {
            Tetra tet = tetra_data[tetraId];
            //int io_res = inside_outside_tet(&tet,&coord);

            array<double,4> bary_coord = bary_tet(&tet,&coord);
            int minCoordIndex = min_element(bary_coord.begin(),bary_coord.end()) - bary_coord.begin();

            if(bary_coord[minCoordIndex] >= 0){
              // we found our simplex
              last_tetraId=tetraId;
              interp_value = bary_coord[0]*(*func)[tet.vertices[0]] + bary_coord[1]*(*func)[tet.vertices[1]] + bary_coord[2]*(*func)[tet.vertices[2]] + bary_coord[3]*(*func)[tet.vertices[3]];
              break;
            } else {
              // it's the wrong simplex but we know where to look next, thx to inside_outside_tet info
              size_t next_tetraId=tet.neighbours[minCoordIndex];
              if(next_tetraId==-1){
                // we are outside of the convex hull
                last_tetraId=tetraId;
                break;
              } else {
                // we walk to the next simplex
                tetraId=next_tetraId;
              }
            }
            step_count++;
          }
          (*res)[res_index] = interp_value;
          tot_walk_count+=step_count+1;
        }
      }
    }

    //start debug - time section 2
    auto t2 = high_resolution_clock::now();
    duration<double, std::milli> ms_double = t2 - t1;
    tot_function_time += ms_double.count();
    // end debug - time section 2

    /*
    cout <<"Total walk count: " << tot_walk_count << endl;
    cout <<"Time spent in section 1 (seconds): " << tot_function_time/1000 << endl;
    cout <<"Time spent in section 2 (seconds): " << tot_function_time2/1000 << endl;
    */
    
    return res;
  }
}