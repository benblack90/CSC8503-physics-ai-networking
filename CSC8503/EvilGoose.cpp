#include "EvilGoose.h"
#include "BehaviourAction.h"
#include "BehaviourNode.h"
#include "BehaviourNodeWithChildren.h"
#include "BehaviourSequence.h"
#include "Ray.h"
#include "NetworkObject.h"

using namespace NCL;
using namespace CSC8503;

EvilGoose::EvilGoose(PlayerObject* p1, PlayerObject* p2, NavigationGrid* navG, GameWorld* g, int networkID)
{
	networkID += 10000;
	networkObject = new NetworkObject(*this, networkID);
	players[0] = p1;
	players[1] = p2;
	this->navG = navG;
	this->g = g;
	lockedExit = false;
	spawnedExitCams = false;
	spawnedMazeCams = false;
	SetupBehaviourTree();
}

EvilGoose::~EvilGoose()
{
	delete root;
}

void EvilGoose::Update(float dt)
{
	//using squared distance to save time
	p1Dist = (players[0] != nullptr) ? (GetTransform().GetPosition() - players[0]->GetTransform().GetPosition()).LengthSquared() : FLT_MAX;
	p2Dist = (players[1] != nullptr) ? (GetTransform().GetPosition() - players[1]->GetTransform().GetPosition()).LengthSquared() : FLT_MAX;
	ExecuteBehaviourTree(dt);
}

void EvilGoose::OnCollisionBegin(GameObject* otherObject)
{
	if (otherObject->GetName() == "lock")
	{
		otherObject->SetActive(false);
		lockedExit = true;
	}
	if (otherObject->GetName() == "mazeDef")
	{
		otherObject->SetActive(false);
		spawnedMazeCams = true;
	}
	if (otherObject->GetName() == "exitDef")
	{
		otherObject->SetActive(false);
		spawnedExitCams = true;
	}
}


bool EvilGoose::NavigateToPoint(Vector3 dest, NavigationPath& path)
{
	path.Clear();
	return navG->FindPath(GetTransform().GetPosition(), dest, path);
}

void EvilGoose::FollowWaypoints()
{
	//same trick as with the CCTVs: orthogonal vector to forward and up lets us determine how to steer
	Vector3 orth = GetTransform().GetMatrix().GetColumn(0).Normalised();
	Vector3 toP = currentWaypoint - GetTransform().GetPosition();
	float dotProd = Vector3::Dot(orth, toP);

	//apply torque proportional to how far off we are 
	if (dotProd > 0)
	{
		GetPhysicsObject()->AddTorque(Vector3(0, -1, 0) * 2 * dotProd);
	}
	if (dotProd < 0)
	{
		GetPhysicsObject()->AddTorque(Vector3(0, 1, 0) * 2 * -dotProd);
	}

	//apply force if we're kinda close - we don't want to wait for the turn to *fully* complete or it'll look mad
	if (abs(dotProd < 0.3))
		GetPhysicsObject()->AddForce(-GetTransform().GetMatrix().GetColumn(2) * 25);
}

