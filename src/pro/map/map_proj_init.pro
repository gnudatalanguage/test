; MAP_PROJ_INIT
; Documentation in progress
; equivalent to the original procedure, except that instead of 40 or
; so projections here we give access to all the proj4 projections (120+)
; Note: the structure of the resulting mapstruct is not compatible with
; IDL, i.e., one cannot use a GDL-defined mapstruct in IDL (if passed
; within a save file), (but one can use an IDL-defined mapstruct in GDL).

pro map_proj_set_split, myMap

    compile_opt idl2, hidden
    
    sinlat = sin(myMap.v0)
    coslat = cos(myMap.v0)
    sinlon = sin(myMap.u0)
    coslon = cos(myMap.u0)
    xyzProjCenter = [coslon*coslat, $
                     sinlon*coslat, sinlat]
    pole = myMap.pole[4:6]        ;Location of pole
    plane = CROSSP(xyzProjCenter, pole)
    split=[myMap.p0lon, myMap.p0lat, plane, 0d]
    MAP_CLIP_SET, MAP=myMap, SPLIT=split
end

function map_proj_init, pindex, p4number=p4number, relaxed=relaxed, rotation=rotation, gctp=gctp, radians=radians, limit=passed_limit, ellipsoid=ellipsoid, semimajor_axis=semimajor_axis, semiminor_axis=semiminor_axis, sphere_radius=sphere_radius, datum=datum, clip=clip,  gdl_precise=gdl_precise, _extra=extra

    compile_opt idl2, hidden

    ON_ERROR, 2  ; return to caller

; NOTE: We are always "relaxed".
; p4num bool indicates pindex is a number and refers to the internal proj4 table of proj4 properties line and not an IDL number for which an equivalent must be found 

; define limit a zeros if not present 
if n_elements(passed_limit) lt 4 then limit=dblarr(4) else limit=passed_limit
; this boolean says some non-default limits are set and limit clippings will be sufficient (To be Tested Thoroughly)
limited=((limit[0] ne limit[2]) or (limit[1] ne limit[3]))

; the common contains all relevant values after initialisation
@gdlcommon_mapprojections_common

nkeys=n_tags(required)

required_kw4=" +"+strlowcase(tag_names(required))+"="

nproj=n_elements(proj)

sindex=pindex
; find projection index, by index:
if (N_ELEMENTS(pindex) le 0) then begin 
   index=where(proj.proj4name eq 'stere') ; stereo is default
; this is a drawback: projection number is always an IDL number 
endif else if (SIZE(pindex, /TYPE) ne 7) then begin
   if keyword_set(p4number) then sindex=proj[pindex].fullname else sindex=idl_ids[pindex]
endif
; now by name, as pindex is a string

; grep best candidate name
shortname = strupcase(strcompress(sindex, /REMOVE_ALL))
                                ;defaults to IDL projection_names, check:
w = strcmp(idl_ids,shortname,strlen(shortname)) & count=total(w)
if count gt 1 then message, /noname, 'Ambiguous Projection abbreviation: ' + sindex
if count eq 1 then begin
   name4=idl_equiv[(where(w eq 1))[0]]
   index=where(proj.proj4name eq name4, count) & if count eq 0 then message, 'Projection ' + sindex + ' apparently does not exist in Proj4 library, fixme.' 
endif else begin
   ; next, proj4 projections
   w = strcmp(compressed_ids1,shortname,strlen(shortname)) & count=total(w)
   if count eq 0 then begin w = strcmp(compressed_ids2,shortname,strlen(shortname)) & count=total(w) & end ; alternative
   if count eq 0 then message, /noname, 'Invalid Projection name: ' + sindex
   if count eq 1 then index=where(w eq 1) else begin ; gt 1: ambiguous
      message, /noname,/informational,'Ambiguous Projection name: ' + sindex
      pos=where(w eq 1)
      choices=compressed_ids1[pos]
      lengths=strlen(choices)
      match=min(lengths,j)
      adopted=choices[j]
      w = strcmp(compressed_ids1,adopted,strlen(compressed_ids1))
      pos=where(w eq 1)
      index=pos[0]
   endelse
endelse

; no more than nproj-1
if index ge nproj then message, /noname,   'Invalid Projection number: ' + strtrim(sindex)

; useful strings:
; required parameters, filled.
filled_required_parameter_string=""
list_of_needed_params=""
; optional parameters as passed
filled_optional_parameter_string=""

