#pragma once
#include "Block.h"
#include "Ray.h"



namespace NCL 
{
    namespace CSC8503 
    {
        class StateMachine;
        class PlayerObject;
        class GameWorld;
        class CCTVStateObject : public Block 
        {
        public:
            enum CamNames
            {
                First,
                Exit1,
                Exit2,
                Maze1,
                Maze2,
                MAX_CAMS
            };
            CCTVStateObject(PlayerObject* p1, PlayerObject* p2, CollisionLayer layer, GameWorld* g, int networkID);
            ~CCTVStateObject();

            void Update(float dt);

        protected:
            
            void SearchRotate(float sceneTime);
            void TrackPlayer();
            void KillIfNoPole();
            void SpotPlayer(int index, GameObject* closest, RayCollision& rayColl);
            GameObject* RayToPlayer(const Vector3& toPNorm, RayCollision& rayColl);
            const int MAX_POSS_PLAYERS = 2;
            const float DOT_PROD_TO_SPOT = 0.9f;
            const int MAX_DIST = 150;

            StateMachine* stateMachine;
            PlayerObject* trackTarget;
            PlayerObject* players[2];
            GameWorld* g;
            
            float sceneTime;
            
            
        };
    }
}
