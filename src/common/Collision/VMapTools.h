/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _VMAPTOOLS_H
#define _VMAPTOOLS_H

#include "Define.h"
#include <G3D/AABox.h>

/**
The Class is mainly taken from G3D/AABSPTree.h but modified to be able to use our internal data structure.
This is an iterator that helps us analysing the BSP-Trees.
The collision detection is modified to return true, if we are inside an object.
*/

namespace VMAP
{
    template<class TValue>
    class WH_COMMON_API IntersectionCallBack
    {
    public:
        TValue*      closestEntity;
        G3D::Vector3 hitLocation;
        G3D::Vector3 hitNormal;

        void operator()(const G3D::Ray& ray, const TValue* entity, bool StopAtFirstHit, float& distance)
        {
            entity->intersect(ray, distance, StopAtFirstHit, hitLocation, hitNormal);
        }
    };

    //==============================================================
    //==============================================================
    //==============================================================

    class WH_COMMON_API MyCollisionDetection
    {
    public:
        static bool collisionLocationForMovingPointFixedAABox(
            const G3D::Vector3&     origin,
            const G3D::Vector3&     dir,
            const G3D::AABox&       box,
            G3D::Vector3&           location,
            bool&                   Inside)
        {
            // Integer representation of a floating-point value.
#define IR(x)   (reinterpret_cast<uint32 const&>(x))

            Inside = true;
            const G3D::Vector3& MinB = box.low();
            const G3D::Vector3& MaxB = box.high();
            G3D::Vector3 MaxT(-1.0f, -1.0f, -1.0f);

            // Find candidate planes.
            for (int i = 0; i < 3; ++i)
            {
                if (origin[i] < MinB[i])
                {
                    location[i] = MinB[i];
                    Inside      = false;

                    // Calculate T distances to candidate planes
                    if (IR(dir[i]))
                    {
                        MaxT[i] = (MinB[i] - origin[i]) / dir[i];
                    }
                }
                else if (origin[i] > MaxB[i])
                {
                    location[i] = MaxB[i];
                    Inside      = false;

                    // Calculate T distances to candidate planes
                    if (IR(dir[i]))
                    {
                        MaxT[i] = (MaxB[i] - origin[i]) / dir[i];
                    }
                }
            }

            if (Inside)
            {
                // definite hit
                location = origin;
                return true;
            }

            // Get largest of the maxT's for final choice of intersection
            int WhichPlane = 0;
            if (MaxT[1] > MaxT[WhichPlane])
            {
                WhichPlane = 1;
            }

            if (MaxT[2] > MaxT[WhichPlane])
            {
                WhichPlane = 2;
            }

            // Check final candidate actually inside box
            if (IR(MaxT[WhichPlane]) & 0x80000000)
            {
                // Miss the box
                return false;
            }

            for (int i = 0; i < 3; ++i)
            {
                if (i != WhichPlane)
                {
                    location[i] = origin[i] + MaxT[WhichPlane] * dir[i];
                    if ((location[i] < MinB[i]) ||
                            (location[i] > MaxB[i]))
                    {
                        // On this plane we're outside the box extents, so
                        // we miss the box
                        return false;
                    }
                }
            }
            /*
            // Choose the normal to be the plane normal facing into the ray
            normal = G3D::Vector3::zero();
            normal[WhichPlane] = (dir[WhichPlane] > 0) ? -1.0 : 1.0;
            */
            return true;

#undef IR
        }
    };
}
#endif
