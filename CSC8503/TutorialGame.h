#include "../NCLCoreClasses/KeyboardMouseController.h"

#pragma once
#include "GameTechRenderer.h"
#ifdef USEVULKAN
#include "GameTechVulkanRenderer.h"
#endif
#include "PhysicsSystem.h"

#include "StateGameObject.h"
#include "PlayerObject.h"
#include "Block.h"
#include "CCTVStateObject.h"
#include "SpawnGoal.h"
#include "NavigationGrid.h"
#include "Menu.h"
#include "EvilGoose.h"
#include "NetworkBase.h"
#include "NetworkObject.h"
#include "NetworkPlayer.h"
#include "NetworkState.h"
#include "GameServer.h"
#include "GameClient.h"

namespace NCL {
	namespace CSC8503 {
		class TutorialGame	: public PacketReceiver	{
		public:

			enum EntityType
			{
				Server,
				Client,
				MAX_ENTITIES
			};

			enum PacketCodes
			{
				SPACE = 1,
				W = 2,
				S = 4,
			};

			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);
			bool GetMenuQuit() { return menuQuit; }
		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();
			

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/

			void InitCubeWall(int numHoriz, int numVert, const Vector3& bottomLeft, const Vector3& cubeDims, bool xDir);
			void InitCubeHouse(int numHoriz, int numVert, const Vector3& bottomLeft, const Vector3& cubeDims);
			void InitSpawnGoal();
			void InitDefaultFloor();
			void InitMaze(Vector3 topLeft);
			
			void SpawnExitDefenceCams();
			void SpawnMazeDefenceCams();
			void LockExit();
			void UpdateSpotStatus(int i);
			void UpdateCCTVCams(float dt);
			void UpdateEvilGoose(float dt);

			void LockCamera();
			void LockedObjectMovement();
			void CheckPlayerGravity(int i);
			void UpdateHittyCube(float dt, EntityType type);
			void UpdateUI(float dt);			
			void Menu(float dt, bool startUp);
			void LaunchHittyCube(EntityType type);

			void StartAsServer();
			void StartAsClient();
			void UpdateAsServer(float dt);
			void WriteAndSendClientPacket(float dt);
			void BroadcastSnapshot(bool deltaFrame);
			int GenNextNetID();
			void UpdateMinimumState();
			void ReceivePacket(int type, GamePacket* payload, int source) override;
			void ProcessClientPacket(ClientPacket* clP);
			void SendPlayerObjPacket(PlayerObject* p);
		
			
			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddStaticAABBCubeToWorld(const Vector3& position, Vector3 dimensions);
			Block* AddAABBCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, CollisionLayer = tempStatic);
			Block* AddOBBCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, CollisionLayer = tempStatic);
			PlayerObject* AddPlayerToWorld(const Vector3& position, std::string name);
			EvilGoose* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position, std::string name);
			CCTVStateObject* AddCameraPoleToWorld(const Vector3& position, float yaw, float pitch);

#ifdef USEVULKAN
			GameTechVulkanRenderer*	renderer;
#else
			GameTechRenderer* renderer;
#endif
			PhysicsSystem*		physics;
			GameWorld*			world;

			KeyboardMouseController controller;

			bool useGravity;
			bool inSelectionMode;
			bool paused;
			bool menu;
			bool menuQuit;
			bool clientConnected;
			bool server;
			bool client;
			bool singlePlayer;
			bool exitLocked;
			bool mazeCamsSpawned;
			bool exitCamsSpawned;
			int lastUsedNetID;
			int playerObjPacketID = 0;
			int clientSideLastPObj = 0;
			float		forceMagnitude;
			float reloadTimer[MAX_ENTITIES];
			float timeToNextPacket;
			int packetsToSnapshot;
			int clientsideLastFullID = 1;
			int serversideLastFullID = 1;
			GameObject* selectionObject = nullptr;

			//network structures
			std::map<int, int> stateIDs;
			std::vector<NetworkObject*> networkObjects;

			Mesh*	capsuleMesh = nullptr;
			Mesh*	cubeMesh	= nullptr;
			Mesh*	sphereMesh	= nullptr;

			Texture*	basicTex	= nullptr;
			Shader*		basicShader = nullptr;

			//Coursework Meshes
			Mesh*	charMesh	= nullptr;
			Mesh*	enemyMesh	= nullptr;
			Mesh*	bonusMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 3, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}
			PlayerObject* player[MAX_ENTITIES];
			CCTVStateObject* cam[CCTVStateObject::CamNames::MAX_CAMS];
			EvilGoose* evG;

			GameObject* objClosest = nullptr;
			GameObject* hittyCube[MAX_ENTITIES];
			
			Vector3 hittyCubeSize;
			AABBVolume* hittyCubeAABB;
			OBBVolume* hittyCubeOBB;
			NavigationGrid* navGrid;
			GameServer* thisServer = nullptr;
			GameClient* thisClient = nullptr;

			Vector3 p1Spawn= Vector3(50, 1, 70);
			Vector3 p2Spawn = Vector3(55, 1, 70);
		};
	}
}

