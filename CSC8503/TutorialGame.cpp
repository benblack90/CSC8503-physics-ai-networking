#include "TutorialGame.h"
#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"

#include "PositionConstraint.h"
#include "OrientationConstraint.h"
#include "StateGameObject.h"
#include "PushdownMachine.h"
#include "PushdownState.h"




using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame() : controller(*Window::GetWindow()->GetKeyboard(), *Window::GetWindow()->GetMouse()) {
	world = new GameWorld();
#ifdef USEVULKAN
	renderer = new GameTechVulkanRenderer(*world);
	renderer->Init();
	renderer->InitStructures();
#else 
	renderer = new GameTechRenderer(*world);
#endif

	physics = new PhysicsSystem(*world);

	forceMagnitude = 10.0f;
	useGravity = true;
	physics->UseGravity(useGravity);
	inSelectionMode = false;
	paused = false;
	menu = false;
	menuQuit = false;

	world->GetMainCamera().SetController(controller);

	controller.MapAxis(0, "Sidestep");
	controller.MapAxis(1, "UpDown");
	controller.MapAxis(2, "Forward");

	controller.MapAxis(3, "XLook");
	controller.MapAxis(4, "YLook");
	server = false;
	client = false;
	timeToNextPacket = 0.0f;
	packetsToSnapshot = -1;
	Menu(Window::GetWindow()->GetTimer().GetTimeDeltaSeconds(), true);

}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes,
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	cubeMesh = renderer->LoadMesh("cube.msh");
	sphereMesh = renderer->LoadMesh("sphere.msh");
	charMesh = renderer->LoadMesh("goat.msh");
	enemyMesh = renderer->LoadMesh("goose.msh");
	bonusMesh = renderer->LoadMesh("sphere.msh");
	capsuleMesh = renderer->LoadMesh("capsule.msh");

	basicTex = renderer->LoadTexture("checkerboard.png");
	basicShader = renderer->LoadShader("scene.vert", "scene.frag");
	InitCamera();
	InitWorld();

}

TutorialGame::~TutorialGame() {
	delete cubeMesh;
	delete sphereMesh;
	delete charMesh;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;

	delete thisServer;
	delete thisClient;
}

void TutorialGame::StartAsServer()
{
	NetworkBase::Initialise();
	thisServer = new GameServer(NetworkBase::GetDefaultPort(), 1);

	thisServer->RegisterPacketHandler(Received_State, this);
	server = true;

	InitialiseAssets();
}

void TutorialGame::StartAsClient()
{
	NetworkBase::Initialise();

	thisClient = new GameClient();
	clientConnected = thisClient->Connect(127, 0, 0, 1, NetworkBase::GetDefaultPort());

	thisClient->RegisterPacketHandler(Delta_State, this);
	thisClient->RegisterPacketHandler(Full_State, this);
	thisClient->RegisterPacketHandler(Player_Obj, this);
	thisClient->RegisterPacketHandler(Player_Connected, this);
	thisClient->RegisterPacketHandler(Player_Disconnected, this);
	client = true;
	InitialiseAssets();
}
void TutorialGame::UpdateAsServer(float dt) {
	packetsToSnapshot--;
	if (packetsToSnapshot < 0) {
		BroadcastSnapshot(false);
		packetsToSnapshot = 5;
	}
	else {
		BroadcastSnapshot(true);
	}
	if (player[Client]->change == true)
	{
		player[Client]->change == false;
		SendPlayerObjPacket(player[Client]);
	}
}

void TutorialGame::SendPlayerObjPacket(PlayerObject* p)
{
	PlayerObjPacket poPack;
	poPack.score = p->GetScore();
	poPack.spotted = p->GetSpotted();
	poPack.win = p->GetWin();
	poPack.stateID = playerObjPacketID++;
	//this should match on the other end too, because they're assigned the same way for everyone
	poPack.networkID = p->GetNetworkObject()->GetNetworkID();
	thisServer->SendGlobalPacket(poPack);
}

void TutorialGame::BroadcastSnapshot(bool deltaFrame) {

	for (int i = 0; i < networkObjects.size(); i++) {
		if (!networkObjects[i]) {
			continue;
		}
		GamePacket* newPacket = nullptr;
		if (networkObjects[i]->WritePacket(&newPacket, deltaFrame, serversideLastFullID)) {
			thisServer->SendGlobalPacket(*newPacket);
			delete newPacket;
		}
	}
}

