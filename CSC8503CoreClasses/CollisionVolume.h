#pragma once
#include "Transform.h"

namespace NCL {
	enum class VolumeType {
		AABB	= 1,
		OBB		= 2,
		Sphere	= 4, 
		Mesh	= 8,
		Capsule = 16,
		Compound= 32,
		Invalid = 256
	};

	class CollisionVolume
	{
	public:
		CollisionVolume() {
			type = VolumeType::Invalid;
		}
		~CollisionVolume() {}

		virtual Maths::Vector3 Support(const Maths::Vector3& dir, const NCL::CSC8503::Transform& tr) const { return Vector3(); }

		VolumeType type;
	};
}