; create list of REQUIRED parameters
required_opt=""
for i=0,nkeys-1 do if required[index].(i) then required_opt+=required_kw4[i]
if strlen(required_opt) gt 0 then begin
   list_of_needed_params=strsplit(required_opt,' ',/extract)
   n_required=n_elements(list_of_needed_params)
endif else n_required=0

; all optional elements in a string; add false northing easting that are
; common for all projections.
optional_opt=optional[index]+" x_0= y_0="
list_of_optional_params="+"+strsplit(optional_opt," ",/extract)
n_optional = n_elements(list_of_optional_params)
;print,list_of_optional_params
;define type(s) of current projection 
property=proj_properties[index]
if (property.EXIST eq 0) then message,'Unfortunately, projection '+shortname+ ' is flagged as absent. Please check MAP_INSTALL in GDL documentation.'
conic=(property.CONIC eq 1)
elliptic=(property.ELL eq 1)
spheric=(property.SPH eq 1)
cylindric=(property.CYL eq 1)
azimuthal=(property.AZI eq 1)
interrupted=(property.INTER eq 1)
; test possibility of applying rotation.
; it can be forbidden by definition of NOROT
rotPossible=(property.NOROT eq 0)
south=0
; this logical tells if a coordinate rotation can be tempted
; basically if projection accepts lat_0 or lat_1 or lat_2 or lat_ts as parameter
; (required or optional) it is not necessary to use the
; general oblique transformation to permit non-zero center_latitudes.
; But, worse, if projection accepts lat_ts it will be extremely
; disagreeable to use a general oblique transformation...
replaceCenterbyTrueScale=0
if (rotPossible) then begin
   w=where( strpos([list_of_needed_params,optional_opt],'lat_') ge 0, count)
   RotPossible=(count eq 0)
   w1=where( strpos(([list_of_needed_params,optional_opt])[w],'lat_ts') ge 0, count)
   replaceCenterByTrueScale=(count gt 0) ; some projections, like default "stereo" use lat_ts in proj4 and CENTER_LATITUDE in IDL. We'll translate.
endif

; there are passed arguments, but no need to bother if they are less
; than required.

passed_params=""

n_passed=0

; enable abbreviated parameters
if n_elements(extra) gt 0 then begin
   
   passed_params = TAG_NAMES(extra)
   
   ; instead of comparing passed_params directly to hash key, check possible abbreviations:
   possible_params=tag_names(dictionary.tostruct())
   for i=0,n_tags(extra)-1 do begin
      w = strcmp(possible_params,passed_params[i],strlen(passed_params[i])) & count=total(w) & j=(where(w eq 1))[0]
      if count eq 1 then begin
         passed_params[i]=possible_params[j] ; make passed_params normalized.
         proj4kw=dictionary[possible_params[j]]
         list_of_passed_params= (n_elements(list_of_passed_params) eq 0)? proj4kw : [list_of_passed_params,proj4kw]
         passed_values= (n_elements(passed_values) eq 0)? strtrim(extra.(i),2) : [passed_values,strtrim(extra.(i),2)]
      endif else if count gt 1 then message,"Ambiguous keyword abbreviation: "+passed_params[i]
   endfor
   n_passed=n_elements(list_of_passed_params) ; the only useful, recognized, ones, along with their passed_values.
endif

; case of lat_ts
if n_passed gt 0 and replaceCenterByTrueScale then begin
 ; if lat_ts is present, no need to convert lat_0 to lat_ts
  w=where(list_of_passed_params eq '+lat_ts=', count) & if count le 0 then begin
     w=where(list_of_passed_params eq '+lat_0=', count) & if count ne 0 then list_of_passed_params[w]='+lat_ts=' ;
                                ;as it will be searched further down either in list_of_needed_params or in  list_of_optional_params, we need to update both
;  w=where(list_of_needed_params eq '+lat_0=', count) & if count ne 0 then list_of_needed_params[w]='+lat_ts=' ;
;  w=where(list_of_optional_params eq '+lat_0=', count) & if count ne
;  0 then list_of_optional_params[w]='+lat_ts=' ;
  endif
endif

if n_passed gt 0 and n_passed ge n_required then begin
; do we have required values for projection?
   if n_required gt 0 then begin
; needed params: get individual proj4 keywords, find if equivalent is
; existing in passed_params. Based on equivalence list above.