void TutorialGame::WriteAndSendClientPacket(float dt) {
	ClientPacket newPacket;
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
		//fire button pressed!
		newPacket.buttonstates = newPacket.buttonstates | SPACE;
	}
	if (Window::GetKeyboard()->KeyDown(KeyCodes::W))
	{	//forward held
		newPacket.buttonstates = newPacket.buttonstates | W;
	}
	if (Window::GetKeyboard()->KeyDown(KeyCodes::S))
	{
		//back held
		newPacket.buttonstates = newPacket.buttonstates | S;
	}

	newPacket.mouseXLook = controller.GetNamedAxis("XLook") * 0.05;
	newPacket.lastID = clientsideLastFullID;	
	thisClient->SendPacket(newPacket);
}

void TutorialGame::ReceivePacket(int type, GamePacket* payload, int source)
{
	if (type == Received_State)
	{
		ClientPacket* clP = (ClientPacket*)payload;
		serversideLastFullID = clP->lastID;
		ProcessClientPacket(clP);
		UpdateMinimumState();
		return;
	}

	if (type == Delta_State || type == Full_State)
	{
		for (int i = 0; i < networkObjects.size(); i++)
		{

			if (!networkObjects[i]) continue;
			networkObjects[i]->ReadPacket(*payload);
		}
		if (type == Full_State)
		{
			FullPacket* fp = (FullPacket*)payload;
			clientsideLastFullID = fp->fullState.stateID;
		}

	}

	if (type == Player_Obj)
	{
		PlayerObjPacket* poPack = (PlayerObjPacket*)payload;
		if (poPack->networkID != player[Client]->GetNetworkObject()->GetNetworkID()
			|| poPack->stateID < clientSideLastPObj)
			return;
		clientSideLastPObj = poPack->stateID;
		player[Client]->SetScore(poPack->score);
		player[Client]->SetSpotted(poPack->spotted);
		player[Client]->SetWin(poPack->win);
	}

}

void TutorialGame::ProcessClientPacket(ClientPacket* clP)
{
	Vector3 fwdAxis = -player[Client]->GetTransform().GetMatrix().GetColumn(2);
	if (clP->buttonstates & W)
	{
		player[Client]->GetPhysicsObject()->AddForce(fwdAxis);
	}
	if (clP->buttonstates & S)
	{
		player[Client]->GetPhysicsObject()->AddForce(-fwdAxis);
	}
	if (clP->buttonstates & SPACE)
	{
		LaunchHittyCube(Client);
	}
	player[Client]->GetPhysicsObject()->AddTorque(Vector3(0, -1, 0) * clP->mouseXLook);
}

void TutorialGame::UpdateMinimumState() {
	//Periodically remove old data from the server
	int minID = INT_MAX;
	int maxID = 0; //we could use this to see if a player is lagging behind?

	for (auto i : stateIDs) {
		minID = std::min(minID, i.second);
		maxID = std::max(maxID, i.second);
	}
	//every client has acknowledged reaching at least state minID
	//so we can get rid of any old states!
	for (int i = 0; i < networkObjects.size(); i++)
	{

		if (!networkObjects[i]) continue;
		networkObjects[i]->UpdateStateHistory(minID); //clear out old states so they arent taking up memory...
	}
}

