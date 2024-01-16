#pragma once

namespace NCL {
	using namespace Maths;
	namespace CSC8503 {
		//class GameObject;
		class NetworkState	{
		public:
			NetworkState();
			virtual ~NetworkState();

			Vector3		position;
			Quaternion	orientation;
			char		score;
			bool		isActive;
			bool		spotted;
			int			stateID;
		};
	}
}