; sort in alphabetic order
      s_needed=sort(list_of_needed_params)
      ;create index table
      tindex=(intarr(n_passed))-1; //-1 to further check
      ; find all matches
      for i=0,n_required-1 do begin
         w=where(list_of_passed_params eq list_of_needed_params[i], count) & if count gt 0 then  tindex[i]=w[0] ; we do not check duplicated entries, we take first.
      endfor
      ; tindex positive values need equal n_required here if all match were made
      w=where(tindex ge 0, count, comp=absent) & if count ne n_required then begin
         kwlist=""
         ; reverse match: now we want needed_params that are not in passed_params.
         for i=0,n_required-1 do begin
            w=where(list_of_needed_params[i] eq list_of_passed_params, count) & if count eq 0 then kwlist+=yranoitcid[list_of_needed_params[i]]+" "
         endfor
         message,"Missing required parameters: "+kwlist
      endif
      ; populate required parameter list
      for i=0,n_required-1 do begin
         ; filter negative values for zone and set south
         if list_of_needed_params[i] eq "+zone=" then begin
            the_zone=fix(passed_values[tindex[i]])
            if the_zone lt 0 then begin
               passed_values[tindex[i]]=strtrim(-1*the_zone,2)
               south=1
            endif
         endif
         filled_required_parameter_string+=" "+list_of_needed_params[i]+passed_values[tindex[i]]
      endfor
   endif

   ; do we have optional values for projection?
   if n_optional gt 0 then begin
      ; sort in alphabetic order
                                ;create index table
      tindex=(intarr(n_optional))-1 ; //-1 to further check
                                ; find all matches
      for i=0,n_optional-1 do begin
         w=where(list_of_passed_params eq list_of_optional_params[i], count) & if count gt 0 then tindex[i]=w[0] 
      endfor
                                ; w is the short list of optional passed values
      w=where(tindex ge 0, count) & if count gt 0 then begin
                                ; populate optional parameter list
         tindex=tindex[w]
         for i=0,count-1 do filled_optional_parameter_string+=" "+list_of_passed_params[tindex[i]]+passed_values[tindex[i]]; strtrim(extra.(tindex[i]),2)
      endif
   endif
endif else begin                ; or not...
   if strlen(required_opt) gt 0 then  message, "Absent (proj4) parameter(s): "+required_opt
endelse

; main string (will need special treatment for rotation etc.)
proj4command="+proj="+proj[index].proj4name+" "

proj4options=filled_required_parameter_string+filled_optional_parameter_string

; ok, proj4options contains all relevant AND permitted parameters. Try to assemble
; all these into valid elements of !map. Up to now we have only
; translated from idl to proj4. now is time to interpret things a bit.

; define defaults values
p0lon = 0d                      ; center longitude
p0lat = 0d                      ; center latitude
 if n_elements(rotation) le 0 then rotation=0d ; rotation
p1=0 ; locally used standard parallels p1 and p2
p2=0
satheight=0

if strlen(proj4options) gt 0 then begin
; convert proj4options to hash
   s=strsplit(strtrim(proj4options,2),"= ",/extract)
   x=where(strpos(s,'+') eq 0, comp=y)
   a=hash(s[x],s[y])
; if a contains "+lon_0" this is p0lon, etc.
   if a.HasKey("+lon_0") then p0lon=(a["+lon_0"]*1d)[0]
   if a.HasKey("+lat_0") then begin
;      if ~rotPossible then message,"projection "+
      p0lat=(a["+lat_0"]*1d)[0]
   endif
   if a.HasKey("+lat_1") then p1=(a["+lat_1"]*1d)[0]
   if a.HasKey("+lat_2") then p2=(a["+lat_2"]*1d)[0]
   if a.HasKey("+h")     then satheight=a["+h"]*1d
endif
; adjust ranges
map_adjlon,p0lon

p0lat= p0lat > (-89.999) & p0lat=p0lat < 89.999
p1=p1 > (-89.999) & p1=p1<89.999
p2=p2 > (-89.999) & p2=p2<89.999
;if (p2 lt p1) then begin & tmp=p2 & p2=p1 & p1=p2 & end 
if (rotPossible) then begin
  if replaceCenterByTrueScale then search_string='lat_ts=' else search_string='+lat_0='