void TutorialGame::UpdateGame(float dt) {
	timeToNextPacket -= dt;
	world->GetMainCamera().UpdateCamera(dt);
	UpdateKeys();
	LockCamera();
	if (server || singlePlayer)
	{
		if (server)
		{
			thisServer->UpdateServer();
			if (clientConnected) UpdateAsServer(dt);
			if (thisServer->GetClientCount() > 0)
			{
				clientConnected = true;
				player[1]->SetActive(true);
			}
		}

		//check if menu should be displayed - winning or regular
		if (menu || (player[0] != nullptr && player[0]->GetWin()) || (player[1] != nullptr && player[1]->GetWin()))
		{
			Menu(dt, false);
		}
		if (!paused)
		{
			UpdateHittyCube(dt, Server);
			if (clientConnected) UpdateHittyCube(dt, Client);

			for (int i = 0; i < MAX_ENTITIES; i++)
			{
				if (player[i] != nullptr)
				{
					CheckPlayerGravity(i);
					UpdateSpotStatus(i);					
				}
			}
			UpdateCCTVCams(dt);
			UpdateUI(dt);
			UpdateEvilGoose(dt);

			physics->Update(dt);
		}
	}

	if (client)
	{
		thisClient->UpdateClient();
		WriteAndSendClientPacket(dt);
		
		if (menu || (player[0] != nullptr && player[0]->GetWin()) || (player[1] != nullptr && player[1]->GetWin()))
		{
			Menu(dt, false);
		}

		if (!paused)
		{
			UpdateUI(dt);
		}		
	}
	world->UpdateWorld(dt);
	renderer->Update(dt);
	renderer->Render();

	Debug::UpdateRenderables(dt);
}

void TutorialGame::UpdateSpotStatus(int i)
{
	if (player[i]->GetSpotTimer() <= 0)
	{
		int score = player[i]->GetScore();
		for (int i = 0; i < score; i++)
		{
			//if spotted, player drops a heist item, and gets sent back to spawn!
			player[i]->AdjustScore(-1);
			AddBonusToWorld(player[i]->GetTransform().GetPosition() + Vector3(i * 2, 1, 0), "heist")
				->GetRenderObject()->SetColour({ 0,0,1,1 });
		}
		if (i == 0) player[i]->GetTransform().SetPosition(p1Spawn);
		if (i == 1) player[i]->GetTransform().SetPosition(p2Spawn);
		player[i]->SetSpotTimer(5.0f);
	}

	//attempt to un-spot the player ... 
	player[i]->SetSpotted(false);
}

void TutorialGame::UpdateCCTVCams(float dt)
{
	for (int i = 0; i < CCTVStateObject::CamNames::MAX_CAMS; i++)
	{
		if (cam[i] != nullptr)
			cam[i]->Update(dt);
	}
}

void TutorialGame::UpdateEvilGoose(float dt)
{
	if(server || singlePlayer) evG->Update(dt);

	if (evG->GetLockedExit() && !exitLocked)
		LockExit();
	if (evG->GetSpawnedExitCams() && !exitCamsSpawned)
		SpawnExitDefenceCams();
	if (evG->GetSpawnedMazeCams() && !mazeCamsSpawned)
		SpawnMazeDefenceCams();
}

void TutorialGame::CheckPlayerGravity(int i)
{
	if (player[i]->GetTransform().GetPosition().y > 1)
		player[i]->GetPhysicsObject()->useGravity = true;
	else player[i]->GetPhysicsObject()->useGravity = false;
}

void TutorialGame::LockCamera()
{
	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedObject->GetTransform().GetOrientation() * lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera().SetPosition(camPos);
		world->GetMainCamera().SetPitch(angles.x);
		world->GetMainCamera().SetYaw(angles.y);
	}
}

void TutorialGame::UpdateUI(float dt)
{
	char score = '0';
	EntityType e;
	if (server || singlePlayer) e = Server;
	else if (client) e = Client;
	score = player[e]->GetScore() + 48;
	std::string scoreText("Heist items recovered: ");
	scoreText.push_back(score);
	Debug::Print(scoreText, Vector2(50, 5), Debug::WHITE);

	if (player[e]->GetSpotted())
	{
		std::string spotTimeText = std::to_string(player[e]->GetSpotTimer());;
		Debug::Print("SPOTTED!", { 50,50 }, Debug::RED);
		Debug::Print(spotTimeText, { 50,40 }, Debug::RED);
		player[e]->AdjustSpotTimer(dt);
	}
	else player[e]->SetSpotTimer(5.0f);

	if (mazeCamsSpawned)
		Debug::Print("EVIL GOOSE HAS DEPLOYED MAZE CAMS", { 5,10 }, Debug::RED);
	if (exitLocked)
		Debug::Print("EVIL GOOSE HAS BLOCKED THE EXIT", { 5,15 }, Debug::RED);
	if (exitCamsSpawned)
		Debug::Print("EVIL GOOSE HAS DEPLOYED EXIT CAMS", { 5, 20 }, Debug::RED);
}


