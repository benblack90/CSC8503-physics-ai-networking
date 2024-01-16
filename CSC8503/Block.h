#pragma once
#include "GameObject.h"
#include "string"
#include "NetworkObject.h"

namespace NCL
{
	namespace CSC8503
	{
		class Block : public GameObject
		{
		public:

			Block(int networkID, CollisionLayer layer = tempStatic)
				:GameObject("", layer, false)
			{
				//keeping objects of the same type together: blocks are in the 1000s
				networkID += 1000;
				networkObject = new NetworkObject(*this, networkID);
			}
			/*Blocks begin the game temporarily static - when they collide with a non-static object (i.e. the player or a hitty cube,
			they suddenly change layers and start appreciating gravity. This saves us some computational power*/
			void OnCollisionBegin(GameObject* otherObject) override
			{
				if (layer & tempStatic)
				{
					layer = standard;
					this->physicsObject->useGravity = true;
				}
				
			}

		};
	}
}