; try a general oblique transformation
   if spheric and n_passed ne 0 then begin
      w=where(list_of_passed_params eq search_string, count)
      if count gt 0 then begin  ; try general oblique
         p0lat=extra.(w[0])
         if p0lat ne 0 then begin 
            if p0lat gt 89.999 then p0lat = 89.999 ;take some precautions as PROJ.4 is not protected!!! 
            if p0lat lt -89.999 then p0lat = -89.999 ;
            proj4command="+proj=ob_tran +o_proj="+proj[index].proj4name+" +o_lat_p="+strtrim(90.0-p0lat,2) ; center azimuth not OK, FIXME! +" +o_lon_p="+strtrim(center_azimuth,2)
            p0lat = 0.0
         endif
      endif
   endif
endif
; insure following values are zero-dimension doubles (due to Hash, could be arrays)

; for conic projections, although lat_0 is not in the list of
; authorized parameters, it works, so we add it, as it is very
; important to center the projection.
if (~rotPossible and conic or spheric) then begin ;  ~elliptic ?
 w=where(list_of_passed_params eq '+lat_0=', count)
 if count gt 0 then begin
    val=passed_values[w[0]]
    p0lat=double(val)
    proj4options+=" +lat_0="+val
 endif
endif


; create a 999 !map
myMap={!map}
myMap.projection=999
mymap.p[15]=index ;!useful for map_proj_info and unused apparently.
myMap.p0lon = p0lon
myMap.p0lat = p0lat
myMap.u0 = p0lon * !dtor
myMap.v0 = p0lat * !dtor
;myMap.a = semimajor     ; ellipsoid --> need table of correspondences!
;myMap.e2 = e2

myMap.rotation = rotation                      ; map rotation
myMap.cosr=cos(rotation*!dtor)
myMap.sinr=sin(rotation*!dtor)
myMap.pole=[0,!DPI/2,0,0,0,0,1] ; need to define myMap.pole BEFORE calling MAP_PROJ_SET_SPLIT 

MAP_CLIP_SET, MAP=myMap, /RESET        ;Clear clipping pipeline.
; do various clever things...
; need to get base proj4 name!
p4n=proj[index].proj4name
; need to keep ony the real proj4 name if perchance there was additional commands already set in the name
p4n=(strsplit(strtrim(p4n,2),' ',/extract))[0] 

; 1) get !map useful values
; radius or ell or..
myMap.a=6370997.0d ; default
myMap.e2=1
hasRadius=0 
ellipticalusagerequired=(p4n eq "utm" or p4n eq "ups")
if n_elements(sphere_radius) gt 0 then begin 
   hasRadius=1
   myMap.a=sphere_radius[0]
endif else begin 
   radius=6370997.0d ; myMap.a already ok
endelse

hasEll=0 

if n_elements(ellipsoid) gt 0 or n_elements(datum) gt 0 then begin ; case where myMap.a will be false!
   hasEll=1
   if n_elements(datum) gt 0 and n_elements(ellipsoid) eq 0 then ellipsoid=datum ; in case both are present.
   if ~(SIZE(ellipsoid, /TYPE) eq 7) then begin ; must give name (index in list)
      if ellipsoid gt 25 or ellipsoid lt 0 then message,"Invalid value for keyword ELLIPSOID: "+strtrim(ellipsoid,2)
      ellipsoid=ellipsoid_proj[ellipsoid] 
   endif else begin
      w = strcmp(strupcase(ellipsoid_idl),strupcase(ellipsoid),strlen(ellipsoid)) & count=total(w)
      if count gt 0 then begin
         pos=where(w gt 0) & ellipsoid=ellipsoid_proj[pos[0]]
      endif
   endelse
endif 

; defining +a and +b overrides R and Ell
hasDefinedEll=0
if n_elements(SEMIMAJOR_AXIS) gt 0 then begin
 hasRadius=0
 hasEll=0
 if ~n_elements(SEMIMINOR_AXIS) gt 0 then Message,"Keywords SEMIMAJOR_AXIS and SEMIMINOR_AXIS must both be supplied."
 hasDefinedEll=1
 myMap.a=SEMIMAJOR_AXIS
 f=1.-(semiminor_axis/semimajor_axis)
 myMap.e2 = 2*f-f^2
endif