void TutorialGame::Menu(float dt, bool startUp)
{

	PushdownMachine machine;
	if (startUp)
	{
		machine.SetInitialState(new StartMenu());
	}
	else if (player[Server]->GetWin())
		machine.SetInitialState(new WinnerMenu());
	else if (player[Client]->GetWin())
		machine.SetInitialState(new WinnerMenu());
	else
		machine.SetInitialState(new MainMenu());
	while (Window::GetWindow()->UpdateWindow())
	{
		if (!machine.Update(dt) || machine.messages.message == 's' || machine.messages.message == 'c')
		{
			menu = false;
			paused = false;
			if (machine.messages.message == 'r')
				InitWorld();

			if (machine.messages.message == 'q')
				menuQuit = true;

			if (machine.messages.message == 's')
				StartAsServer();

			if (machine.messages.message == 'c')
				StartAsClient();
			if (machine.messages.message == '1')
			{
				singlePlayer = true;
				InitialiseAssets();
			}


			return;
		}
		renderer->Render();
	}
}

void TutorialGame::UpdateHittyCube(float dt, EntityType type)
{
	reloadTimer[type] += dt;
	if (hittyCube[type]->IsActive())
	{
		if (reloadTimer[type] > 4)
		{
			//reset velocities, to make sure it's not holding onto old velocities
			hittyCube[type]->GetPhysicsObject()->ClearForces();
			hittyCube[type]->GetPhysicsObject()->SetLinearVelocity({ 0,0,0 });
			hittyCube[type]->GetPhysicsObject()->SetAngularVelocity({ 0,0,0 });
			hittyCube[type]->GetTransform().SetPosition({ 0,5000,0 });
			hittyCube[type]->SetActive(false);
		}
		//deal with hitty cube falling victim to a dropped physics cycle: we want to make sure it's definitely moving!
		if (abs(hittyCube[type]->GetPhysicsObject()->GetLinearVelocity().x) < 0.001f &&
			abs(hittyCube[type]->GetPhysicsObject()->GetLinearVelocity().z < 0.001f) &&
			abs(hittyCube[type]->GetPhysicsObject()->GetForce().LengthSquared() < 0.001f))
		{
			Vector3 forward = player[type]->GetTransform().GetOrientation() * Vector3(0, 350, -5000);
			hittyCube[type]->GetPhysicsObject()->AddForce(forward);
		}

	}
}

void TutorialGame::LaunchHittyCube(EntityType type)
{
	if (!hittyCube[type]->IsActive())
	{
		hittyCube[type]->SetActive(true);
		hittyCube[type]->GetPhysicsObject()->useGravity = true;
		hittyCube[type]->GetTransform().SetPosition(player[type]->GetTransform().GetPosition() + player[type]->GetTransform().GetOrientation() * Vector3(0, 2, -3));
		Vector3 forward = player[type]->GetTransform().GetOrientation() * Vector3(0, 0.1, -1) * 5000;
		hittyCube[type]->GetPhysicsObject()->AddForce(forward);
		hittyCube[type]->GetPhysicsObject()->AddTorque({ RandomValue(0,1000),RandomValue(0,1000),RandomValue(0,1000) });
		reloadTimer[type] = 0;
	}
}

int TutorialGame::GenNextNetID()
{
	return lastUsedNetID++;
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F1)) {
		if (server || singlePlayer) InitWorld(); //We can reset the simulation at any time with F1
	}


	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F9)) {
		if (server || singlePlayer) world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F10)) {
		if (server || singlePlayer) world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F7)) {
		if (server || singlePlayer) world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F8)) {
		if (server || singlePlayer) world->ShuffleObjects(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
		if (server || singlePlayer) LaunchHittyCube(Server);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::L)) {
		if (server || singlePlayer)
		{
			if (lockedObject == player[Server])
				lockedObject = evG;
			else lockedObject = player[Server];
		}
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::P))
	{
		if (server || singlePlayer)
		{
			paused = !paused;
			menu = !menu;
		}
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view = world->GetMainCamera().BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();


	if (Window::GetKeyboard()->KeyDown(KeyCodes::UP) || Window::GetKeyboard()->KeyDown(KeyCodes::W)) {
		if (server || singlePlayer) selectionObject->GetPhysicsObject()->AddForce(fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::DOWN) || Window::GetKeyboard()->KeyDown(KeyCodes::S)) {
		if (server || singlePlayer) selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
	}
	if (server || singlePlayer) selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -1, 0) * controller.GetNamedAxis("XLook") * 0.05);
}

