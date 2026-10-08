#pragma once

#include "Math.hpp"

#include <cmath>

namespace triangle_geometry {

inline constexpr double RelativeCrossSquaredEpsilon = 2.0e-8;

inline bool faceNormal(const Vec3& a,const Vec3& b,const Vec3& c,Vec3& normal) {
    const auto finite=[](const Vec3& v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
    normal={};
    if(!finite(a)||!finite(b)||!finite(c))return false;
    const double ux=static_cast<double>(b.x)-a.x,uy=static_cast<double>(b.y)-a.y,uz=static_cast<double>(b.z)-a.z;
    const double vx=static_cast<double>(c.x)-a.x,vy=static_cast<double>(c.y)-a.y,vz=static_cast<double>(c.z)-a.z;
    const double edgeUSq=ux*ux+uy*uy+uz*uz,edgeVSq=vx*vx+vy*vy+vz*vz;
    if(!(edgeUSq>0.0)||!(edgeVSq>0.0)||!std::isfinite(edgeUSq)||!std::isfinite(edgeVSq))return false;
    const double nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
    const double crossSq=nx*nx+ny*ny+nz*nz,relativeDenominator=edgeUSq*edgeVSq;
    if(!std::isfinite(crossSq)||!std::isfinite(relativeDenominator)||
        crossSq<=relativeDenominator*RelativeCrossSquaredEpsilon)return false;
    const double inverseLength=1.0/std::sqrt(crossSq);
    normal={static_cast<float>(nx*inverseLength),static_cast<float>(ny*inverseLength),static_cast<float>(nz*inverseLength)};
    return finite(normal);
}

}
