/** 
    
    This file is part of NxOgre.
    
    Copyright (c) 2009 Robin Southern, http://www.nxogre.org
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
    
*/

                                                                                       

#ifndef NXOGRE_MESHRENDERABLE_H
#define NXOGRE_MESHRENDERABLE_H

                                                                                       

#include "NxOgreStable.h"
#include "NxOgreCommon.h"
#include "NxOgreVertexBuffer.h"
#include "NxOgreIndexBuffer.h"
#include "NxOgreMeshStats.h"

                                                                                       

namespace NxOgre
{

                                                                                       

/*! class. MeshRenderable
    desc.
        Set of vertices, indexes, normals and texture coordinates for Cloth or SoftBody meshes, that
        are read by the user for rendering. These vertices/indexes will be updated frequently, and
        used with a Renderable class.
*/
class NxOgrePublicClass MeshRenderable
{
  
 public: // Functions
  
  /*! constructor. MeshRenderable
  */
  MeshRenderable(Mesh* mesh, float tearingFactor = 1.5f);
  
  /*! destructor. MeshRenderable
  */
 ~MeshRenderable();
  
  /* private function
  */
  void getPhysXMeshData(NxMeshData&);
  
 public:

  VertexBuffer             vertices;

  NormalBuffer             normals;

  IndexBuffer              indexes;

  IndexBuffer              parent_indexes;

  TextureCoordinateBuffer  texture_coordinates;

  protected:
  
  /* configure
  */
  void configure(float tearingFactor);
  
  Mesh*       mMesh;
  
  MeshStats   mStats;
   
}; // class ClothRenderable

                                                                                       

} // namespace NxOgre

                                                                                       

#endif