void TutorialGame::InitCamera() {
	world->GetMainCamera().SetNearPlane(0.1f);
	world->GetMainCamera().SetFarPlane(500.0f);
	world->GetMainCamera().SetPitch(-15.0f);
	world->GetMainCamera().SetYaw(315.0f);
	world->GetMainCamera().SetPosition(Vector3(-60, 40, 60));
	//lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	player[Server] = AddPlayerToWorld(p1Spawn, "player1");
	player[Server]->GetRenderObject()->SetColour({ 0.2,1,0.5,1 });
	player[Client] = AddPlayerToWorld(p2Spawn, "player2");
	player[Client]->GetRenderObject()->SetColour({ 1,0.6,0.8,1 });
	if (!clientConnected) player[Client]->SetActive(false);
	EntityType e;
	if (server || singlePlayer)
		e = Server;
	if (client) e = Client;
	lockedObject = player[e];
	selectionObject = player[e];

	exitLocked = false;
	mazeCamsSpawned = false;
	exitCamsSpawned = false;
	hittyCubeSize = Vector3(0.5, 0.5, 0.5);
	for (short i = 0; i < MAX_ENTITIES; i++)
	{
		hittyCube[i] = AddOBBCubeToWorld(player[i]->GetTransform().GetPosition() + Vector3(0, 2, -1), hittyCubeSize, 3.0f, standard);
		hittyCube[i]->GetRenderObject()->SetColour({ 1,0,0,1 });
		//hitty cubes get IDs in the 100s to keep them together
		hittyCube[i]->SetNetworkObject(new NetworkObject(*hittyCube[i], GenNextNetID() + 100));
		networkObjects.push_back(hittyCube[i]->GetNetworkObject());
		hittyCube[i]->SetActive(false);
	}

	hittyCubeAABB = new AABBVolume(hittyCubeSize);
	hittyCubeOBB = new OBBVolume(hittyCubeSize);

	InitDefaultFloor();
	InitCubeHouse(3, 3, Vector3(70, 0, -70), Vector3(2, 2, 2));
	InitCubeHouse(3, 3, Vector3(-70, 0, 70), Vector3(2, 2, 2));
	InitMaze(Vector3(-100, 1, -100));
	InitSpawnGoal();
	cam[CCTVStateObject::CamNames::First] = AddCameraPoleToWorld({ -80,0,30 }, 270, 0);
	evG = AddEnemyToWorld(Vector3(-70, 0, -70));

}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();
	Vector3 floorSize = Vector3(100, 0.5f, 100);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, 0, basicShader));
	floor->GetRenderObject()->SetColour({ 0.99,0.62,0.01,1 });
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();
	floor->SetCollisionLayer(staticObj);

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple'
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

Block* TutorialGame::AddAABBCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass, CollisionLayer layer) {
	Block* cube = new Block(GenNextNetID(), layer);
	networkObjects.push_back(cube->GetNetworkObject());

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->GetRenderObject()->SetColour({ 1,1,1,1 });
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddStaticAABBCubeToWorld(const Vector3& position, Vector3 dimensions)
{
	GameObject* cube = new GameObject();
	cube->SetCollisionLayer(staticObj);

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(0);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}


Block* TutorialGame::AddOBBCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass, CollisionLayer layer) {
	Block* cube = new Block(GenNextNetID(), layer);
	networkObjects.push_back(cube->GetNetworkObject());
	OBBVolume* volume = new OBBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

PlayerObject* TutorialGame::AddPlayerToWorld(const Vector3& position, std::string name) {
	float meshSize = 1.0f;
	float inverseMass = 20.0f;

	PlayerObject* character = new PlayerObject(name, GenNextNetID());
	networkObjects.push_back(character->GetNetworkObject());
	SphereVolume* volume = new SphereVolume(1.0f);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), charMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();
	character->GetPhysicsObject()->useGravity = false;

	//set so the player character doesn't constantly spin out of control
	character->GetPhysicsObject()->SetFrameAngularDampingCoeff(2.0f);
	character->GetPhysicsObject()->SetFrameLinearDampingCoeff(1.0f);

	world->AddGameObject(character);

	return character;
}

