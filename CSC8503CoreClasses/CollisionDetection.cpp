#include "CollisionDetection.h"
#include "CollisionVolume.h"
#include "AABBVolume.h"
#include "OBBVolume.h"
#include "SphereVolume.h"
#include "Window.h"
#include "Maths.h"
#include "Debug.h"

using namespace NCL;

bool CollisionDetection::RayPlaneIntersection(const Ray& r, const Plane& p, RayCollision& collisions) {
	float ln = Vector3::Dot(p.GetNormal(), r.GetDirection());

	if (ln == 0.0f) {
		return false; //direction vectors are perpendicular!
	}

	Vector3 planePoint = p.GetPointOnPlane();

	Vector3 pointDir = planePoint - r.GetPosition();

	float d = Vector3::Dot(pointDir, p.GetNormal()) / ln;

	collisions.collidedAt = r.GetPosition() + (r.GetDirection() * d);

	return true;
}

bool CollisionDetection::RayIntersection(const Ray& r, GameObject& object, RayCollision& collision) {
	bool hasCollided = false;

	const Transform& worldTransform = object.GetTransform();
	const CollisionVolume* volume = object.GetBoundingVolume();

	if (!volume) {
		return false;
	}

	switch (volume->type) {
	case VolumeType::AABB:		hasCollided = RayAABBIntersection(r, worldTransform, (const AABBVolume&)*volume, collision); break;
	case VolumeType::OBB:		hasCollided = RayOBBIntersection(r, worldTransform, (const OBBVolume&)*volume, collision); break;
	case VolumeType::Sphere:	hasCollided = RaySphereIntersection(r, worldTransform, (const SphereVolume&)*volume, collision); break;

	case VolumeType::Capsule:	hasCollided = RayCapsuleIntersection(r, worldTransform, (const CapsuleVolume&)*volume, collision); break;
	}

	return hasCollided;
}

bool CollisionDetection::RayBoxIntersection(const Ray& r, const Vector3& boxPos, const Vector3& boxSize, RayCollision& collision) {
	Vector3 boxMin = boxPos - boxSize;
	Vector3 boxMax = boxPos + boxSize;

	Vector3 rayPos = r.GetPosition();
	Vector3 rayDir = r.GetDirection();

	Vector3 tVals(-1, -1, -1);

	for (int i = 0; i < 3; i++)
	{
		//for each axis, check the appropriate (far) side of the box
		if (rayDir[i] > 0)
			tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
		else if (rayDir[i] < 0)
			tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
	}
	float bestT = tVals.GetMaxElement();
	if (bestT < 0.0f)	//check the ray isn't facing the wrong direction
		return false;

	Vector3 intersection = rayPos + (rayDir * bestT);
	const float epsilon = 0.0001f;
	//check against epsilon (not 0) to allow for floating point errors
	for (int i = 0; i < 3; i++)
	{
		if (intersection[i] + epsilon < boxMin[i] ||
			intersection[i] - epsilon > boxMax[i])
			return false;
	}
	collision.collidedAt = intersection;
	collision.rayDistance = bestT;
	return true;
}

bool CollisionDetection::RayAABBIntersection(const Ray& r, const Transform& worldTransform, const AABBVolume& volume, RayCollision& collision) {

	Vector3 boxPos = worldTransform.GetPosition();
	Vector3 boxSize = volume.GetHalfDimensions();
	return RayBoxIntersection(r, boxPos, boxSize, collision);
}

bool CollisionDetection::RayOBBIntersection(const Ray& r, const Transform& worldTransform, const OBBVolume& volume, RayCollision& collision) {

	Quaternion orientation = worldTransform.GetOrientation();
	Vector3 position = worldTransform.GetPosition();

	Matrix3 transform = Matrix3(orientation);
	Matrix3 invTransform = Matrix3(orientation.Conjugate());

	Vector3 localRayPos = r.GetPosition() - position;

	Ray tempRay(invTransform * localRayPos, invTransform * r.GetDirection());

	//the vector call here is creating a vector at the origin: we're testing against an 'AABB' at the origin
	//just cuz it's easier than transforming it.
	bool collided = RayBoxIntersection(tempRay, Vector3(), volume.GetHalfDimensions(), collision);

	if (collided)
	{
		collision.collidedAt = transform * collision.collidedAt + position;
	}
	return collided;
}

