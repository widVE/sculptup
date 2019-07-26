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

                                                                                       

#ifndef NXOGRE_RAY_H
#define NXOGRE_RAY_H

                                                                                       

#include "NxOgreStable.h"
#include "NxOgreCommon.h"

                                                                                       

namespace NxOgre
{

                                                                                       

/** \brief
*/
class NxOgrePublicClass Ray
{
  
  
  public: // Functions
  
  /** \brief Constructor with 0,0,0 as Origin and 1,0,0 as Direction.
  */
                                              Ray();
  
  /** \brief Alternate Constructor.
  */
                                              Ray(const Vec3& origin, const Vec3& direction);
  
  /** \brief Destructor.
  */
                                             ~Ray();
  
  public: // Variables
  
  /** \brief The origin in global coordinates of the ray.
  */
                                              Vec3 mOrigin;
  
  /** \brief The normalised direction of the ray.
  */
                                              Vec3 mDirection;
  
  
}; // class ClassName

                                                                                       

} // namespace NxOgre

                                                                                       

#endif