; 2) split, clip..
; treat all interrupted cases separately (probably need to change !Map
; pipeline size to accomodate for many-faceted projections: not done
; but easy!
; non-azimuthal projections: split at 180 degrees from map center,
; then:
; conic: will cut out pole region at some lat, and stop somewhere on
; the other side, usually not far from the lat_2 if exists.
; cylindric: should cut 'cylinder ends' ---> not done properly for
; transverse?
; azim: should cut somewhere: gnomonic cannot show one hemisphere,
; other can, but will be very distorted.
; transverse mercator projections are treated also in "interrupted"
if (interrupted or p4n eq 'bipc') then begin
 case p4n of
         "bipc": BEGIN
       splits = [-20,-110] + p0lon

       for i=0,n_elements(splits)-1 do begin 
          theta = !dtor * splits[i]
          MAP_CLIP_SET, map=myMap, SPLIT=[splits[i], 0, -sin(theta), cos(theta), 0., 0.]
       endfor 
       myMap.up_flags=1000 ; redefine "epsilon" due to precision problems in proj4!!!!

         END

    "igh": BEGIN
       splits = [-180, -40, -100, -20, 80] + 180d + p0lon
       
       for i=0,n_elements(splits)-1 do begin 
          theta = !dtor * splits[i]
          MAP_CLIP_SET, map=myMap, SPLIT=[splits[i], 0, -sin(theta), cos(theta), 0., 0.]
       endfor 
       myMap.up_flags=1000000 ; redefine "epsilon" due to precision problems in pr
    END

    "rhealpix": BEGIN
       splits = [-3*45, -2*45, -45, 0, 45, 2*45, 3*45] + 180d + p0lon

       for i=0,n_elements(splits)-1 do begin 
          theta = !dtor * splits[i]
          MAP_CLIP_SET, map=myMap, SPLIT=[splits[i], 0, -sin(theta), cos(theta), 0., 0.]
       endfor 
       map_clip_set, map=myMap, SPLIT=[0,90,0,0,1d,-2d/3d]
       map_clip_set, map=myMap, SPLIT=[0,-90,0,0,-1d,-2d/3d]
       myMap.up_flags=10000 ; redefine "epsilon" due to precision problems in proj4!!!!
    END

    "healpix": BEGIN
       splits = [-3*45, -2*45, -45, 0, 45, 2*45, 3*45] + 180d + p0lon

       for i=0,n_elements(splits)-1 do begin 
          theta = !dtor * splits[i]
          MAP_CLIP_SET, map=myMap, SPLIT=[splits[i], 0, -sin(theta), cos(theta), 0., 0.]
       endfor 
       myMap.up_flags=10000 ; redefine "epsilon" due to precision problems in proj4!!!!
    END
 ELSE: print,"Interrupted projection "+p4n+" is not yet properly taken into account in map_proj_init, please FIXME!"
 endcase
endif else begin ; not interrupted
   if not azimuthal then begin
      MAP_PROJ_SET_SPLIT,myMap ; for all projs non azim
; conics: clip around the poles
      if conic then begin
                                ; apparently clipping is done 10 degrees above or below equator for
                                ; opposite hemisphere and at +75 degrees on same hemisphere unless the
                                ; standard parallels are not on the same side of equator, giving a 75
                                ; degree clip on both sides. 
         test1= (p1 ge 0.0) ? 1 : -1
         test2= (p2 ge 0.0) ? 1 : -1
         if (test1 eq test2) then begin
            map_clip_set, map=myMap, clip_plane=[0,0,test1,sin(!dtor*10.)]
            map_clip_set, map=myMap, clip_plane=[0,0,-1*test2,sin(!dtor*75.0)]
            myMap.p[13]=-1*test1*10. ; use it to store this value, see map_grid, map_horizon
            myMap.p[14]=test2*75.    ; use it to store this value, see map_grid, map_horizon
         endif else begin
            map_clip_set, map=myMap, clip_plane=[0,0,1,sin(!dtor*75.0)]
            map_clip_set, map=myMap, clip_plane=[0,0,-1,sin(!dtor*75.0)]
            myMap.p[13]=75.     ; use it to store this value, see map_grid, map_horizon
            myMap.p[14]=-75.    ; use it to store this value, see map_grid, map_horizon
         endelse
      endif else if cylindric then begin
         map_clip_set, map=myMap, clip_plane=[0,0,1,sin(!dtor*89.99)]
         map_clip_set, map=myMap, clip_plane=[0,0,-1,sin(!dtor*89.99)]
         myMap.p[13]=89.99        ; use it to store this value, see map_grid, map_horizon
         myMap.p[14]=-89.99       ; use it to store this value, see map_grid, map_horizon
      endif
   endif else begin               ; azim projs.
      case p4n of
         "nsper": BEGIN
            if satheight eq 0 then begin 
               MAP_CLIP_SET, MAP=myMap, CLIP_PLANE=[cos(myMap.u0)*cos(myMap.v0), sin(myMap.u0)*cos(myMap.v0), sin(myMap.v0), -0.5d]
               myMap.p[14]=-0.5d ; use it to store this value, see map_grid, map_horizon
            endif else begin
               val=-1.01d /(1+satheight/myMap.a)
               MAP_CLIP_SET, MAP=myMap, CLIP_PLANE=[cos(myMap.u0)*cos(myMap.v0), sin(myMap.u0)*cos(myMap.v0), sin(myMap.v0), val]
               myMap.p[14]=val  ; use it to store this value, see map_grid, map_horizon
            endelse
         END
         "gnom": BEGIN 
            MAP_CLIP_SET, MAP=myMap, CLIP_PLANE=[cos(myMap.u0)*cos(myMap.v0), sin(myMap.u0)*cos(myMap.v0), sin(myMap.v0), -0.5d]
            myMap.p[14]=-0.5d   ; use it to store this value, see map_grid, map_horizon
         END
         ELSE: BEGIN
            MAP_CLIP_SET, MAP=myMap, CLIP_PLANE=[cos(myMap.u0)*cos(myMap.v0), sin(myMap.u0)*cos(myMap.v0), sin(myMap.v0), -1d-8]
            myMap.p[14]=-1d-8   ; use it to store this value, see map_grid, map_horizon
         END
      ENDCASE
   endelse                      ; end azim projs
endelse                         ; not interrupted

if ellipticalusagerequired then proj4Options+=' +ellps=GRS80 '
if south then proj4Options+=' +south'
; finalize projection to be used in finding limits:
myMap.up_name=proj4command+" "+proj4Options

if (hasDefinedEll) then begin
myMap.up_name+=" +a="+strtrim(SEMIMAJOR_AXIS[0],2)+" +b="+strtrim(SEMIMINOR_AXIS[0],2)
endif else if (hasRadius and ~ellipticalusagerequired) then begin
myMap.up_name+=" +R="+strtrim(SPHERE_RADIUS[0],2)
endif else if (hasEll) then begin
myMap.up_name+=" +ell="+ellipsoid
endif

print,myMap.up_name
; 3) Set LIMITs and clip.