EvilGoose* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize = 1.0f;
	float inverseMass = 0.5f;

	EvilGoose* character = new EvilGoose(player[0], player[1], navGrid, world, GenNextNetID());
	networkObjects.push_back(character->GetNetworkObject());

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->GetRenderObject()->SetColour({ 1,0,0,1 });
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();
	character->GetPhysicsObject()->SetFrameAngularDampingCoeff(2.0f);
	character->GetPhysicsObject()->SetFrameLinearDampingCoeff(3.0f);

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position, std::string name) {
	GameObject* bonus = new GameObject(name, collectable);

	SphereVolume* volume = new SphereVolume(0.5f);
	bonus->SetBoundingVolume((CollisionVolume*)volume);
	bonus->GetTransform()
		.SetScale(Vector3(1, 1, 1))
		.SetPosition(position);

	bonus->SetRenderObject(new RenderObject(&bonus->GetTransform(), bonusMesh, nullptr, basicShader));
	bonus->SetPhysicsObject(new PhysicsObject(&bonus->GetTransform(), bonus->GetBoundingVolume()));
	bonus->GetPhysicsObject()->SetInverseMass(1.0f);
	bonus->GetPhysicsObject()->InitSphereInertia();
	bonus->GetPhysicsObject()->useGravity = false;
	bonus->SetNetworkObject(new NetworkObject(*bonus, GenNextNetID() + 100000));
	networkObjects.push_back(bonus->GetNetworkObject());
	world->AddGameObject(bonus);

	return bonus;
}

void TutorialGame::LockExit()
{
	InitCubeWall(10, 3, p1Spawn + Vector3(-10, 0, -12), { 1,1,1 }, true);
	exitLocked = true;
}


void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -0.5, 0));
}


void TutorialGame::InitCubeWall(int numHoriz, int numVert, const Vector3& bottomLeft, const Vector3& cubeDims, bool xDir)
{
	for (int y = 0; y < numVert; y++)
	{
		for (int xz = 0; xz < numHoriz; xz++)
		{
			Vector3 position = (xDir) ? Vector3(bottomLeft.x + xz * cubeDims.x * 2, bottomLeft.y + y * cubeDims.y * 2, bottomLeft.z) :
				Vector3(bottomLeft.x, bottomLeft.y + y * cubeDims.y * 2, bottomLeft.z + xz * cubeDims.z * 2);
			AddAABBCubeToWorld(position, cubeDims, 5.0f);
		}
	}
}

void TutorialGame::InitCubeHouse(int numHoriz, int numVert, const Vector3& bottomLeft, const Vector3& cubeDims)
{
	InitCubeWall(numHoriz, numVert, bottomLeft, cubeDims, true);
	InitCubeWall(numHoriz, numVert, Vector3(bottomLeft.x - cubeDims.x * 2, bottomLeft.y, bottomLeft.z), cubeDims, false);
	InitCubeWall(numHoriz, numVert, Vector3(bottomLeft.x + numHoriz * cubeDims.x * 2, bottomLeft.y, bottomLeft.z), cubeDims, false);
	InitCubeWall(numHoriz + 2, numVert, Vector3(bottomLeft.x - cubeDims.x * 2, bottomLeft.y, bottomLeft.z + numHoriz * cubeDims.z * 2), cubeDims, true);
	AddBonusToWorld(Vector3(bottomLeft.x + ((numHoriz / 2) * cubeDims.x * 2), bottomLeft.y + 1, bottomLeft.z + ((numHoriz / 2) * cubeDims.z * 2)), "heist")
		->GetRenderObject()->SetColour({ 0,0,1,1 });
}


