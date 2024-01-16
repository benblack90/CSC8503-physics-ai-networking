#pragma once
#include "Block.h"



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
            
            void SearchRotate(float sceneTime, float dt);
            void TrackPlayer();

            StateMachine* stateMachine;
            PlayerObject* trackTarget;
            PlayerObject* players[2];
            GameWorld* g;
            
            float sceneTime;
            
            
        };
    }
}