bool CollisionDetection::RaySphereIntersection(const Ray& r, const Transform& worldTransform, const SphereVolume& volume, RayCollision& collision) {
	Vector3 spherePos = worldTransform.GetPosition();
	float sphereRadius = volume.GetRadius();

	Vector3 dir = (spherePos - r.GetPosition());
	float sphereProj = Vector3::Dot(dir, r.GetDirection());

	if (sphereProj < 0.0f) //if it's facing the wrong way ...
	{
		return false;
	}

	Vector3 closestPoint = r.GetPosition() + (r.GetDirection() * sphereProj);
	float sphereDist = (closestPoint - spherePos).Length();
	if (sphereDist > sphereRadius) // if it's outside the radius ... 
	{
		return false;
	}
	float offset = sqrt((sphereRadius * sphereRadius) - (sphereDist * sphereDist)); //distance between closest point and edge of the sphere (using pythagoras)

	collision.rayDistance = sphereProj - offset;
	collision.collidedAt = r.GetPosition() + (r.GetDirection() * collision.rayDistance);

	return true;
}

bool CollisionDetection::RayCapsuleIntersection(const Ray& r, const Transform& worldTransform, const CapsuleVolume& volume, RayCollision& collision) {
	return false;
}

bool CollisionDetection::ObjectIntersection(GameObject* a, GameObject* b, CollisionInfo& collisionInfo) {
	const CollisionVolume* volA = a->GetBoundingVolume();
	const CollisionVolume* volB = b->GetBoundingVolume();

	if (!volA || !volB) {
		return false;
	}

	

	collisionInfo.a = a;
	collisionInfo.b = b;

	Transform& transformA = a->GetTransform();
	Transform& transformB = b->GetTransform();

	VolumeType pairType = (VolumeType)((int)volA->type | (int)volB->type);

	//Two AABBs
	if (pairType == VolumeType::AABB) {
		return AABBIntersection((AABBVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}
	//Two Spheres
	if (pairType == VolumeType::Sphere) {
		return SphereIntersection((SphereVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	//Two OBBs
	if (pairType == VolumeType::OBB) {
		return GJK(a, b, collisionInfo);
	}
	//Two Capsules

	//AABB vs Sphere pairs
	if (volA->type == VolumeType::AABB && volB->type == VolumeType::Sphere) {
		return AABBSphereIntersection((AABBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBSphereIntersection((AABBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	//AABB vs OBB
	if (volA->type == VolumeType::AABB && volB->type == VolumeType::OBB)
	{
		return GJK(a, b, collisionInfo);
	}
	if (volA->type == VolumeType::OBB && volB->type == VolumeType::AABB)
	{
		collisionInfo.a = b;
		collisionInfo.b = a;
		return GJK(b, a, collisionInfo);
	}

	//OBB vs sphere pairs
	if (volA->type == VolumeType::OBB && volB->type == VolumeType::Sphere) {
		return OBBSphereIntersection((OBBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::OBB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return OBBSphereIntersection((OBBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	//Capsule vs other interactions
	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::Sphere) {
		return SphereCapsuleIntersection((CapsuleVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Capsule) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return SphereCapsuleIntersection((CapsuleVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::AABB) {
		return AABBCapsuleIntersection((CapsuleVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}
	if (volB->type == VolumeType::Capsule && volA->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBCapsuleIntersection((CapsuleVolume&)*volB, transformB, (AABBVolume&)*volA, transformA, collisionInfo);
	}

	return false;
}

bool CollisionDetection::AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& halfSizeA, const Vector3& halfSizeB) {
	Vector3 delta = posB - posA;
	Vector3 totalSize = halfSizeA + halfSizeB;

	if (abs(delta.x) < totalSize.x &&
		abs(delta.y) < totalSize.y &&
		abs(delta.z) < totalSize.z) {
		return true;
	}
	return false;
}

//AABB/AABB Collisions
bool CollisionDetection::AABBIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxAPos = worldTransformA.GetPosition();
	Vector3 boxBPos = worldTransformB.GetPosition();

	Vector3 boxASize = volumeA.GetHalfDimensions();
	Vector3 boxBSize = volumeB.GetHalfDimensions();

	//test if there's any overlap before doing anything
	bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize);
	if (overlap)
	{
		static const Vector3 faces[6] =
		{
			Vector3(-1, 0, 0), Vector3(1, 0, 0),	//these faces align with the distances below: left, right, down, up, far, near
			Vector3(0, -1, 0), Vector3(0, 1, 0),
			Vector3(0, 0, -1), Vector3(0, 0, 1),
		};

		Vector3 maxA = boxAPos + boxASize;
		Vector3 minA = boxAPos - boxASize;
		Vector3 maxB = boxBPos + boxBSize;
		Vector3 minB = boxBPos - boxBSize;

		float distances[6] =
		{
			(maxB.x - minA.x),	//distance of box b to 'left' of box a
			(maxA.x - minB.x),	//distance of box b to 'right' of box a
			(maxB.y - minA.y),	//distance of box b to 'bottom' of box a
			(maxA.y - minB.y),	//distance of box b to 'top' of box a 
			(maxB.z - minA.z),	//distance of box b to 'far' of box a
			(maxA.z - minB.z)	//distance of box b to 'near' of box a
		};
		float penetration = FLT_MAX;
		Vector3 bestAxis;

		for (int i = 0; i < 6; i++)
		{
			if (distances[i] < penetration)
			{
				//find the smallest penetration value ...
				penetration = distances[i];
				//... and what axis it's on
				bestAxis = faces[i];
			}
		}
		collisionInfo.AddContactPoint(Vector3(), Vector3(), bestAxis, penetration);
		return true;
	}
	return false;
}

//Sphere / Sphere Collision
bool CollisionDetection::SphereIntersection(const SphereVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	float radii = volumeA.GetRadius() + volumeB.GetRadius();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();

	float deltaLength = delta.Length();

	if (deltaLength < radii)
	{
		float penetration = (radii - deltaLength);
		Vector3 normal = delta.Normalised();
		Vector3 localA = normal * volumeA.GetRadius();
		Vector3 localB = -normal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		return true;
	}
	return false;
}

//AABB - Sphere Collision
bool CollisionDetection::AABBSphereIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxSize = volumeA.GetHalfDimensions();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();

	Vector3 closestPointOnBox = Maths::Clamp(delta, -boxSize, boxSize);
	Vector3 localPoint = delta - closestPointOnBox;
	float distance = localPoint.Length();

	if (distance < volumeB.GetRadius())
	{
		Vector3 collisionNormal = localPoint.Normalised();
		float penetration = (volumeB.GetRadius() - distance);

		Vector3 localA = Vector3();
		Vector3 localB = -collisionNormal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
		return true;
	}
	return false;
}

//OBB - Sphere collision
bool  CollisionDetection::OBBSphereIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Matrix3 oBBInvTransform = Matrix3(worldTransformA.GetOrientation().Conjugate());
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	Transform tempSphere;
	tempSphere.SetPosition(oBBInvTransform * delta);
	AABBVolume tempVA(volumeA.GetHalfDimensions());
	Transform aBBBoxAtOrigin;
	if (AABBSphereIntersection(tempVA, aBBBoxAtOrigin, volumeB, tempSphere, collisionInfo))
	{
		collisionInfo.point.localA = worldTransformA.GetOrientation() * collisionInfo.point.localA;
		collisionInfo.point.localB = worldTransformA.GetOrientation() * collisionInfo.point.localB;
		collisionInfo.point.normal = worldTransformA.GetOrientation() * collisionInfo.point.normal;
		collisionInfo.point.normal.Normalise();
		return true;
	}
	return false;
}

/* This function follows the principles laid down in Casey Muratori's excellent 'Implementing GJK - 2006' video (https://www.youtube.com/watch?v=Qupqu1xe7Io, accessed
* Dec '23), which provides an excellent explanation and geometric visualisation of how the algorithm works.
* It also follows the improvements provided by Kevin Moran's implementation of GJK (https://github.com/kevinmoran/GJK/blob/master/GJK.h, accessed Dec '23), which
* streamlines away some of the unnecessary checks in Muratori's original version.
*/
bool CollisionDetection::GJK(GameObject* a, GameObject* b, CollisionInfo& collisionInfo)
{
	//draw a tetrahedron within a Minkowski difference, searching for an encapsulation of the origin
	//if found, return true, but also run EPA to fill in collionInfo with collision point & normal

	MinkVals corners[GJK_MAX];

	Vector3 searchIn = b->GetTransform().GetPosition() - a->GetTransform().GetPosition();
	BuildMinkVals(searchIn, a, b, corners, GJK_C);
	searchIn = -corners[GJK_C].mkw;
	BuildMinkVals(searchIn, a, b, corners, GJK_B);

	//early out if origin not crossed by second point
	if (Vector3::Dot(corners[GJK_B].mkw, searchIn) < 0) return false;

	//if we didn't early out, our newest point, b, must be beyond the origin. Therefore, search in the direction of the origin, perpendicular to BC
	searchIn = Vector3::Cross(Vector3::Cross(corners[GJK_C].mkw - corners[GJK_B].mkw, -corners[GJK_B].mkw), corners[GJK_C].mkw - corners[GJK_B].mkw);
	//if it actually is the origin, though, try skirt round it
	if (searchIn == Vector3(0, 0, 0))
	{
		searchIn = Vector3::Cross(corners[GJK_C].mkw - corners[GJK_B].mkw, Vector3(1, 0, 0));
		if (searchIn == Vector3(0, 0, 0)) searchIn = Vector3::Cross(corners[GJK_C].mkw - corners[GJK_B].mkw, Vector3(0, 0, -1));
	}

	bool triCasePassed = false;
	for (int i = 0; i < 64; i++)
	{
		BuildMinkVals(searchIn, a, b, corners, GJK_A);
		if (Vector3::Dot(corners[GJK_A].mkw, searchIn) < 0) return false;

		if (!triCasePassed)
		{
			triCasePassed = GJKTriangleCase(corners, searchIn);
			continue;
		}

		if (triCasePassed)
		{
			if (GJKTetraCase(corners, searchIn))
			{
				FindClosestFace(a, b, corners, collisionInfo);
				
				return true;
			}
		}

	}
	//ran out of tries: no intersection :(
	return false;
}


/* This function follows the principles laid down in Casey Muratori's brilliant 'Implementing GJK - 2006' video (https://www.youtube.com/watch?v=Qupqu1xe7Io, accessed
* Dec '23), which provides an excellent explanation and geometric visualisation of how the algorithm works.
* It also follows the improvements provided by Kevin Moran's implementation of GJK (https://github.com/kevinmoran/GJK/blob/master/GJK.h, accessed Dec '23), which
* streamlines away some of the unnecessary checks in Muratori's original version.
*/
bool CollisionDetection::GJKTriangleCase(MinkVals* corners, Vector3& searchIn)
{
	Vector3 AB = corners[GJK_B].mkw - corners[GJK_A].mkw;
	Vector3 AC = corners[GJK_C].mkw - corners[GJK_A].mkw;
	Vector3 ABCn = Vector3::Cross(AB, AC);

	if (Vector3::Dot(Vector3::Cross(AB, ABCn), -corners[GJK_A].mkw) > 0)
	{
		//in the direction of AB's normal, so remake triangle searching in that direction, and towards origin
		searchIn = Vector3::Cross(Vector3::Cross(AB, -corners[GJK_A].mkw), AB);
		corners[GJK_C] = corners[GJK_A];
		return false;
	}
	if (Vector3::Dot(Vector3::Cross(ABCn, AC), -corners[GJK_A].mkw) > 0)
	{
		//in the direction of AC's normal, so remake triangle searching in that direction, and towards origin
		searchIn = Vector3::Cross(Vector3::Cross(AC, -corners[GJK_A].mkw), AC);
		corners[GJK_B] = corners[GJK_A];
		return false;
	}

	//if we've made it this far, origin must be above or below the tri, and A's next position will be the spike of the tetrahedron
	if (Vector3::Dot(ABCn, -corners[GJK_A].mkw) > 0)
	{
		searchIn = ABCn;
		corners[GJK_D] = corners[GJK_C];
		corners[GJK_C] = corners[GJK_B];
		corners[GJK_B] = corners[GJK_A];
		return true;
	}

	else
	{
		searchIn = -ABCn;
		corners[GJK_D] = corners[GJK_B];
		corners[GJK_B] = corners[GJK_A];
		return true;
	}
}

/* This function follows the principles laid down in Casey Muratori's excellent 'Implementing GJK - 2006' video (https://www.youtube.com/watch?v=Qupqu1xe7Io, accessed
* Dec '23), which provides an excellent explanation and geometric visualisation of how the algorithm works.
* It also follows the improvements provided by Kevin Moran's implementation of GJK (https://github.com/kevinmoran/GJK/blob/master/GJK.h, accessed Dec '23), which
* streamlines away some of the unnecessary checks in Muratori's original version.
*/
bool CollisionDetection::GJKTetraCase(MinkVals* corners, Vector3& searchIn)
{
	Vector3 AB = corners[GJK_B].mkw - corners[GJK_A].mkw;
	Vector3 AC = corners[GJK_C].mkw - corners[GJK_A].mkw;
	Vector3 AD = corners[GJK_D].mkw - corners[GJK_A].mkw;
	Vector3 ABCn = Vector3::Cross(AB, AC);
	Vector3 ACDn = Vector3::Cross(AC, AD);
	Vector3 ADBn = Vector3::Cross(AD, AB);

	if (Vector3::Dot(ABCn, -corners[GJK_A].mkw) > 0)
	{
		//preserve winding, and bring 'D' forward along the ABCn normal as the new A
		searchIn = ABCn;
		corners[GJK_D] = corners[GJK_C];
		corners[GJK_C] = corners[GJK_B];
		corners[GJK_B] = corners[GJK_A];
		return false;
	}

	if (Vector3::Dot(ACDn, -corners[GJK_A].mkw) > 0)
	{
		//preserve winding, and bring 'B' forward along the ACDn normal as the new A
		searchIn = ACDn;
		corners[GJK_B] = corners[GJK_A];
		return false;
	}
	if (Vector3::Dot(ADBn, -corners[GJK_A].mkw) > 0)
	{
		//preserve winding, and bring 'C' forward along the ADBn normal as the new A
		searchIn = ADBn;
		corners[GJK_C] = corners[GJK_D];
		corners[GJK_D] = corners[GJK_B];
		corners[GJK_B] = corners[GJK_A];
		return false;
	}

	//if we aren't outside a face, we must be enclosed in the tetrahedron! Victory!
	return true;
}

void CollisionDetection::BuildMinkVals(Vector3 searchIn, GameObject* a, GameObject* b, MinkVals* mv, GJK_Points gjk_ind)
{
	mv[gjk_ind].aPos = a->GetBoundingVolume()->Support(-searchIn, a->GetTransform());
	mv[gjk_ind].bPos = b->GetBoundingVolume()->Support(searchIn, b->GetTransform());
	mv[gjk_ind].mkw = mv[gjk_ind].bPos - mv[gjk_ind].aPos;
}

void NCL::CollisionDetection::FindClosestFace(GameObject* a, GameObject* b, MinkVals* corners, CollisionInfo& collisionInfo)
{
	std::vector<Face> faces;
	Face holder;
	holder.a = corners[GJK_A]; holder.b = corners[GJK_B]; holder.c = corners[GJK_C];
	holder.BuildNorm();
	faces.push_back(holder);
	holder.a = corners[GJK_A]; holder.b = corners[GJK_C]; holder.c = corners[GJK_D];
	holder.BuildNorm();
	faces.push_back(holder);
	holder.a = corners[GJK_A]; holder.b = corners[GJK_D]; holder.c = corners[GJK_B];
	holder.BuildNorm();
	faces.push_back(holder);
	holder.a = corners[GJK_B]; holder.b = corners[GJK_D]; holder.c = corners[GJK_C];
	holder.BuildNorm();
	faces.push_back(holder);
	
	int closestFaceIndex = 0;
	float minDist = FLT_MAX;
	for (int i = 0; i < faces.size(); i++)
	{
		float dist = abs(Vector3::Dot(faces[i].a.mkw, faces[i].normal));
		if (dist < minDist)
		{
			minDist = dist;
			closestFaceIndex = i;
		}		
	}
	Plane triPlane;
	triPlane.PlaneFromTri(faces[closestFaceIndex].a.mkw, faces[closestFaceIndex].b.mkw, faces[closestFaceIndex].c.mkw);
	Vector3 projectedOrigin = triPlane.ProjectPointOntoPlane(Vector3(0, 0, 0));
	float areaA = Maths::AreaofTri3D(projectedOrigin, faces[closestFaceIndex].b.mkw, faces[closestFaceIndex].c.mkw);
	float areaB = Maths::AreaofTri3D(projectedOrigin, faces[closestFaceIndex].c.mkw, faces[closestFaceIndex].a.mkw);
	float areaC = Maths::AreaofTri3D(projectedOrigin, faces[closestFaceIndex].a.mkw, faces[closestFaceIndex].b.mkw);
	float totalArea = Maths::AreaofTri3D(faces[closestFaceIndex].a.mkw, faces[closestFaceIndex].b.mkw, faces[closestFaceIndex].c.mkw);
	Vector3 barymetric({ 0,0,0 });
	barymetric.x = areaA / totalArea;
	barymetric.y = areaB / totalArea;
	barymetric.z = areaC / totalArea;
	Vector3 localA = faces[closestFaceIndex].a.aPos * barymetric.x
		+ faces[closestFaceIndex].b.aPos * barymetric.y
		+ faces[closestFaceIndex].c.aPos * barymetric.z;
	Vector3 localB = faces[closestFaceIndex].a.bPos * barymetric.x
		+ faces[closestFaceIndex].b.bPos * barymetric.y
		+ faces[closestFaceIndex].c.bPos * barymetric.z;
	float pen = (localA - localB).Length();
	Vector3 norm = (localA - localB).Normalised();
	localA -= a->GetTransform().GetPosition();
	localB -= b->GetTransform().GetPosition();
	collisionInfo.AddContactPoint(localA, localB, norm, pen);
}


bool CollisionDetection::AABBCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	return false;
}

bool CollisionDetection::SphereCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	return false;
}


Matrix4 GenerateInverseView(const Camera& c) {
	float pitch = c.GetPitch();
	float yaw = c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix4::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Matrix4 GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	float negDepth = nearPlane - farPlane;

	float invNegDepth = negDepth / (2 * (farPlane * nearPlane));

	Matrix4 m;

	float h = 1.0f / tan(fov * PI_OVER_360);

	m.array[0][0] = aspect / h;
	m.array[1][1] = tan(fov * PI_OVER_360);
	m.array[2][2] = 0.0f;

	m.array[2][3] = invNegDepth;//// +PI_OVER_360;
	m.array[3][2] = -1.0f;
	m.array[3][3] = (0.5f / nearPlane) + (0.5f / farPlane);

	return m;
}

Vector3 CollisionDetection::Unproject(const Vector3& screenPos, const PerspectiveCamera& cam) {
	Vector2i screenSize = Window::GetWindow()->GetScreenSize();

	float aspect = Window::GetWindow()->GetScreenAspect();
	float fov = cam.GetFieldOfVision();
	float nearPlane = cam.GetNearPlane();
	float farPlane = cam.GetFarPlane();

	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	Matrix4 proj = cam.BuildProjectionMatrix(aspect);

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
		(screenPos.y / (float)screenSize.y) * 2.0f - 1.0f,
		(screenPos.z),
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetection::BuildRayFromMouse(const PerspectiveCamera& cam) {
	Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
	Vector2i screenSize = Window::GetWindow()->GetScreenSize();

	//We remove the y axis mouse position from height as OpenGL is 'upside down',
	//and thinks the bottom left is the origin, instead of the top left!
	Vector3 nearPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		-0.99999f
	);

	//We also don't use exactly 1.0 (the normalised 'end' of the far plane) as this
	//causes the unproject function to go a bit weird. 
	Vector3 farPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		0.99999f
	);

	Vector3 a = Unproject(nearPos, cam);
	Vector3 b = Unproject(farPos, cam);
	Vector3 c = b - a;

	c.Normalise();

	return Ray(cam.GetPosition(), c);
}

//http://bookofhook.com/mousepick.pdf
Matrix4 CollisionDetection::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	Matrix4 m;

	float t = tan(fov * PI_OVER_360);

	float neg_depth = nearPlane - farPlane;

	const float h = 1.0f / t;

	float c = (farPlane + nearPlane) / neg_depth;
	float e = -1.0f;
	float d = 2.0f * (nearPlane * farPlane) / neg_depth;

	m.array[0][0] = aspect / h;
	m.array[1][1] = tan(fov * PI_OVER_360);
	m.array[2][2] = 0.0f;

	m.array[2][3] = 1.0f / d;

	m.array[3][2] = 1.0f / e;
	m.array[3][3] = -c / (d * e);

	return m;
}

/*
And here's how we generate an inverse view matrix. It's pretty much
an exact inversion of the BuildViewMatrix function of the Camera class!
*/
Matrix4 CollisionDetection::GenerateInverseView(const Camera& c) {
	float pitch = c.GetPitch();
	float yaw = c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(yaw, Vector3(0, 1, 0)) *
		Matrix4::Rotation(pitch, Vector3(1, 0, 0));

	return iview;
}


/*
If you've read through the Deferred Rendering tutorial you should have a pretty
good idea what this function does. It takes a 2D position, such as the mouse
position, and 'unprojects' it, to generate a 3D world space position for it.

Just as we turn a world space position into a clip space position by multiplying
it by the model, view, and projection matrices, we can turn a clip space
position back to a 3D position by multiply it by the INVERSE of the
view projection matrix (the model matrix has already been assumed to have
'transformed' the 2D point). As has been mentioned a few times, inverting a
matrix is not a nice operation, either to understand or code. But! We can cheat
the inversion process again, just like we do when we create a view matrix using
the camera.

So, to form the inverted matrix, we need the aspect and fov used to create the
projection matrix of our scene, and the camera used to form the view matrix.

*/
Vector3	CollisionDetection::UnprojectScreenPosition(Vector3 position, float aspect, float fov, const PerspectiveCamera& c) {
	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(c) * GenerateInverseProjection(aspect, fov, c.GetNearPlane(), c.GetFarPlane());


	Vector2i screenSize = Window::GetWindow()->GetScreenSize();

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(position.x / (float)screenSize.x) * 2.0f - 1.0f,
		(position.y / (float)screenSize.y) * 2.0f - 1.0f,
		(position.z) - 1.0f,
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