/*
																	 Root selection
																			|
				_____________________________________________________________________________________________
				|									  |														|
		chase player seq					 protect maze seq										Exit defence selector
				|									  |														|
		 ________________				____________________________					____________________________________________
		|		         |				|							|					|											|
nav to player     move to location   nav to maze				 move to location		|											|
																			lock exits sequence						spawn exit cam sequence
																						|											|
																			____________________________							|
																			|							|				____________________
																nav to exit lock trig			move to location		|					|
																													nav to exit				|
																													 cam trig		move to location
*/
void EvilGoose::SetupBehaviourTree()
{
	BehaviourAction* navToMazeCamTrig = new BehaviourAction("navigate to maze cam trigger", [&](float dt, BehaviourState state)->BehaviourState
		{
			//don't try to spawn the maze cams if they've already been spawned
			if (spawnedMazeCams)
				return Failure;

			if (state == Initialise)
			{
				if (NavigateToPoint(navG->GetMazeDefButton(), this->path))
				{
					destVec = navG->GetMazeDefButton();
					return Success;
				}

				else
					return Failure;
			}
			return state;
		}
	);
	BehaviourAction* navToExitLockTrig = new BehaviourAction("navigate to exit lock trigger", [&](float dt, BehaviourState state)->BehaviourState
		{
			//don't try to lock the exit if it's been locked before
			if (lockedExit)
				return Failure;

			if (state == Initialise)
			{
				if (NavigateToPoint(navG->GetLockButton(), this->path))
				{
					destVec = navG->GetLockButton();
					return Success;
				}
				else
					return Failure;
			}
			return state;
		}
	);

	BehaviourAction* navToExitCamTrig = new BehaviourAction("navigate to exit cam trigger", [&](float dt, BehaviourState state)->BehaviourState
		{
			//don't try to spawn the exit cams if that's already been done
			if (spawnedExitCams)
				return Failure;

			if (state == Initialise)
			{
				if (NavigateToPoint(navG->GetExitDefButton(), this->path))
				{
					destVec = navG->GetExitDefButton();
					return Success;
				}
				else
					return Failure;
			}
			return state;
		}
	);
	BehaviourAction* moveToLocation = new BehaviourAction("move to location", [&](float dt, BehaviourState state)->BehaviourState
		{
			//if the player is within 50, forget moving to wherever you're moving
			if (std::min(p1Dist, p2Dist) < 50 * 50)
				return Failure;

			if (state == Initialise)
			{
				path.PopWaypoint(currentWaypoint);
				state = Ongoing;
			}
			else if (state == Ongoing)
			{
				FollowWaypoints();

				if ((GetTransform().GetPosition() - currentWaypoint).LengthSquared() < 25)
					//if we've popped our last waypoint and can pop no more ... set the actual vector3 of the target as a waypoint
					if (!path.PopWaypoint(currentWaypoint))
					{
						currentWaypoint = destVec;
					}
				if ((GetTransform().GetPosition() - currentWaypoint).Length() <= 0.3)
					return Success;
			}
			return state;
		}
	);

	BehaviourAction* navToPlayer = new BehaviourAction("navigate to player", [&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{


				//if over 50 away, don't even bother, unless everything else has been done
				if (std::min(p1Dist, p2Dist) > 50 * 50 && !(spawnedExitCams && spawnedMazeCams && lockedExit))
					return Failure;

				PlayerObject* target;
				if (p1Dist < p2Dist)
					target = players[0];
				else
					target = players[1];

				if (NavigateToPoint(target->GetTransform().GetPosition(), this->path))
				{
					destVec = target->GetTransform().GetPosition();
					return Success;
				}
				else
					return Failure;
			}
			return state;
		}
	);

	BehaviourAction* moveToPlayer = new BehaviourAction("move to player", [&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{
				path.PopWaypoint(currentWaypoint);
				state = Ongoing;
			}
			else if (state == Ongoing)
			{

				//if they get further than 75 distance away, give up (unless there's nothing better to do)
				if ((destVec - GetTransform().GetPosition()).LengthSquared() > 75 * 75 && !(spawnedExitCams && spawnedMazeCams && lockedExit))
					return Failure;

				PlayerObject* target;
				if (p1Dist < p2Dist)
					target = players[0];
				else
					target = players[1];

				//if we found ourselves outside the maze, attempt to beam a line directly at the player, and follow that
				//if we can, we want to save computational power!
				if (GetTransform().GetPosition().z > 0 || GetTransform().GetPosition().x > 0)
				{
					Ray vision(GetTransform().GetPosition(), (target->GetTransform().GetPosition() - GetTransform().GetPosition()).Normalised());
					RayCollision rayColl;
					g->Raycast(vision, rayColl, true, this);
					GameObject* node = (GameObject*) rayColl.node;
					if (node->GetCollisionLayer() & player)
					{
						currentWaypoint = target->GetTransform().GetPosition();
					}
				}
				
				//otherwise, map a path towards them via pathfinding
				else if ((GetTransform().GetPosition() - currentWaypoint).LengthSquared() < 25)
				{
					if (NavigateToPoint(target->GetTransform().GetPosition(), this->path))
					{
						destVec = target->GetTransform().GetPosition();
						path.PopWaypoint(currentWaypoint);
						path.PopWaypoint(currentWaypoint);
					}
					else return Failure;

					if ((GetTransform().GetPosition() - destVec).LengthSquared() < 25)
						currentWaypoint = destVec;
				}
				if ((GetTransform().GetPosition() - currentWaypoint).Length() <= 0.5)
					return Success;

				FollowWaypoints();
			}
			return state;
		}
	);


	BehaviourSequence* lockExits = new BehaviourSequence("lock exits");
	lockExits->AddChild(navToExitLockTrig);
	lockExits->AddChild(moveToLocation);
	BehaviourSequence* spawnExitCams = new BehaviourSequence("spawn exit cams");
	spawnExitCams->AddChild(navToExitCamTrig);
	spawnExitCams->AddChild(moveToLocation);
	BehaviourSelector* exitDefence = new BehaviourSelector("exit defence select");
	exitDefence->AddChild(lockExits);
	exitDefence->AddChild(spawnExitCams);

	BehaviourSequence* protectMaze = new BehaviourSequence("protect maze");
	protectMaze->AddChild(navToMazeCamTrig);
	protectMaze->AddChild(moveToLocation);

	BehaviourSequence* chasePlayer = new BehaviourSequence("chase player");
	chasePlayer->AddChild(navToPlayer);
	chasePlayer->AddChild(moveToPlayer);

	root->AddChild(chasePlayer);
	root->AddChild(protectMaze);
	root->AddChild(exitDefence);

}

void EvilGoose::ExecuteBehaviourTree(float dt)
{
	BehaviourState state = root->Execute(dt);
	if (state == Failure || state == Success)
		root->Reset();

}