CCTVStateObject* TutorialGame::AddCameraPoleToWorld(const Vector3& position, float yaw, float pitch)
{
	AddOBBCubeToWorld(position, { 1,20,1 });

	CCTVStateObject* cam = new CCTVStateObject(player[0], player[1], tempStatic, world, GenNextNetID());
	networkObjects.push_back(cam->GetNetworkObject());
	OBBVolume* volume = new OBBVolume({ 1,1,3 });
	cam->SetBoundingVolume((CollisionVolume*)volume);

	cam->GetTransform()
		.SetPosition(position + Vector3(0, 21, 0))
		.SetScale(Vector3(1, 1, 3) * 2);

	cam->SetRenderObject(new RenderObject(&cam->GetTransform(), cubeMesh, basicTex, basicShader));
	cam->SetPhysicsObject(new PhysicsObject(&cam->GetTransform(), cam->GetBoundingVolume()));

	cam->GetPhysicsObject()->SetInverseMass(10);
	cam->GetPhysicsObject()->InitCubeInertia();
	cam->GetPhysicsObject()->useGravity = false;
	world->AddGameObject(cam);
	Quaternion angle = Quaternion::EulerAnglesToQuaternion(pitch, yaw, 0);
	cam->GetTransform().SetOrientation(angle);
	cam->GetPhysicsObject()->SetFrameAngularDampingCoeff(2.0f);


	return cam;

}

void TutorialGame::SpawnExitDefenceCams()
{
	cam[CCTVStateObject::CamNames::Exit1] = AddCameraPoleToWorld(p1Spawn + Vector3(-15, 0, 0), 0, 0);
	cam[CCTVStateObject::CamNames::Exit2] = AddCameraPoleToWorld(p1Spawn + Vector3(15, 0, 0), 0, 0);
	exitCamsSpawned = true;
}
void TutorialGame::SpawnMazeDefenceCams()
{
	cam[CCTVStateObject::CamNames::Maze1] = AddCameraPoleToWorld({ -95,0,-50 }, 270, 0);
	cam[CCTVStateObject::CamNames::Maze2] = AddCameraPoleToWorld({ 0,0,-50 }, 90, 0);
	mazeCamsSpawned = true;
}
void TutorialGame::InitMaze(Vector3 topLeft)
{
	navGrid = new NavigationGrid("maze.txt", topLeft);
	GridNode* nodes = navGrid->GetAllNodes();
	for (int i = 0; i < navGrid->GetGridWidth() * navGrid->GetGridHeight(); i++)
	{
		if (nodes[i].type == 'x')
		{
			AddStaticAABBCubeToWorld(nodes[i].position, Vector3((float)navGrid->GetNodeSize() / 2, 2, (float)navGrid->GetNodeSize() / 2))
				->GetRenderObject()->SetColour({ 0,1,0,1 });
		}
		if (nodes[i].type == 'L')
			AddBonusToWorld(nodes[i].position + Vector3{ 0,1,0 }, "lock")->GetRenderObject()->SetColour({ 1,0,1,1 });
		if (nodes[i].type == 'M')
			AddBonusToWorld(nodes[i].position + Vector3{ 0,1,0 }, "mazeDef")->GetRenderObject()->SetColour({ 1,0,1,1 });
		if (nodes[i].type == 'E')
			AddBonusToWorld(nodes[i].position + Vector3{ 0,1,0 }, "exitDef")->GetRenderObject()->SetColour({ 1,0,1,1 });
	}

}


void TutorialGame::InitSpawnGoal()
{
	SpawnGoal* sg = new SpawnGoal(GenNextNetID());
	networkObjects.push_back(sg->GetNetworkObject());
	AABBVolume* volume = new AABBVolume({ 10,1,10 });
	sg->SetBoundingVolume((CollisionVolume*)volume);

	sg->GetTransform()
		.SetPosition(p1Spawn)
		.SetScale(volume->GetHalfDimensions() * 2);

	sg->SetPhysicsObject(new PhysicsObject(&sg->GetTransform(), sg->GetBoundingVolume()));

	sg->GetPhysicsObject()->SetInverseMass(0);
	sg->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(sg);

	GameObject* b = AddStaticAABBCubeToWorld(Vector3(p1Spawn.x - volume->GetHalfDimensions().x, p1Spawn.y, p1Spawn.z), { 1,1,10 });
	b = AddStaticAABBCubeToWorld(Vector3(p1Spawn.x + volume->GetHalfDimensions().x, p1Spawn.y, p1Spawn.z), { 1,1,10 });
	b = AddStaticAABBCubeToWorld(Vector3(p1Spawn.x, p1Spawn.y, p1Spawn.z + volume->GetHalfDimensions().z), { 10,1,1 });
}
