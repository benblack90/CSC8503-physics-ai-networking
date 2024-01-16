#pragma once
#include "CollisionVolume.h"
#include "Vector3.h"

namespace NCL {
	using namespace NCL::Maths;
	class AABBVolume : CollisionVolume
	{
	public:
		AABBVolume(const Vector3& halfDims) {
			type		= VolumeType::AABB;
			halfSizes	= halfDims;
		}
		~AABBVolume() {

		}

		Vector3 GetHalfDimensions() const {
			return halfSizes;
		}

		Maths::Vector3 Support(const Maths::Vector3& dir, const NCL::CSC8503::Transform& tr) const override
		{
			Vector3 localDir = tr.GetOrientation().Conjugate() * dir;
			Vector3 support;
			if (localDir.x > 0) support.x = halfSizes.x; else support.x = -halfSizes.x;
			if (localDir.y > 0) support.y = halfSizes.y; else support.y = -halfSizes.y;
			if (localDir.z > 0) support.z = halfSizes.z; else support.z = -halfSizes.z;
			support = tr.GetOrientation() * support + tr.GetPosition();
			return support;

		}

	protected:
		Vector3 halfSizes;
	};
}