gdl_set_map_limits, myMap, limit, gdl_precise=gdl_precise

; 4) transform
MAP_CLIP_SET, MAP=myMap, /transform        ;apply transform

return,myMap
end

; in case one wants to add new projections to the ../resource/maps/projections.csv file.
pro map_proj_auxiliary_read_csv
    compile_opt idl2

    ON_ERROR, 2  ; return to caller
 restore,"csv.sav"
 nproj=n_elements(csv_proj.field1)
 names={PROJ4NAME:"",FULLNAME:"",OTHERNAME:""}
 proj_property={EXIST:1B,SPH:0B,CONIC:0B,AZI:0B,ELL:0B,CYL:0B,MISC:0B,NOINV:0B,NOROT:0B,INTER:0b} ; note uppercase
; fill the sorted, uniq, list of REQUIRED values
 t=strtrim(csv_proj.field5,2) & s=strjoin(t) & t=strsplit(s,"= ",/extract) 
 q=t[sort(t)] & required_template_list=q[uniq(q)]
 for i=0,n_elements(required_template_list)-1 do map_struct_append, required_template, required_template_list[i], 0b

 proj=replicate(names,nproj)
 proj.PROJ4NAME=csv_proj.FIELD1
 proj.FULLNAME=csv_proj.FIELD2
 proj.OTHERNAME=csv_proj.FIELD3

 proj_properties=replicate(proj_property,nproj)
 csv_proj.field4=strupcase(strcompress(csv_proj.field4,/remove_all)) ; note uppercase
 ntags=n_tags(proj_property)
 tname=strupcase(tag_names(proj_property))
 for i=0,ntags-1 do proj_properties[ WHERE(STRMATCH(csv_proj.field4, '*'+tname[i]+'*') EQ 1)].(i)=1

 required=replicate(required_template,nproj)
 ntags=n_tags(required_template)
 tname=strlowcase(tag_names(required_template)) ; ALL LOWCASE in table FOR THE MOMENT. WARNING if NOT!!!
 for i=0,ntags-1 do begin & w=WHERE(STRMATCH(csv_proj.field5, '*'+tname[i]+'=*', /FOLD_CASE) EQ 1, count) & if (count gt 0) then required[w].(i)=1 & end

