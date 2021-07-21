/***************************************************************************
                          triangulation.cpp  -  GDL library function
                             -------------------
    begin                : Aug 30 2017
    copyright            : (C) 2017 by Gilles Duvert

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

using namespace std;
#include "delaunator/delaunator-header-only.hpp"
#include "tripack.c"
#include "stripack.c"
#include "ssrfpack.c"
//#include "akima760.c"
//#include "akima761.c"

namespace lib {

//  using namespace std;
  
  inline size_t nextHalfedge(size_t e) { return (e % 3 == 2) ? e - 2 : e + 1; }  
  inline size_t prevHalfedge(size_t e) { return (e % 3 == 0) ? e + 2 : e - 1; }  
  inline size_t triangleOfEdge(size_t e)  { return floor(e / 3); }
  
  void GDL_Triangulate(EnvT* e) {

    DDoubleGDL *xVal, *yVal, *fvalue;
    DLong npts;
    SizeT nParam = e->NParam();
    if (nParam < 2) e->Throw("Incorrect number of arguments."); //actually IDL permits to *not* have the 3rd argument.
    bool wantsTriangles=(nParam > 2);
    bool wantsEdge=(nParam == 4);
    if (wantsTriangles)  e->AssureGlobalPar(2); //since we return values in it?  
    if (wantsEdge)  e->AssureGlobalPar(3); //since we return values in it?  

    static int sphereIx=e->KeywordIx( "SPHERE");
    static int degreeIx=e->KeywordIx( "DEGREES");
    static int fvalueIx=e->KeywordIx( "FVALUE");
    bool isSphere=(e->KeywordPresent(sphereIx));
    bool doDegree=(e->KeywordPresent(degreeIx));
    bool hasFvalue=(e->KeywordPresent(fvalueIx));
    if (!isSphere) {
      doDegree=false;
      hasFvalue=false;
    }
    bool wantsDupes=false;
    static int dupesIx=e->KeywordIx( "REPEATS");
    if( e->KeywordPresent( dupesIx)) wantsDupes=true;
    
    bool wantsConnectivity=false;
    static int connIx=e->KeywordIx( "CONNECTIVITY");
    if( e->KeywordPresent( connIx)) wantsConnectivity=true;
    
    //check since we return values in them 
    if (isSphere)  e->AssureGlobalKW(sphereIx); 
    if (hasFvalue)  e->AssureGlobalKW(fvalueIx);
    if (wantsDupes)  e->AssureGlobalKW(dupesIx); 
    if (wantsConnectivity)  e->AssureGlobalKW(connIx);
    
    //some things depend on X and Y being doubles of not
    bool isDouble=false;
    if ( e->GetParDefined(0)->Type()==GDL_DOUBLE &&  e->GetParDefined(1)->Type()==GDL_DOUBLE )  isDouble=true;
    xVal = e->GetParAs< DDoubleGDL > (0);
    if (xVal->Rank() == 0) e->Throw("Expression must be an array in this context: " + e->GetParString(0));
    npts = xVal->N_Elements();
    yVal = e->GetParAs< DDoubleGDL > (1);
    if (yVal->Rank() == 0) e->Throw("Expression must be an array in this context: " + e->GetParString(1));
    if (yVal->N_Elements() != npts) e->Throw("X & Y arrays must have same number of points.");
    if (hasFvalue) {
      fvalue=e->GetKWAs<DDoubleGDL>(fvalueIx);
      if (fvalue->Rank() == 0) e->Throw("Expression must be an array in this context: " +  e->GetString( fvalueIx)+".");
      if (fvalue->N_Elements() != npts) e->Throw("X & Y arrays must have same number of points."); //yes yes.
    }
   
    DLong maxEl, minEl;
    DDouble maxVal;
    xVal->MinMax(&minEl,&maxEl,NULL,NULL,false);
    //maximum ABSOLUTE value
    maxVal=abs((*xVal)[maxEl]);
    yVal->MinMax(&minEl,&maxEl,NULL,NULL,false);
    //maximum ABSOLUTE value
    maxVal=std::max(maxVal,abs((*yVal)[maxEl]));

    DDouble dtol = isDouble ? 1e-12 : 1e-6;
    //Tol is irrelevant in our implementation, as (s)tripack work with tol=machine precision. 
    
    DDouble* xx=&(*xVal)[0];
    DDouble* yy=&(*yVal)[0];    

    //IN SPHERE MODE, xVal and yVal ARE RETURNED, ARE DOUBLE PRECISION and their ORDER is MODIFIED,
    //the input points are sorted using the coordinate (x or y) covering max range (y prefered). It is not the case here.
    //SPHERE does not support duplicate points yet.
    
    if (isSphere) {
      DDoubleGDL *sc_xVal=xVal;
      DDoubleGDL *sc_yVal=yVal;
      Guard<BaseGDL>xguard;
      Guard<BaseGDL>yguard;
      if (doDegree) {
        static DDouble DToR=double(3.1415926535897932384626433832795)/180.0;
        sc_xVal=new DDoubleGDL(npts,BaseGDL::NOZERO);
        for (DLong i=0; i<npts; ++i) (*sc_xVal)[i]=(*xVal)[i]*DToR;
        xguard.reset(sc_xVal);
        sc_yVal=new DDoubleGDL(npts,BaseGDL::NOZERO);
        for (DLong i=0; i<npts; ++i) (*sc_yVal)[i]=(*yVal)[i]*DToR;
        yguard.reset(sc_yVal);
      }
      DDoubleGDL* x=new DDoubleGDL(npts,BaseGDL::NOZERO);
      DDoubleGDL* y=new DDoubleGDL(npts,BaseGDL::NOZERO);
      DDoubleGDL* z=new DDoubleGDL(npts,BaseGDL::NOZERO);
      //Stripack is lat, lon and IDL lon,lat
      DLong ret0=stripack::trans_(&npts, (DDouble*)sc_yVal->DataAddr(), (DDouble*)sc_xVal->DataAddr(),(DDouble*)x->DataAddr(),(DDouble*)y->DataAddr(),(DDouble*)z->DataAddr()); 
      DLong listsize=6*npts-12;
      //we use GDL objects as they will be visible outside at GDL level.
      DLongGDL* list=new DLongGDL(listsize,BaseGDL::NOZERO);
      DLongGDL* lptr=new DLongGDL(listsize,BaseGDL::NOZERO);
      DLongGDL* lend=new DLongGDL(npts,BaseGDL::NOZERO);
      DLong* near__=(DLong*)malloc(npts*sizeof(DLong)); //initial name "near" would not work as this is reserved in Windows C. 
      DLong* next=(DLong*)malloc(npts*sizeof(DLong));
      DDouble* dist=(DDouble*)malloc(npts*sizeof(DDouble));
      DLong ier=0;
      DLong lnew=0;
      DLong ret1=stripack::sph_trmesh_(&npts,(DDouble*)x->DataAddr(),(DDouble*)y->DataAddr(),(DDouble*)z->DataAddr(), (DLong*)list->DataAddr(), (DLong*)lptr->DataAddr(), (DLong*)lend->DataAddr(), &lnew, near__, next, dist, &ier);
      free(dist);
      free(next);
      free(near__);
      if (ier !=0) {
        GDLDelete(list);
        GDLDelete(lptr);
        GDLDelete(lend);
        GDLDelete(x);
        GDLDelete(y);
        GDLDelete(z);
        if (ier == -2) e->Throw("The first 3 nodes lie on the same great circle.");
        else e->Throw("Duplicate nodes encountered.");
      }
      //Structure returned in SPHERE is not the same as IDL (although it must be possible to write
      //something identical, but for what use? The structure will be passed to the TRIGRID function
      //that uses SRFPACK, which in turn uses this structure, containing x,y,z,list,lptr,lend
      // creating the output anonymous structure
      DStructDesc* sphere_desc = new DStructDesc("$truct");
      SpDDouble aDbleArr(dimension((SizeT)npts));
      SpDLong aLongArr1(dimension((SizeT)listsize));
      SpDLong aLongArr2(dimension((SizeT)npts));
      sphere_desc->AddTag("X", &aDbleArr);
      sphere_desc->AddTag("Y", &aDbleArr);
      sphere_desc->AddTag("Z", &aDbleArr);
      sphere_desc->AddTag("LIST", &aLongArr1);
      sphere_desc->AddTag("LPTR", &aLongArr1);
      sphere_desc->AddTag("LEND", &aLongArr2);
      DStructGDL* sphere = new DStructGDL(sphere_desc, dimension());

      sphere->InitTag("X", *x); 
      sphere->InitTag("Y", *y); 
      sphere->InitTag("Z", *z); 
      sphere->InitTag("LIST", *list); 
      sphere->InitTag("LPTR", *lptr); 
      sphere->InitTag("LEND", *lend); 
      e->SetKW(sphereIx,sphere);
    
      if (wantsTriangles) {
        //convert to triangle list
        DLong nrow = 6; //no arcs.
        DLong* ltri = (DLong*) malloc((12 * npts) * sizeof (DLong));
        DLong ntriangles = 0;
        DLong ret2=stripack::sph_trlist_(&npts, (DLong*)list->DataAddr(), (DLong*)lptr->DataAddr(), (DLong*)lend->DataAddr(), &nrow, &ntriangles, ltri, &ier);
        if (ier != 0)
        {
          free(ltri);
          e->Throw("Unexpected Error in STRIPACK, SPH_TRLIST routine. Please report.");
        }
        SizeT d[2];
        d[1] = ntriangles;
        d[0] = 3;
        DLongGDL* returned_triangles = new DLongGDL(dimension(d, 2), BaseGDL::NOZERO);
        for (SizeT j = 0; j < ntriangles; ++j)
        {
          for (int i = 0; i < 3; ++i)
          {
            (*returned_triangles)[3 * j + i] = (ltri[6 * j + i] - 1); //nubering starts a 0.
          }
        }
        free(ltri);
        //pass back to GDL env:
        e->SetPar(2, returned_triangles);
      }      
      if (wantsEdge) {
        DLong* nodes=(DLong*)malloc(npts*sizeof(DLong));
        DLong nb=0;
        DLong na=0;
        DLong nt=0;
        DLong ret3=stripack::sph_bnodes_(&npts, (DLong*)list->DataAddr(), (DLong*)lptr->DataAddr(), (DLong*)lend->DataAddr(), nodes, &nb, &na, &nt);
        if (nb>0){
          DLongGDL* returned_edges = new DLongGDL(nb, BaseGDL::NOZERO);
          for (SizeT j = 0; j < nb; ++j) (*returned_edges)[j]=nodes[j]-1;
          e->SetPar(3, returned_edges);
        } else e->SetPar(3, new DLongGDL(-1));
        free(nodes);
      }
      if (hasFvalue) {
        //No need to reorder whatever.
//        //create a dummy array of same size
//        DDoubleGDL* ret=new DDoubleGDL(npts,BaseGDL::NOZERO);
//        //do not forget remove 1 to get C array indexes
//        DLong index_lend, index_list; //can be negative.
//        (*ret)[0]=(*fvalue)[(*list)[0]-1];
//        for (SizeT i=0; i<npts-1; ++i) {
//          index_lend=(*lend)[i]; //as it starts at 1, is the next point, not the end of current point neighbours.
//          index_list=(*list)[index_lend]; //can be negative
//          if (index_list < 0) index_list=-index_list-1; else index_list--; //remove negative values and set as C index.
//          (*ret)[i+1]=(*fvalue)[index_list]; //next point
//        }
//        e->SetKW(fvalueIx,ret);
      }
       //no connectivity in SPHERE mode in IDL, but yes we can!
      if (wantsConnectivity) {
        //remove 1 to get C array indexes.
        for (SizeT i = 0; i < lnew-1; ++i) (*lptr)[i]--; 
        for (SizeT i = 0; i < npts; ++i) (*lend)[i]--;
        DLong* array=(DLong*)malloc((2*(lnew-1))*sizeof(DLong)); // size > max possible connectivity 
        SizeT runningindex=npts+1;
        SizeT startindex=0;
        array[startindex++]=npts+1;
        for (SizeT i = 0; i < npts; ++i) {
          //is it an exterior point? Yes if the termination of the connectivity list is exterior.
          DLong lpl=(*lend)[i];
          if ((*list)[lpl]<0) array[runningindex++]=i; //exterior - we write it
          //write all points until nfin=lend[i] is found again using lptr connectivity pointers:
          DLong lp=lpl; //see nbcnt_()
          do {
            lp=(*lptr)[lp];
            array[runningindex++]=((*list)[lp]>0)?(*list)[lp]-1:(-(*list)[lp])-1;
          } while (lp!=lpl);
          array[startindex++]=runningindex;
        }
        DLongGDL* connections = new DLongGDL(runningindex, BaseGDL::NOZERO);
        for (SizeT i = 0; i < runningindex; ++i) (*connections)[i]=array[i];
        e->SetKW(connIx,connections);
        free(array);
      }
      //no more cleanup, x,y,z,and list,lptr,lend are in the returned structure!
    } else {
      
//      // for PLANE triangulation, everything must be scaled in order to have triangulation independent of range,
//      // as the triangulation code IS sensitive to the values of X and Y.
//      if (maxVal > 0) {
//        for (DLong i = 0; i < npts; ++i) xx[i] /= maxVal;
//        for (DLong i = 0; i < npts; ++i) yy[i] /= maxVal;
//      }

      // for PLANE triangulation, it is possible that everything must be scaled in order to have triangulation independent of range.
      // unfortunately this may have dire consequences. At the meoment I pass unscaled X and Y
      std::vector<double> coords;
//      for (DLong i = 0; i < npts; ++i) {coords.push_back(xx[i]/maxVal);coords.push_back(yy[i]/maxVal);}
      for (DLong i = 0; i < npts; ++i) {coords.push_back(xx[i]);coords.push_back(yy[i]);}
      delaunator::Delaunator tri(coords);
      
      DLong ndupes=tri.dupes.size();
      if (wantsDupes) {
        if (ndupes ==0) {
          DLongGDL* nothing=new DLongGDL(dimension(2),BaseGDL::ZERO); 
          nothing->Dec();
          e->SetKW(dupesIx, nothing);
        }
        else {
          DLongGDL* returned_dupes = new DLongGDL(dimension(2,ndupes), BaseGDL::NOZERO);
          for (DLong i = 0; i < ndupes; ++i) {
            (*returned_dupes)[2*i+0]=tri.dupes[i].first;
            (*returned_dupes)[2*i+1]=tri.dupes[i].second;
          } 
          e->SetKW(dupesIx, returned_dupes);
        }
      }
      
       if (wantsTriangles) {
         DLong ntriangles=tri.triangles.size()/3;
         //convert to triangle list
        SizeT d[2];
        d[1] = ntriangles;
        d[0] = 3;
        DLongGDL* returned_triangles = new DLongGDL(dimension(d, 2), BaseGDL::NOZERO);        
        for (DLong j = 0; j < ntriangles; ++j)
        {
          (*returned_triangles)[3 * j + 0] = tri.triangles[3*j+0]; 
          (*returned_triangles)[3 * j + 2] = tri.triangles[3*j+1]; 
          (*returned_triangles)[3 * j + 1] = tri.triangles[3*j+2]; //write it counterclockwise as IDL does.
        }
        e->SetPar(2, returned_triangles);
       }
      // get Hull values needed at two places.
      std::vector<size_t> hull;
      if (wantsEdge || wantsConnectivity) {
        size_t index = tri.hull_start;
        hull.push_back(index);
        index = tri.hull_next[index];
        while (index != tri.hull_start) {
          hull.push_back(index);
          index = tri.hull_next[index];
        };
      }
      if (wantsEdge) {
        DLong nb=hull.size();
        DLongGDL* returned_edges = new DLongGDL(nb, BaseGDL::NOZERO);
        for (DLong j = 0; j < nb; ++j) (*returned_edges)[j]=hull[nb-1-j]; //write it counterclockwise as IDL does.
        e->SetPar(3, returned_edges);
      }
      if (wantsConnectivity) {
        const std::size_t INVALID_INDEX = (std::numeric_limits<std::size_t>::max)();
       // as described in https://mapbox.github.io/delaunator/ it is not straightforward as there is no direct link points<->half-edges.
       // let's build a map with a half-edge for each point, which will be a starting half-edge for connectivity.
       //*** NOTE: CONNECTIVITY is oriented as in IDL 'naturally' ***
        std::map<size_t,size_t>accel; //key is point, value is the out half-edge.
        for (size_t e=0; e<tri.triangles.size();++e) {
           size_t endpoint = tri.triangles[nextHalfedge(e)];
           if (accel.find(endpoint)==accel.end() || tri.halfedges[e]==INVALID_INDEX ) accel.insert(std::pair<size_t,size_t>(endpoint,e));
        }
        // Now, points on hull will need a special treatment, as we must start the connectivity exploration at the previous hull position
        // otherwise the connectivity search will end prematurely on a INVALID_INDEX
        // We have computed the hull point list: for each hull point, find the halfedge that goes from the previous hull point to it.
        //This is the halfedge to start with in accel:
        for (int i=0; i< hull.size(); ++i) {
          size_t point=hull[i]; //this is an existing point by construction, not a duplicate in dupes.
          size_t nextpt=(i==0)?hull[hull.size()-1]:hull[i-1]; //idem.
          std::map<size_t, size_t>::iterator iter = accel.find(nextpt);
          size_t start = (*iter).second; //one of the halfedges starting with current point
          size_t incoming=start;
          //get adjacent halfedges until point is encountered.
          size_t outgoing;
          do {
            outgoing = nextHalfedge(incoming); //the ougoing halfegde -> the point k in fact:  assert(tri.triangles[outgoing]==effective_index[k]);
            incoming = tri.halfedges[outgoing]; //the following incoming half-edge
          } while (incoming!=INVALID_INDEX); //stop when halfedge going to 'point' is found. 
          iter = accel.find(point);
          accel.erase(iter);
          accel.insert(std::pair<size_t,size_t>(point,outgoing));
        }
        
        //in connectivity we MUST have all the points, even the duplicated ones. Connectivity of duplicated points is wrong with IDL (intentional?).
        // we could avoid this easily as I believe this is an IDL bug. We just have to reproduce the connectivity of the first encounter
        // of the duplicated point.
        DLong* array=(DLong*)malloc((npts*npts+npts+1)*sizeof(DLong)); // size > max possible connectivity 
        DLong runningindex = npts+1; // we report for all points 
        DLong startindex=0;
        array[startindex++]= npts+1;
        DLong* effective_index=(DLong*)malloc(npts*sizeof(DLong));//this is the list of npts vertexes for which we want the neighbour. It must be
        // the  list of l_npts (returned) indexes, with indexes of the first instance of duplicated points for the duplicated points.
        for (DLong i=0; i< npts; ++i) effective_index[i]=i;
        if (ndupes != 0) { 
          for (DLong idup=0; idup< ndupes; ++idup) {
            DLong encounter=tri.dupes[idup].second;
            effective_index[encounter]=tri.dupes[idup].first;
          }
        }
        
        for (DLong k = 0; k < npts; ++k) { 
          //find the triangle in accel:
          size_t point=effective_index[k];

          //if point is in the hull, make provision to add point itself at start and next point at end.
          size_t nextpt;
          bool isHull=false;
          for (int i = 0; i < hull.size(); ++i) {
            if (hull[i] == point) {
              isHull=true;
              nextpt = ((i + 1) >= hull.size()) ? hull[0] : hull[i + 1];
              array[runningindex++] = point;
              break;
            }
          }
          std::map<size_t, size_t>::iterator iter = accel.find(point);
          size_t start = (*iter).second; //one of the halfedges starting with current point
          size_t incoming=start;
          //get adjacent halfedges and write their end points to connectivity until all encountered.
          do {
            /*THE FOLLOWING LINE IS KNOWN TO HAVE CRASHED ON UNREPRODUCTIBLE CIRCUMSTANCES ??? */
            array[runningindex++]=tri.triangles[incoming]; //the point outgoing halfedge ends. 
            size_t outgoing = nextHalfedge(incoming); //the ougoing halfegde -> the point k in fact:  assert(tri.triangles[outgoing]==effective_index[k]);
            incoming = tri.halfedges[outgoing]; //the following incoming half-edge
          } while (incoming!=INVALID_INDEX && incoming != start); //stop when start halfedge is found.
          if (isHull) array[runningindex++] = nextpt;
          array[startindex++]=runningindex;
        }
        DLongGDL* connections = new DLongGDL(runningindex, BaseGDL::NOZERO);
        for (DLong i = 0; i < runningindex; ++i) (*connections)[i]=array[i];
        e->SetKW(connIx,connections);
        free(array);
      }
    }
  }

  
  BaseGDL* trigrid_fun_spherical(EnvT* e) {
    //NOT USED in this case: EXTRAPOLATE, MAX_VALUE, MIN_VALUE, QUINTIC, XOUT, YOUT
    static DDouble DToR=double(3.1415926535897932384626433832795)/180.0;
    SizeT nParam = e->NParam();
    if (nParam < 3) e->Throw("Incorrect number of arguments.");
    //OK, trigrid does not care if more than 3 args in sphere mode. Limit at 6 is done at interpreter level.

    // Get NX, NY values if present
    DLong nx = 51;
    DLong ny = 51;

    static int degreeIx=e->KeywordIx( "DEGREES");
    bool doDegree=(e->KeywordPresent(degreeIx));

    // in difference with IDL we accept xout and yout for spherical data.
    static int xoutIx=e->KeywordIx( "XOUT");
    bool doXout=(e->KeywordSet(xoutIx)); //and not "present"
    static int youtIx=e->KeywordIx( "YOUT");
    bool doYout=(e->KeywordSet(youtIx));
    if ((doXout && !doYout) || (doYout && !doXout)) e->Throw("Incorrect number of arguments.");
    static int xgridIx=e->KeywordIx( "XGRID");
    bool doXgrid=(e->KeywordPresent(xgridIx));
    static int ygridIx=e->KeywordIx( "YGRID");
    bool doYgrid=(e->KeywordPresent(ygridIx));

    static int nxIx = e->KeywordIx("NX");
    static int nyIx = e->KeywordIx("NY");
    bool canUseLimitsx=true;
    bool canUseLimitsy=true;
    if (e->KeywordSet(nxIx)) { canUseLimitsx=false; e->AssureLongScalarKW(nxIx, nx);}
    if (e->KeywordSet(nyIx)) { canUseLimitsy=false; e->AssureLongScalarKW(nyIx, ny);}
    if (nx < 1 || ny < 1) e->Throw("Array dimensions must be greater than 0.");
    // we will further on define GS, whose value may overwrite the nx, ny.
    
    BaseGDL* p0 = e->GetParDefined(0); //F
    if (p0->Rank() == 0) e->Throw("Expression must be an array in this context: " + e->GetParString(0));
    DDoubleGDL* fval=static_cast<DDoubleGDL*>(p0->Convert2(GDL_DOUBLE, BaseGDL::COPY));
    BaseGDL* p1 = e->GetParDefined(1);
    if (p1->Rank() == 0) e->Throw("Expression must be an array in this context: " + e->GetParString(1));
    if (p1->N_Elements() != 2) e->Throw("Array must have 2 elements: " + e->GetParString(1));
    DDoubleGDL* GS = static_cast<DDoubleGDL*>(p1->Convert2(GDL_DOUBLE, BaseGDL::COPY));

    BaseGDL* p2 = e->GetParDefined(2);
    if (p2->Rank() == 0) e->Throw("Expression must be an array in this context: " + e->GetParString(2));
    if (p2->N_Elements() != 4) e->Throw("Array must have 4 elements: " + e->GetParString(2));
    DDoubleGDL* limits = static_cast<DDoubleGDL*> (p2->Convert2(GDL_DOUBLE, BaseGDL::COPY));      

    //Sphere is present since we are called. Test it.
    DStructGDL* sphere=NULL;
    static int sphereIx = e->KeywordIx( "SPHERE");
    BaseGDL* test=e->GetKW(sphereIx);
    
    int xTag,yTag,zTag,listTag,lptrTag,lendTag;
    
    if (test->Type() == GDL_STRUCT)
    {
      sphere = static_cast<DStructGDL*> (test);

      xTag = sphere->Desc()->TagIndex("X");
      yTag = sphere->Desc()->TagIndex("Y");
      zTag = sphere->Desc()->TagIndex("Z");
      listTag = sphere->Desc()->TagIndex("LIST");
      lptrTag = sphere->Desc()->TagIndex("LPTR");
      lendTag = sphere->Desc()->TagIndex("LEND");
      if (xTag < 0 || yTag < 0 || zTag < 0 || listTag < 0 ||lptrTag < 0 || lendTag < 0) e->Throw("Invalid structure for the SPHERE keyword.");
    } else {
      e->Throw("SPHERE keyword must be a structure.");
    }

    DDoubleGDL* xVal = static_cast<DDoubleGDL*>(sphere->GetTag(xTag,0));
    DDoubleGDL* yVal = static_cast<DDoubleGDL*>(sphere->GetTag(yTag,0));
    DDoubleGDL* zVal = static_cast<DDoubleGDL*>(sphere->GetTag(zTag,0));
    if (xVal->N_Elements() != yVal->N_Elements())e->Throw("Invalid structure for the SPHERE keyword.");
    if (xVal->N_Elements() != zVal->N_Elements())e->Throw("Invalid structure for the SPHERE keyword.");
    DLongGDL* list = static_cast<DLongGDL*>(sphere->GetTag(listTag,0));
    DLongGDL* lptr = static_cast<DLongGDL*>(sphere->GetTag(lptrTag,0));
    DLongGDL* lend = static_cast<DLongGDL*>(sphere->GetTag(lendTag,0));
    
    //get npts
    DLong npts=fval->N_Elements();
    if (xVal->N_Elements()<npts) npts=xVal->N_Elements();
    
    // Determine grid range
    DDouble xref=0.0, yref=0.0;
    DDouble xval, xinc;
    DDouble yval, yinc;
    DDouble xrange; 
    DDouble yrange;
    //compute World positions of each pixels.
    if ((*limits)[0]==(*limits)[2] || (*limits)[0]==(*limits)[2]) e->Throw("Inconsistent coordinate bounds.");
    xval = (*limits)[0];
    if (doDegree) {
      xval*=DToR;
      xrange = (*limits)[2]*DToR - xval;
    } else xrange = (*limits)[2] - xval;
    yval = (*limits)[1];
    if (doDegree) {
      yval*=DToR;
      yrange = (*limits)[3]*DToR - yval;
    } else yrange = (*limits)[3] - yval;
    // Determine grid spacing
    xinc = xrange / (nx-1);
    yinc = yrange / (ny-1);
    if (canUseLimitsx && (*GS)[0] > 0.0)
    {
      xinc = (*GS)[0];
      if (doDegree) xinc*=DToR;
      nx = (xrange / xinc) +1;
    }
    if (canUseLimitsy && (*GS)[1] > 0.0)
    {
      yinc = (*GS)[1];
      if (doDegree) yinc*=DToR;
      ny = (yrange / yinc) +1;
    }
    DDouble *plon, *plat;
    bool cleanplon=false;
    bool cleanplat=false;

    if (doXout) {
      DDoubleGDL* xOut=e->GetKWAs<DDoubleGDL>(xoutIx);
      nx=xOut->N_Elements();
      if (nx < 1) e->Throw("XOUT must have >1 elements.");
      plon=(DDouble*)xOut->DataAddr();
      if (doDegree) for (int i=0; i<nx; ++i) plon[i]*=DToR;
    } else {
      plon = (DDouble*)malloc(nx*sizeof(DDouble));
      for (int i=0; i<nx; ++i) plon[i]=(i-xref)*xinc+xval;
      cleanplon=true;
    }

    if (doYout) {
      DDoubleGDL* yOut=e->GetKWAs<DDoubleGDL>(youtIx);
      ny=yOut->N_Elements();
      if (ny < 1) e->Throw("YOUT must have >1 elements.");
      plat=(DDouble*)yOut->DataAddr();
      if (doDegree) for (int j=0; j<ny; ++j) plat[j]*=DToR;
    } else {
      plat = (DDouble*)malloc(ny*sizeof(DDouble));
      for (int j=0; j<ny; ++j) plat[j]=(j-yref)*yinc+yval;
      cleanplat=true;
    }

    // Setup return array
    DLong dims[2];
    dims[0] = ny;
    dims[1] = nx;
    dimension dim((DLong *) dims, 2);
    DDoubleGDL* res = new DDoubleGDL(dim, BaseGDL::ZERO);
    DLong ier=0;

    //must be done here, unif_() below messes with plat and plon...
    if (doXgrid) {
      DDoubleGDL *xgrid=new DDoubleGDL(nx, BaseGDL::NOZERO);
      for (int i=0; i<nx; ++i) (*xgrid)[i]=plon[i];
      if (doDegree) for (int i=0; i<nx; ++i) (*xgrid)[i]/=DToR;
      e->SetKW(xgridIx, xgrid);
    }
    if (doYgrid) {
      DDoubleGDL *ygrid=new DDoubleGDL(ny, BaseGDL::NOZERO);
      for (int i=0; i<ny; ++i) (*ygrid)[i]=plat[i];
      if (doDegree) for (int i=0; i<ny; ++i) (*ygrid)[i]/=DToR;
      e->SetKW(ygridIx, ygrid);
    }
    
    DLong iflgs=0;
    DLong iflgg=0;
    DDouble grad=0.0;
    DDouble sigma=1.;
    DLong ret2=ssrfpack::unif_ (&npts,(DDouble*)xVal->DataAddr(), (DDouble*)yVal->DataAddr(),
              (DDouble*)zVal->DataAddr(), (DDouble*)fval->DataAddr(),(DLong*)list->DataAddr(), (DLong*)lptr->DataAddr(),
              (DLong*)lend->DataAddr(), &iflgs, &sigma, &ny, &ny, &nx, plat, plon, &iflgg, 
	&grad, (DDouble*)res->DataAddr() , &ier);
    if (cleanplon) free(plon);
    if (cleanplat) free(plat);
    if (ret2 != 0) {
      if (ret2==-1) e->Throw("Error in SSRFPACK unif_() function: \"N, NI, NJ, or IFLGG is outside its valid range\".");
      if (ret2==-2) e->Throw("Error in SSRFPACK unif_() function: Nodes are colinear");
      if (ret2==-3) e->Throw("Error in SSRFPACK unif_() function: extrapolation failed due to the uniform grid extending too far beyond the triangulation boundary.");
    }

    return res->Transpose(NULL);
  }

  template< typename T1, typename T2>
  void gdlGrid2DData(DLong nx, DDouble* x, DLong xref, DDouble xval, DDouble xinc, DLong ny, DDouble* y, DLong yref, DDouble yval, DDouble yinc, DLong ntri, DLongGDL* tri,
    DDoubleGDL* xVal, DDoubleGDL* yVal, T1* zVal, bool domaxvalue,
    bool dominvalue, T2 maxVal, T2 minVal, T1* res, bool input) {

    if (dominvalue || domaxvalue) {
      for (SizeT triIndex = 0; triIndex < ntri; ++triIndex) {
        // first, find whose region of grid is concerned by this triangle: boundingBox.
        DDouble xmin;
        DDouble xmax;
        DDouble ymin;
        DDouble ymax;
        DDouble x1, y1, x2, y2, x3, y3;
        x1 = (*xVal)[(*tri)[3 * triIndex]];
        y1 = (*yVal)[(*tri)[3 * triIndex]];
        x2 = (*xVal)[(*tri)[3 * triIndex + 1]];
        y2 = (*yVal)[(*tri)[3 * triIndex + 1]];
        x3 = (*xVal)[(*tri)[3 * triIndex + 2]];
        y3 = (*yVal)[(*tri)[3 * triIndex + 2]];
        xmin = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
        ymin = y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
        xmax = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
        ymax = y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

        // x=(i-xref)*xinc+xval; -> i=(x-xval)/xinc+xref
        int pxmin = ((xmin - xval) / xinc) + xref;
        pxmin = (pxmin < 0) ? 0 : pxmin;
        int pxmax = ((xmax - xval) / xinc) + xref + 1;
        pxmax = (pxmax > nx) ? nx : pxmax;
        pxmax = (pxmax < pxmin) ? pxmin : pxmax;
        int pymin = ((ymin - yval) / yinc) + yref;
        pymin = (pymin < 0) ? 0 : pymin;
        int pymax = ((ymax - yval) / yinc) + yref + 1;
        pymax = (pymax > ny) ? ny : pymax;
        pymax = (pymax < pymin) ? pymin : pymax;

        DDouble zc = (*zVal)[(*tri)[3 * triIndex + 2]];
        DDouble zac = (*zVal)[(*tri)[3 * triIndex]] - zc;
        DDouble zbc = (*zVal)[(*tri)[3 * triIndex + 1]] - zc;
        DDouble y23 = (y2 - y3);
        DDouble x32 = (x3 - x2);
        DDouble y31 = (y3 - y1);
        DDouble x13 = (x1 - x3);
        DDouble det = (y23 * x13 - x32 * y31);
        DDouble minD = (det<0)?det:0 ; // we MUST be agnostic as to the orientation of the triangle, even if TRIANGULATE is clockwise.
        DDouble maxD = (det>0)?det:0;
        for (SizeT iy = pymin; iy < pymax; ++iy) {
          DDouble dy = y[iy] - y3;
          DLong start = iy*nx;
          for (SizeT ix = pxmin; ix < pxmax; ++ix) {
            DDouble dx = x[ix] - x3;
            DDouble a = y23 * dx + x32 * dy;
            if (a < minD || a > maxD) continue;
            DDouble b = y31 * dx + x13 * dy;
            if (b < minD || b > maxD) continue;
            DDouble c = det - a - b;
            if (c < minD || c > maxD) continue;
            DDouble valbis = (a * zac + b * zbc) / det + zc;
            if ((dominvalue && valbis < minVal) || (domaxvalue && valbis > maxVal)) {
            } else (*res)[start + ix] = valbis;            
          } //ix loop
        } // iy
      } //tri
    } else {
        for (SizeT triIndex = 0; triIndex < ntri; ++triIndex) {
        // first, find whose region of grid is concerned by this triangle: boundingBox.
        DDouble xmin;
        DDouble xmax;
        DDouble ymin;
        DDouble ymax;
        DDouble x1, y1, x2, y2, x3, y3;
        x1 = (*xVal)[(*tri)[3 * triIndex]];
        y1 = (*yVal)[(*tri)[3 * triIndex]];
        x2 = (*xVal)[(*tri)[3 * triIndex + 1]];
        y2 = (*yVal)[(*tri)[3 * triIndex + 1]];
        x3 = (*xVal)[(*tri)[3 * triIndex + 2]];
        y3 = (*yVal)[(*tri)[3 * triIndex + 2]];
        xmin = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
        ymin = y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
        xmax = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
        ymax = y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

        // x=(i-xref)*xinc+xval; -> i=(x-xval)/xinc+xref
        int pxmin = ((xmin - xval) / xinc) + xref;
        pxmin = (pxmin < 0) ? 0 : pxmin;
        int pxmax = ((xmax - xval) / xinc) + xref + 1 ;
        pxmax = (pxmax > nx ) ? nx : pxmax;
        pxmax = (pxmax < pxmin) ? pxmin : pxmax;
        int pymin = ((ymin - yval) / yinc) + yref;
        pymin = (pymin < 0) ? 0 : pymin;
        int pymax = ((ymax - yval) / yinc) + yref + 1;
        pymax = (pymax > ny) ? ny : pymax;
        pymax = (pymax < pymin) ? pymin : pymax;

        DDouble zc  = (*zVal)[(*tri)[3 * triIndex + 2]];
        DDouble zac = (*zVal)[(*tri)[3 * triIndex]] - zc;
        DDouble zbc = (*zVal)[(*tri)[3 * triIndex + 1]] - zc;
        DDouble y23 = (y2 - y3);
        DDouble x32 = (x3 - x2);
        DDouble y31 = (y3 - y1);
        DDouble x13 = (x1 - x3);
        DDouble det = (y23 * x13 - x32 * y31);
        DDouble minD = (det<0)?det:0 ; // we MUST be agnostic as to the orientation of the triangle, even if TRIANGULATE is clockwise.
        DDouble maxD = (det>0)?det:0;
        for (SizeT iy = pymin; iy < pymax; ++iy) {
          DDouble dy = y[iy]-y3;
          DLong start = iy*nx;
          for (SizeT ix = pxmin; ix < pxmax; ++ix) {
            DDouble dx = x[ix]-x3; 
            DDouble a = y23 * dx + x32 * dy;
            if (a < minD || a > maxD) continue;
            DDouble b = y31 * dx + x13 * dy;
            if (b < minD || b > maxD) continue;
            DDouble c = det - a - b;
            if (c < minD || c > maxD) continue;
            (*res)[start + ix] = (a * zac + b * zbc)/det + zc;
          } //ix loop
        } // iy
      } //tri
    }
  }

  void gdlGrid2DDataCpx(DLong nx, DDouble* x, DLong xref, DDouble xval, DDouble xinc, DLong ny, DDouble* y, DLong yref, DDouble yval, DDouble yinc, DLong ntri, DLongGDL* tri,
    DDoubleGDL* xVal, DDoubleGDL* yVal, DComplexDblGDL* zVal, bool domaxvalue,
    bool dominvalue, DComplexDbl maxVal, DComplexDbl minVal, DComplexDblGDL* res, bool input) {

    if (dominvalue || domaxvalue) {
      for (SizeT triIndex = 0; triIndex < ntri; ++triIndex) {
        // first, find whose region of grid is concerned by this triangle: boundingBox.
        DDouble xmin;
        DDouble xmax;
        DDouble ymin;
        DDouble ymax;
        DDouble x1, y1, x2, y2, x3, y3;
        x1 = (*xVal)[(*tri)[3 * triIndex]];
        y1 = (*yVal)[(*tri)[3 * triIndex]];
        x2 = (*xVal)[(*tri)[3 * triIndex + 1]];
        y2 = (*yVal)[(*tri)[3 * triIndex + 1]];
        x3 = (*xVal)[(*tri)[3 * triIndex + 2]];
        y3 = (*yVal)[(*tri)[3 * triIndex + 2]];
        xmin = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
        ymin = y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
        xmax = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
        ymax = y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

        // x=(i-xref)*xinc+xval; -> i=(x-xval)/xinc+xref
        int pxmin = ((xmin - xval) / xinc) + xref;
        pxmin = (pxmin < 0) ? 0 : pxmin;
        int pxmax = ((xmax - xval) / xinc) + xref + 1;
        pxmax = (pxmax > nx) ? nx : pxmax;
        pxmax = (pxmax < pxmin) ? pxmin : pxmax;
        int pymin = ((ymin - yval) / yinc) + yref;
        pymin = (pymin < 0) ? 0 : pymin;
        int pymax = ((ymax - yval) / yinc) + yref + 1;
        pymax = (pymax > ny) ? ny : pymax;
        pymax = (pymax < pymin) ? pymin : pymax;

        DComplexDbl zc = (*zVal)[(*tri)[3 * triIndex + 2]];
        DComplexDbl zac = std::complex<double>((*zVal)[(*tri)[3 * triIndex]].real() - zc.real(), (*zVal)[(*tri)[3 * triIndex]].imag() - zc.imag());
        DComplexDbl zbc = std::complex<double>((*zVal)[(*tri)[3 * triIndex + 1]].real() - zc.real(), (*zVal)[(*tri)[3 * triIndex + 1]].imag() - zc.imag());
        DDouble y23 = (y2 - y3);
        DDouble x32 = (x3 - x2);
        DDouble y31 = (y3 - y1);
        DDouble x13 = (x1 - x3);
        DDouble det = (y23 * x13 - x32 * y31);
        DDouble minD = (det<0)?det:0 ; // we MUST be agnostic as to the orientation of the triangle, even if TRIANGULATE is clockwise.
        DDouble maxD = (det>0)?det:0;
        for (SizeT iy = pymin; iy < pymax; ++iy) {
          DDouble dy = y[iy] - y3;
          DLong start = iy*nx;
          for (SizeT ix = pxmin; ix < pxmax; ++ix) {
            DDouble dx = x[ix] - x3;
            DDouble a = y23 * dx + x32 * dy;
            if (a < minD || a > maxD) continue;
            DDouble b = y31 * dx + x13 * dy;
            if (b < minD || b > maxD) continue;
            DDouble c = det - a - b;
            if (c < minD || c > maxD) continue;
            DDouble valbisR = (a * zac.real() + b * zbc.real())/det + zc.real();
            DDouble valbisI = (a * zac.imag() + b * zbc.imag())/det + zc.imag();
            if ((dominvalue && valbisR < minVal.real()) || (domaxvalue && valbisR > maxVal.real())) { valbisR = (*res)[start + ix].real(); }
            if ((dominvalue && valbisI < minVal.imag()) || (domaxvalue && valbisI > maxVal.imag())) { valbisI = (*res)[start + ix].imag(); }
            (*res)[start + ix] = std::complex<double>(valbisR,valbisI);
          } //ix loop
        } // iy
      } //tri
    } else {
      for (SizeT triIndex = 0; triIndex < ntri; ++triIndex) {
        // first, find whose region of grid is concerned by this triangle: boundingBox.
        DDouble xmin;
        DDouble xmax;
        DDouble ymin;
        DDouble ymax;
        DDouble x1, y1, x2, y2, x3, y3;
        x1 = (*xVal)[(*tri)[3 * triIndex]];
        y1 = (*yVal)[(*tri)[3 * triIndex]];
        x2 = (*xVal)[(*tri)[3 * triIndex + 1]];
        y2 = (*yVal)[(*tri)[3 * triIndex + 1]];
        x3 = (*xVal)[(*tri)[3 * triIndex + 2]];
        y3 = (*yVal)[(*tri)[3 * triIndex + 2]];
        xmin = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
        ymin = y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
        xmax = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
        ymax = y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

        // x=(i-xref)*xinc+xval; -> i=(x-xval)/xinc+xref
        int pxmin = ((xmin - xval) / xinc) + xref;
        pxmin = (pxmin < 0) ? 0 : pxmin;
        int pxmax = ((xmax - xval) / xinc) + xref + 1;
        pxmax = (pxmax > nx) ? nx : pxmax;
        pxmax = (pxmax < pxmin) ? pxmin : pxmax;
        int pymin = ((ymin - yval) / yinc) + yref;
        pymin = (pymin < 0) ? 0 : pymin;
        int pymax = ((ymax - yval) / yinc) + yref + 1;
        pymax = (pymax > ny) ? ny : pymax;
        pymax = (pymax < pymin) ? pymin : pymax;

        DComplexDbl zc = (*zVal)[(*tri)[3 * triIndex + 2]];
        DComplexDbl zac = std::complex<double>((*zVal)[(*tri)[3 * triIndex]].real() - zc.real(), (*zVal)[(*tri)[3 * triIndex]].imag() - zc.imag());
        DComplexDbl zbc = std::complex<double>((*zVal)[(*tri)[3 * triIndex + 1]].real() - zc.real(), (*zVal)[(*tri)[3 * triIndex + 1]].imag() - zc.imag());
        DDouble y23 = (y2 - y3);
        DDouble x32 = (x3 - x2);
        DDouble y31 = (y3 - y1);
        DDouble x13 = (x1 - x3);
        DDouble det = (y23 * x13 - x32 * y31);
        DDouble minD = (det<0)?det:0 ; // we MUST be agnostic as to the orientation of the triangle, even if TRIANGULATE is clockwise.
        DDouble maxD = (det>0)?det:0;
        for (SizeT iy = pymin; iy < pymax; ++iy) {
          DDouble dy = y[iy] - y3;
          DLong start = iy*nx;
          for (SizeT ix = pxmin; ix < pxmax; ++ix) {
            DDouble dx = x[ix] - x3;
            DDouble a = y23 * dx + x32 * dy;
            if (a < minD || a > maxD) continue;
            DDouble b = y31 * dx + x13 * dy;
            if (b < minD || b > maxD) continue;
            DDouble c = det - a - b;
            if (c < minD || c > maxD) continue;
            DDouble valbisR = (a * zac.real() + b * zbc.real())/det + zc.real();
            DDouble valbisI = (a * zac.imag() + b * zbc.imag())/det + zc.imag();
            (*res)[start + ix] = std::complex<double>(valbisR,valbisI);
          } //ix loop
        } // iy
      } //tri
    }
  }
  
  template< typename T1, typename T2>
  void gdlGrid2DData(DLong nx, DDouble* x, DLong ny, DDouble* y, DLong ntri, DLongGDL* tri,
    DDoubleGDL* xVal, DDoubleGDL* yVal, T1* zVal, bool domaxvalue,
    bool dominvalue, T2 maxVal, T2 minVal, T1* res, bool input){

    if (dominvalue || domaxvalue) {
      for (SizeT triIndex = 0; triIndex < ntri; ++triIndex) {
        // first, find whose region of grid is concerned by this triangle: boundingBox.
        DDouble xmin;
        DDouble xmax;
        DDouble ymin;
        DDouble ymax;
        DDouble x1, y1, x2, y2, x3, y3;
        x1 = (*xVal)[(*tri)[3 * triIndex]];
        y1 = (*yVal)[(*tri)[3 * triIndex]];
        x2 = (*xVal)[(*tri)[3 * triIndex + 1]];
        y2 = (*yVal)[(*tri)[3 * triIndex + 1]];
        x3 = (*xVal)[(*tri)[3 * triIndex + 2]];
        y3 = (*yVal)[(*tri)[3 * triIndex + 2]];
        xmin = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
        ymin = y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
        xmax = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
        ymax = y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

        // find first and last index of x and y that will intersect this BBox
        DLong k = 0;
        while (k < (nx - 1) && x[k + 1] < xmin) {
          k++;
        }
        int pxmin = k;
        while (k < (nx - 1) && x[k + 1] < xmax) {
          k++;
        }
        int pxmax = k;
        k = 0;
        while (k < (ny - 1) && y[k + 1] < ymin) {
          k++;
        }
        int pymin = k;
        while (k < (ny - 1) && y[k + 1] < ymax) {
          k++;
        }
        int pymax = k;
        
        DDouble zc = (*zVal)[(*tri)[3 * triIndex + 2]];
        DDouble zac = (*zVal)[(*tri)[3 * triIndex]] - zc;
        DDouble zbc = (*zVal)[(*tri)[3 * triIndex + 1]] - zc;
        DDouble y23 = (y2 - y3);
        DDouble x32 = (x3 - x2);
        DDouble y31 = (y3 - y1);
        DDouble x13 = (x1 - x3);
        DDouble det = (y23 * x13 - x32 * y31);
        DDouble minD = (det<0)?det:0 ; // we MUST be agnostic as to the orientation of the triangle, even if TRIANGULATE is clockwise.
        DDouble maxD = (det>0)?det:0;
        for (SizeT iy = pymin; iy <= pymax; ++iy) {
          DDouble dy = y[iy] - y3;
          DLong start = iy*nx;
          for (SizeT ix = pxmin; ix <= pxmax; ++ix) {
            DDouble dx = x[ix] - x3;
            DDouble a = y23 * dx + x32 * dy;
            if (a < minD || a > maxD) continue;
            DDouble b = y31 * dx + x13 * dy;
            if (b < minD || b > maxD) continue;
            DDouble c = det - a - b;
            if (c < minD || c > maxD) continue;
            DDouble valbis = (a * zac + b * zbc) / det + zc;
            if ((dominvalue && valbis < minVal) || (domaxvalue && valbis > maxVal)) {
            } else (*res)[start + ix] = valbis;            
          } //ix loop
        } // iy
      } //tri
    } else {
        for (SizeT triIndex = 0; triIndex < ntri; ++triIndex) {
        // first, find whose region of grid is concerned by this triangle: boundingBox.
        DDouble xmin;
        DDouble xmax;
        DDouble ymin;
        DDouble ymax;
        DDouble x1, y1, x2, y2, x3, y3;
        x1 = (*xVal)[(*tri)[3 * triIndex]];
        y1 = (*yVal)[(*tri)[3 * triIndex]];
        x2 = (*xVal)[(*tri)[3 * triIndex + 1]];
        y2 = (*yVal)[(*tri)[3 * triIndex + 1]];
        x3 = (*xVal)[(*tri)[3 * triIndex + 2]];
        y3 = (*yVal)[(*tri)[3 * triIndex + 2]];
        xmin = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
        ymin = y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
        xmax = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
        ymax = y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

        // find first and last index of x and y that will intersect this BBox
        DLong k = 0;
        while (k < (nx - 1) && x[k + 1] < xmin) {
          k++;
        }
        int pxmin = k;
        while (k < (nx - 1) && x[k + 1] < xmax) {
          k++;
        }
        int pxmax = k;
        k = 0;
        while (k < (ny - 1) && y[k + 1] < ymin) {
          k++;
        }
        int pymin = k;
        while (k < (ny - 1) && y[k + 1] < ymax) {
          k++;
        }
        int pymax = k;

        DDouble zc  = (*zVal)[(*tri)[3 * triIndex + 2]];
        DDouble zac = (*zVal)[(*tri)[3 * triIndex]] - zc;
        DDouble zbc = (*zVal)[(*tri)[3 * triIndex + 1]] - zc;
        DDouble y23 = (y2 - y3);
        DDouble x32 = (x3 - x2);
        DDouble y31 = (y3 - y1);
        DDouble x13 = (x1 - x3);
        DDouble det = (y23 * x13 - x32 * y31);
        DDouble minD = (det<0)?det:0 ; // we MUST be agnostic as to the orientation of the triangle, even if TRIANGULATE is clockwise.
        DDouble maxD = (det>0)?det:0;
        for (SizeT iy = pymin; iy <= pymax; ++iy) {
          DDouble dy = y[iy]-y3;
          DLong start = iy*nx;
          for (SizeT ix = pxmin; ix <= pxmax; ++ix) {
            DDouble dx = x[ix]-x3; 
            DDouble a = y23 * dx + x32 * dy;
            if (a < minD || a > maxD) continue;
            DDouble b = y31 * dx + x13 * dy;
            if (b < minD || b > maxD) continue;
            DDouble c = det - a - b;
            if (c < minD || c > maxD) continue;
            (*res)[start + ix] = (a * zac + b * zbc)/det + zc;
          } //ix loop
        } // iy
      } //tri
    }
  }  
  
  void gdlGrid2DDataCpx(DLong nx, DDouble* x, DLong ny, DDouble* y, DLong ntri, DLongGDL* tri,
    DDoubleGDL* xVal, DDoubleGDL* yVal, DComplexDblGDL* zVal, bool domaxvalue,
    bool dominvalue, DComplexDbl maxVal, DComplexDbl minVal, DComplexDblGDL* res, bool input) {

    if (dominvalue || domaxvalue) {
      for (SizeT triIndex = 0; triIndex < ntri; ++triIndex) {
        // first, find whose region of grid is concerned by this triangle: boundingBox.
        DDouble xmin;
        DDouble xmax;
        DDouble ymin;
        DDouble ymax;
        DDouble x1, y1, x2, y2, x3, y3;
        x1 = (*xVal)[(*tri)[3 * triIndex]];
        y1 = (*yVal)[(*tri)[3 * triIndex]];
        x2 = (*xVal)[(*tri)[3 * triIndex + 1]];
        y2 = (*yVal)[(*tri)[3 * triIndex + 1]];
        x3 = (*xVal)[(*tri)[3 * triIndex + 2]];
        y3 = (*yVal)[(*tri)[3 * triIndex + 2]];
        xmin = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
        ymin = y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
        xmax = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
        ymax = y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

        // find first and last index of x and y that will intersect this BBox
        DLong k = 0;
        while (k < (nx - 1) && x[k + 1] < xmin) {
          k++;
        }
        int pxmin = k;
        while (k < (nx - 1) && x[k + 1] < xmax) {
          k++;
        }
        int pxmax = k;
        k = 0;
        while (k < (ny - 1) && y[k + 1] < ymin) {
          k++;
        }
        int pymin = k;
        while (k < (ny - 1) && y[k + 1] < ymax) {
          k++;
        }
        int pymax = k;
  
        DComplexDbl zc = (*zVal)[(*tri)[3 * triIndex + 2]];
        DComplexDbl zac = std::complex<double>((*zVal)[(*tri)[3 * triIndex]].real() - zc.real(), (*zVal)[(*tri)[3 * triIndex]].imag() - zc.imag());
        DComplexDbl zbc = std::complex<double>((*zVal)[(*tri)[3 * triIndex + 1]].real() - zc.real(), (*zVal)[(*tri)[3 * triIndex + 1]].imag() - zc.imag());
        DDouble y23 = (y2 - y3);
        DDouble x32 = (x3 - x2);
        DDouble y31 = (y3 - y1);
        DDouble x13 = (x1 - x3);
        DDouble det = (y23 * x13 - x32 * y31);
        DDouble minD = (det<0)?det:0 ; // we MUST be agnostic as to the orientation of the triangle, even if TRIANGULATE is clockwise.
        DDouble maxD = (det>0)?det:0;
        for (SizeT iy = pymin; iy <= pymax; ++iy) {
          DDouble dy = y[iy] - y3;
          DLong start = iy*nx;
          for (SizeT ix = pxmin; ix <= pxmax; ++ix) {
            DDouble dx = x[ix] - x3;
            DDouble a = y23 * dx + x32 * dy;
            if (a < minD || a > maxD) continue;
            DDouble b = y31 * dx + x13 * dy;
            if (b < minD || b > maxD) continue;
            DDouble c = det - a - b;
            if (c < minD || c > maxD) continue;
            DDouble valbisR = (a * zac.real() + b * zbc.real()) / det + zc.real();
            DDouble valbisI = (a * zac.imag() + b * zbc.imag()) / det + zc.imag();
            if ((dominvalue && valbisR < minVal.real()) || (domaxvalue && valbisR > maxVal.real())) {
              valbisR = (*res)[start + ix].real();
            }
            if ((dominvalue && valbisI < minVal.imag()) || (domaxvalue && valbisI > maxVal.imag())) {
              valbisI = (*res)[start + ix].imag();
            }
            (*res)[start + ix] = std::complex<double>(valbisR, valbisI);
          } //ix loop
        } // iy
      } //tri
    } else {
      for (SizeT triIndex = 0; triIndex < ntri; ++triIndex) {
        // first, find whose region of grid is concerned by this triangle: boundingBox.
        DDouble xmin;
        DDouble xmax;
        DDouble ymin;
        DDouble ymax;
        DDouble x1, y1, x2, y2, x3, y3;
        x1 = (*xVal)[(*tri)[3 * triIndex]];
        y1 = (*yVal)[(*tri)[3 * triIndex]];
        x2 = (*xVal)[(*tri)[3 * triIndex + 1]];
        y2 = (*yVal)[(*tri)[3 * triIndex + 1]];
        x3 = (*xVal)[(*tri)[3 * triIndex + 2]];
        y3 = (*yVal)[(*tri)[3 * triIndex + 2]];
        xmin = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
        ymin = y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
        xmax = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
        ymax = y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

        // find first and last index of x and y that will intersect this BBox
        DLong k = 0;
        while (k < (nx - 1) && x[k + 1] < xmin) {
          k++;
        }
        int pxmin = k;
        while (k < (nx - 1) && x[k + 1] < xmax) {
          k++;
        }
        int pxmax = k;
        k = 0;
        while (k < (ny - 1) && y[k + 1] < ymin) {
          k++;
        }
        int pymin = k;
        while (k < (ny - 1) && y[k + 1] < ymax) {
          k++;
        }
        int pymax = k;

        DComplexDbl zc = (*zVal)[(*tri)[3 * triIndex + 2]];
        DComplexDbl zac = std::complex<double>((*zVal)[(*tri)[3 * triIndex]].real() - zc.real(), (*zVal)[(*tri)[3 * triIndex]].imag() - zc.imag());
        DComplexDbl zbc = std::complex<double>((*zVal)[(*tri)[3 * triIndex + 1]].real() - zc.real(), (*zVal)[(*tri)[3 * triIndex + 1]].imag() - zc.imag());
        DDouble y23 = (y2 - y3);
        DDouble x32 = (x3 - x2);
        DDouble y31 = (y3 - y1);
        DDouble x13 = (x1 - x3);
        DDouble det = (y23 * x13 - x32 * y31);
        DDouble minD = (det<0)?det:0 ; // we MUST be agnostic as to the orientation of the triangle, even if TRIANGULATE is clockwise.
        DDouble maxD = (det>0)?det:0;
        for (SizeT iy = pymin; iy <= pymax; ++iy) {
          DDouble dy = y[iy] - y3;
          DLong start = iy*nx;
          for (SizeT ix = pxmin; ix <= pxmax; ++ix) {
            DDouble dx = x[ix] - x3;
            DDouble a = y23 * dx + x32 * dy;
            if (a < minD || a > maxD) continue;
            DDouble b = y31 * dx + x13 * dy;
            if (b < minD || b > maxD) continue;
            DDouble c = det - a - b;
            if (c < minD || c > maxD) continue;
            DDouble valbisR = (a * zac.real() + b * zbc.real()) / det + zc.real();
            DDouble valbisI = (a * zac.imag() + b * zbc.imag()) / det + zc.imag();
            (*res)[start + ix] = std::complex<double>(valbisR, valbisI);
          } //ix loop
        } // iy
      } //tri
    }
  }

  BaseGDL* trigrid_fun_plane(EnvT* e) {
    SizeT nParam = e->NParam();
    if (nParam < 4) e->Throw("Incorrect number of arguments.");
    //OK, trigrid does not care if more than 3 args in sphere mode. Limit at 6 is done at interpreter level.


    BaseGDL* p0 = e->GetParDefined(0); //x
    BaseGDL* p1 = e->GetParDefined(1); //y
    BaseGDL* p2 = e->GetParDefined(2); //z

    DType type=GDL_FLOAT;
    bool isComplex=(p2->Type()==GDL_COMPLEX || p2->Type()==GDL_COMPLEXDBL);
    //Double output is not only defined by z but also by x and y.
    if (p2->Type()==GDL_DOUBLE || p2->Type()==GDL_COMPLEXDBL) type=GDL_DOUBLE;
    else if (p1->Type()==GDL_DOUBLE || p1->Type()==GDL_COMPLEXDBL) type=GDL_DOUBLE;
    else if (p0->Type()==GDL_DOUBLE || p0->Type()==GDL_COMPLEXDBL) type=GDL_DOUBLE;
    
    BaseGDL* p3 = e->GetParDefined(3); //triangles

    if (p0->N_Elements() != p1->N_Elements() ||
      p0->N_Elements() != p2->N_Elements() ||
      p1->N_Elements() != p2->N_Elements())
      e->Throw("X, Y, or Z array dimensions are incompatible.");

    if (p0->N_Elements() < 3) e->Throw("Value of Bounds is out of allowed range.");

    if (p3->Rank() == 0)            e->Throw("Expression must be an array in this context: " + e->GetParString(0));
    if (p3->N_Elements() % 3 != 0)  e->Throw("Array of triangles incorrectly dimensioned.");

    DLong ntri = p3->N_Elements() / 3;

    if (p0->Rank() == 0)      e->Throw("Expression must be an array in this context: " + e->GetParString(0));
    if (p1->Rank() == 0)      e->Throw("Expression must be an array in this context: " + e->GetParString(1));
    if (p2->Rank() == 0)      e->Throw("Expression must be an array in this context: " + e->GetParString(2));

    static int xoutIx=e->KeywordIx( "XOUT");
    bool doXout=(e->KeywordSet(xoutIx)); 
    static int youtIx=e->KeywordIx( "YOUT");
    bool doYout=(e->KeywordSet(youtIx));
    if ((doXout && !doYout) || (doYout && !doXout)) e->Throw("Incorrect number of arguments.");
    static int xgridIx=e->KeywordIx( "XGRID");
    bool doXgrid=(e->KeywordPresent(xgridIx));
    static int ygridIx=e->KeywordIx( "YGRID");
    bool doYgrid=(e->KeywordPresent(ygridIx));

    static int inputIx=e->KeywordIx( "INPUT");
    bool doInput=(e->KeywordPresent(inputIx));
    
    // Get NX, NY values if present
    DLong nx = 51;
    DLong ny = 51;
    static int nxIx = e->KeywordIx("NX");
    static int nyIx = e->KeywordIx("NY");
    bool canUseLimitsx=true;
    bool canUseLimitsy=true;
    if (e->KeywordSet(nxIx)) { canUseLimitsx=false; e->AssureLongScalarKW(nxIx, nx);}
    if (e->KeywordSet(nyIx)) { canUseLimitsy=false; e->AssureLongScalarKW(nyIx, ny);}
    if (nx < 1 || ny < 1) e->Throw("Array dimensions must be greater than 0.");
    // we will further on define GS, whose value may overwrite the nx, ny.
 
    DDoubleGDL* GS = NULL;
    DDoubleGDL* limits = NULL;
    if (nParam > 4)
    {
      BaseGDL* p4 = e->GetParDefined(4);
      if (p4->Rank() == 0) e->Throw("Expression must be an array in this context: " + e->GetParString(4));
      if (p4->N_Elements() != 2) e->Throw("Array must have 2 elements: " + e->GetParString(4));
      GS = static_cast<DDoubleGDL*>(p4->Convert2(GDL_DOUBLE, BaseGDL::COPY));
    }
    if (nParam == 6)
    {
      BaseGDL* p5 = e->GetParDefined(5);
      if (p5->Rank() == 0) e->Throw("Expression must be an array in this context: " + e->GetParString(5));
      if (p5->N_Elements() != 4) e->Throw("Array must have 4 elements: " + e->GetParString(5));
      limits = static_cast<DDoubleGDL*> (p5->Convert2(GDL_DOUBLE, BaseGDL::COPY));      
    }
    DLong minxEl;
    DLong maxxEl;
    DLong minyEl;
    DLong maxyEl;
    DDoubleGDL* xVal = static_cast<DDoubleGDL*>(p0->Convert2(GDL_DOUBLE, BaseGDL::COPY));
    DDoubleGDL* yVal = static_cast<DDoubleGDL*>(p1->Convert2(GDL_DOUBLE, BaseGDL::COPY));
    DLongGDL* tri = static_cast<DLongGDL*>(p3->Convert2(GDL_LONG, BaseGDL::COPY));
    //get min max X Y.
    xVal->MinMax(&minxEl, &maxxEl, NULL, NULL, true);
    yVal->MinMax(&minyEl, &maxyEl, NULL, NULL, true);
    // Determine grid range
    DLong xref=0, yref=0;
    DDouble xval = (*xVal)[minxEl];
    DDouble yval = (*yVal)[minyEl];
    DDouble xrange = (*xVal)[maxxEl] - (*xVal)[minxEl];
    DDouble yrange = (*yVal)[maxyEl] - (*yVal)[minyEl];
    //compute World positions of each pixels.
    if (limits != NULL)
    {
      if (canUseLimitsx) {
        xval = (*limits)[0];
        xrange = (*limits)[2] - xval;
      }
      if (canUseLimitsy) {
        yval = (*limits)[1];
        yrange = (*limits)[3] - yval;
      }
    }
    // Determine grid spacing
    DDouble xinc = xrange / (nx-1);
    DDouble yinc = yrange / (ny-1);
    if (GS != NULL && canUseLimitsx)
    {
      xinc = (*GS)[0];
      nx = (DLong) ceil(xrange / xinc) +1;
    }
    if (GS != NULL && canUseLimitsy)
    {
      yinc = (*GS)[1];
      ny = (DLong) ceil(yrange / yinc) +1;
    }

    DDouble *x, *y;
    bool cleanx=false;
    bool cleany=false;

    if (doXout) {
      DDoubleGDL* xOut=e->GetKWAs<DDoubleGDL>(xoutIx);
      nx=xOut->N_Elements();
      if (nx < 1) e->Throw("XOUT must have >1 elements.");
      x=(DDouble*)xOut->DataAddr();
    } else {
      x = (DDouble*)malloc(nx*sizeof(DDouble));
      for (int i=0; i<nx; ++i) x[i]=(i-xref)*xinc+xval;
      cleanx=true;
    }

    if (doYout) {
      DDoubleGDL* yOut=e->GetKWAs<DDoubleGDL>(youtIx);
      ny=yOut->N_Elements();
      if (ny < 1) e->Throw("YOUT must have >1 elements.");
      y=(DDouble*)yOut->DataAddr();
    } else {
      y = (DDouble*)malloc(ny*sizeof(DDouble));
      for (int j=0; j<ny; ++j) y[j]=(j-yref)*yinc+yval;
      cleany=true;
    }

    //must be done here
    if (doXgrid) {
      DDoubleGDL *xgrid=new DDoubleGDL(nx, BaseGDL::NOZERO);
      for (int i=0; i<nx; ++i) (*xgrid)[i]=x[i];
      e->SetKW(xgridIx, xgrid);
    }
    if (doYgrid) {
      DDoubleGDL *ygrid=new DDoubleGDL(ny, BaseGDL::NOZERO);
      for (int i=0; i<ny; ++i) (*ygrid)[i]=y[i];
      e->SetKW(ygridIx, ygrid);
    }
    
    // Setup return array
    DLong dims[2];
    dims[0] = nx;
    dims[1] = ny;
    dimension dim((DLong *) dims, 2);

    // if INPUT kw, test if input is an existing variable
    BaseGDL* inputKwData=NULL;
    DDoubleGDL* inputArray=NULL;
    bool inputArrayPresent=false;
    if (doInput) {
      inputKwData=e->GetKW(inputIx);
      if (!(inputKwData==NULL)) { //does exist, check compatibility 
        if (inputKwData->Dim() != dim) e->Throw("Array has a corrupted descriptor");
        inputArrayPresent=true;
      }
    }

    if (isComplex) {
      DComplexDblGDL* minValG=NULL;
      static int minvalueIx=e->KeywordIx( "MIN_VALUE");
      bool dominvalue=(e->KeywordPresent(minvalueIx));
      if (dominvalue) minValG=e->GetKWAs<DComplexDblGDL>(minvalueIx);
      DComplexDblGDL* maxValG=NULL;
      static int maxvalueIx=e->KeywordIx( "MAX_VALUE");
      bool domaxvalue=(e->KeywordPresent(maxvalueIx));
      if (domaxvalue) maxValG=e->GetKWAs<DComplexDblGDL>(maxvalueIx);
      DComplexDblGDL* missValG=NULL;
      static int missvalueIx=e->KeywordIx( "MISSING");
      bool doMiss=e->KeywordPresent(missvalueIx);
      if (doMiss) missValG=e->GetKWAs<DComplexDblGDL>(missvalueIx);
      DComplexDblGDL* zVal = static_cast<DComplexDblGDL*>(p2->Convert2(GDL_COMPLEXDBL, BaseGDL::COPY));
      DComplexDblGDL* res;
      if (inputArrayPresent) res=static_cast<DComplexDblGDL*>(inputKwData->Convert2(GDL_COMPLEXDBL, BaseGDL::COPY));
      else if (doMiss) {
        res = missValG->New(dim, BaseGDL::INIT);
      } else {
        res= new DComplexDblGDL(dim, BaseGDL::ZERO);
      }

      DComplexDbl minVal=std::complex<double>(0,0);
      if (minValG!=NULL) minVal=std::complex<double>((*minValG)[0].real(),(*minValG)[0].imag());
      DComplexDbl maxVal=std::complex<double>(0,0);
      if (maxValG!=NULL) maxVal=std::complex<double>((*maxValG)[0].real(),(*maxValG)[0].imag());
      if (doXout) gdlGrid2DDataCpx(nx, x, ny, y, ntri, tri, xVal, yVal, zVal, domaxvalue, dominvalue, maxVal, minVal, res, inputArrayPresent);
      else gdlGrid2DDataCpx(nx, x, xref,xval,xinc, ny, y, yref,yval,yinc,ntri, tri, xVal, yVal, zVal, domaxvalue, dominvalue, maxVal, minVal, res, inputArrayPresent);
      if (type==GDL_FLOAT) return res->Convert2(GDL_COMPLEX,BaseGDL::CONVERT); else return res;
    }else{
      DDouble minVal=0;
      static int minvalueIx=e->KeywordIx( "MIN_VALUE");
      bool dominvalue=(e->KeywordPresent(minvalueIx));
      if (dominvalue) e->AssureDoubleScalarKW(minvalueIx, minVal);
      DDouble maxVal=0;
      static int maxvalueIx=e->KeywordIx( "MAX_VALUE");
      bool domaxvalue=(e->KeywordPresent(maxvalueIx));
      if (domaxvalue) e->AssureDoubleScalarKW(maxvalueIx, maxVal);
      DDoubleGDL* missVal=NULL;
      static int missvalueIx=e->KeywordIx( "MISSING");
      bool doMiss=e->KeywordPresent(missvalueIx);
      if (doMiss) missVal=e->GetKWAs<DDoubleGDL>(missvalueIx);
      DDoubleGDL* zVal = static_cast<DDoubleGDL*>(p2->Convert2(GDL_DOUBLE, BaseGDL::COPY));
      DDoubleGDL* res;
      if (inputArrayPresent) {
        res = static_cast<DDoubleGDL*> (inputKwData->Convert2(GDL_DOUBLE, BaseGDL::COPY));
      } else if (doMiss) {
        res = missVal->New(dim, BaseGDL::INIT);
      } else {
        res = new DDoubleGDL(dim, BaseGDL::ZERO);
      }
      if (doXout) gdlGrid2DData< DDoubleGDL, DDouble>(nx, x, ny, y, ntri, tri, xVal, yVal, zVal, domaxvalue, dominvalue, maxVal, minVal, res, inputArrayPresent);
      else gdlGrid2DData< DDoubleGDL, DDouble>(nx, x, xref,xval,xinc, ny, y, yref,yval,yinc,ntri, tri, xVal, yVal, zVal, domaxvalue, dominvalue, maxVal, minVal, res, inputArrayPresent);
      if (type==GDL_FLOAT) return res->Convert2(GDL_FLOAT,BaseGDL::CONVERT); else return res;
    }
  }
  

  BaseGDL* trigrid_fun(EnvT* e) {
    //just send to normal or spherical based on presence of SPHERE argument
    static int sphereIx = e->KeywordIx( "SPHERE");
    if(e->KeywordPresent(sphereIx)) return trigrid_fun_spherical(e); else return trigrid_fun_plane(e);
  }
  
  void grid_input(EnvT* e) {
    e->Throw("Writing in progress.");
  }
}

