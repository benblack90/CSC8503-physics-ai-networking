#pragma once
#include "GameObject.h"
#include "NavigationGrid.h"
#include "NavigationPath.h"
#include "PlayerObject.h"
#include "BehaviourSelector.h"
#include "GameWorld.h"

namespace NCL
{
	namespace CSC8503
	{
		class EvilGoose : public GameObject
		{
		public:
			EvilGoose(PlayerObject* p1, PlayerObject* p2, NavigationGrid* navG, GameWorld* g, int networkID);
			~EvilGoose();

			void Update(float dt);
			void SetupBehaviourTree();
			void ExecuteBehaviourTree(float dt);
			void OnCollisionBegin(GameObject* otherObject) override;
			bool GetLockedExit() { return lockedExit; }
			bool GetSpawnedExitCams() { return spawnedExitCams; }
			bool GetSpawnedMazeCams() { return spawnedMazeCams; }
			
		protected:
			bool NavigateToPoint(Vector3 dest, NavigationPath& path);
			void FollowWaypoints();

			float p1Dist;
			float p2Dist;
			bool lockedExit;
			bool spawnedExitCams;
			bool spawnedMazeCams;
			PlayerObject* players[2];
			NavigationGrid* navG;
			BehaviourSelector* root = new BehaviourSelector("root");
			GameWorld* g;
			NavigationPath path;
			Vector3 currentWaypoint;
			Vector3 destVec;
		};
	}
}