; optional
 optional=csv_proj.field6
; 
proj_limits=reform(replicate(-1.0,4*nproj),4,nproj)
proj_scale=dblarr(nproj)

; save once to have map_proj_init work.
save,filen="projDefinitions.sav",proj,proj_properties,required,optional,proj_scale,proj_limits

; now compute default limits [-180..180, -90..90] for all projections.
; the idea is to call all the projections with all 'possible'
; parameters in order to have only unexisting projections that cause a
; (trapped) error. As the projection has been set up, the uv box is
; the one computed in map_proj_init using a brute force
; method. Obvioulsy this could be made more exact if the uv_box was
; part of the database, but imho this small amount of work will rebuke
; everybody. An other option would be to use Proj functions to get all
; the needed information, I've not looked into that.  
; call proj_init for uv_box approximate calculation...
for i=0,nproj-1 do begin
   catch,absent
   if absent ne 0 then begin
      print,'i was',i,' projection was ',proj[i].PROJ4NAME
      proj_properties[i].exist=0b
      continue
   endif
   myMap=map_proj_init(/gdl_precise, i,/p4num,height=1,standard_parall=30,standard_par1=50,standard_par2=-45,sat_tilt=45,center_azim=0,center_lon=0,true_scale_latitude=12,lat_3=13,HOM_LONGITUDE1=1,HOM_LONGITUDE2=80,LON_3=120,OEA_SHAPEN=1, OEA_SHAPEM=1,SOM_LANDSAT_NUMBER=2, SOM_LANDSAT_PATH=22, ZONE=28, center_lat=0) ; uses ellps=wgs84 by default.
      proj_scale[i]=abs(myMap.uv_box[2]-myMap.uv_box[0]) ; number of ellipsoid meters in uv_box
endfor
; proj_scale, only on existing projections.
for i=0,nproj-1 do begin
   catch,absent
   if absent ne 0 then begin
      print,'(known?) problem with projection '+proj[i].PROJ4NAME
      continue
   endif

   if proj_properties[i].exist eq 1 then begin 
      myMap=map_proj_init(/gdl_precise,i,/p4num,sphere=1,height=1,standard_parall=30,standard_par1=50,standard_par2=-45,sat_tilt=45,center_azim=0,center_lon=0,true_scale_latitude=12,lat_3=13,HOM_LONGITUDE1=1,HOM_LONGITUDE2=80,LON_3=120,OEA_SHAPEN=1, OEA_SHAPEM=1,SOM_LANDSAT_NUMBER=2, SOM_LANDSAT_PATH=22, ZONE=28, center_lat=0)
     proj_limits[*,i]=myMap.uv_box ; normalized uv_box
   endif
endfor


save,filen="projDefinitions.sav",proj,proj_properties,required,optional,proj_scale,proj_limits
end

;soffice --headless --convert-to csv projections.ods
;csv_proj=read_csv("projections.csv",n_table=1) & save,csv_proj,filename="csv.sav" & exit
; gdl
; .compile map_proj_init.pro
; MAP_PROJ_AUXILIARY_READ_CSV
; exit

pro test_all_projs, from=from, to=to, lon=lon, lat=lat, halt=halt
  on_error,2
  map_proj_info,proj_names=pjn
  ttt=''
  if n_elements(from) eq 0 then from=1
  if n_elements(to) eq 0 then to=n_elements(pjn) else to=to<(n_elements(pjn)+1)
  if n_elements(lon) eq 0 then lon=-2.33
  if n_elements(lat) eq 0 then lat=48.83
  
  for i=from,to do begin
     catch,absent
     if absent ne 0 then begin
        catch,/cancel
        continue
     endif
     map_set,/advance,lat,lon,name=pjn[i],lat_1=12,lat_2=56,lat_ts=33,height=3,e_cont={cont:1,fill:1,color:'33e469'x,hires:0},/hor,e_hor={nvert:200,fill:1,color:'F06A10'x},e_grid={box_axes:0,color:'1260E2'x,glinethick:1,glinestyle:0},title=pjn[i],/iso,center_azimuth=44,sat_tilt=33
     print,i,pjn[i]
     if keyword_set(halt) then read,ttt,prompt='Waiting for keypad input...' else wait,1
  endfor
end
