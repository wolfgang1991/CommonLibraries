#ifndef TRIANGULATE_H
#define TRIANGULATE_H

//based on code by John W. Ratcliff (jratcliff@verant.com) in public domain (see also: https://lists.w3.org/Archives/Public/public-sdw-wg/2015Dec/0067.html )

#include <irrArray.h>
#include <vector2d.h>

// a polygon/contour and a series of triangles.
typedef irr::core::array< irr::core::vector2d<irr::f32> > Vector2dVector;


class Triangulate
{
public:

  // triangulate a contour/polygon, places results in array
  // as series of triangles. (array allocated size must be large enough)
  static bool Process(const Vector2dVector &contour,
                      Vector2dVector &result);

  // compute area of a contour/polygon
  static float Area(const Vector2dVector &contour);

  // decide if point Px/Py is inside triangle defined by
  // (Ax,Ay) (Bx,By) (Cx,Cy)
  static bool InsideTriangle(float Ax, float Ay,
                      float Bx, float By,
                      float Cx, float Cy,
                      float Px, float Py);


private:
  static bool Snip(const Vector2dVector &contour,int u,int v,int w,int n,int *V);

};


#endif
