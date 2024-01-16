#pragma once
#include "GameObject.h"
#include "PlayerObject.h"

namespace NCL
{
	namespace CSC8503
	{
		class SpawnGoal : public GameObject
		{
		public:
			SpawnGoal(int networkID)
				:GameObject("",zone,false)
			{
				networkID += 100000;
				networkObject = new NetworkObject(*this, networkID);
			}

			void OnCollisionBegin(GameObject* otherObject) override
			{
				if (otherObject->GetCollisionLayer() & player)
				{
					PlayerObject* p = (PlayerObject*)otherObject;
					if (p->GetScore() == 2)
						p->SetWin(true);
				}
			}
		};
	}
}
