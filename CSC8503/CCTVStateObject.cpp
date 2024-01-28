#pragma once
#include "CCTVStateObject.h"
#include "PlayerObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"
#include "Debug.h"
#include "GameWorld.h"
#include "NetworkObject.h"

using namespace NCL;
using namespace CSC8503;



CCTVStateObject::CCTVStateObject(PlayerObject* p1, PlayerObject* p2, CollisionLayer layer, GameWorld* g, int networkID)
	:Block(layer)
{
	networkID += 10;
	networkObject = new NetworkObject(*this, networkID);
	players[0] = p1;
	players[1] = p2;
	this->g = g;
	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)-> void
		{
			this->SearchRotate(sceneTime);
		}
	);
	State* stateB = new State([&](float dt)-> void
		{
			this->TrackPlayer();
		}
	);

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);

	stateMachine->AddTransition(new StateTransition(stateA, stateB, [&]()->bool
		{
			return trackTarget != nullptr;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateA, [&]()->bool
		{
			return trackTarget == nullptr;
		}
	));
}

CCTVStateObject::~CCTVStateObject() {
	delete stateMachine;
}

void CCTVStateObject::Update(float dt) {



	//only do any of the state machine stuff if the camdera hasn't been knocked off its pole
	if (layer & tempStatic)
	{
		KillIfNoPole();
		sceneTime += dt;

		Vector3 camForward = -GetTransform().GetMatrix().GetColumn(2).Normalised();
		float dotProd;
		for (int i = 0; i < MAX_POSS_PLAYERS; i++)
		{
			if (players[i] != nullptr)
			{
				Vector3 toP = (players[i]->GetTransform().GetPosition() - GetTransform().GetPosition());
				RayCollision rayColl;
				GameObject* closest = RayToPlayer(toP.Normalised(), rayColl);
				dotProd = Vector3::Dot(camForward, toP.Normalised());

				if (dotProd > DOT_PROD_TO_SPOT && toP.Length() < MAX_DIST && closest != nullptr)
				{
					if (closest->GetCollisionLayer() == player)
					{
						SpotPlayer(i, closest, rayColl);
					}
					continue;
				}
				if (players[i] == trackTarget) trackTarget = nullptr;
			}
		}
		stateMachine->Update(dt);
	}
}

void CCTVStateObject::SpotPlayer(int index, GameObject* closest, RayCollision& rayColl)
{
	PlayerObject* closestP = (PlayerObject*)closest;
	closestP->SetSpotted(true);
	Debug::DrawLine(GetTransform().GetPosition(), rayColl.collidedAt, { 1,0,0,1 }, 0.01f);
	if (trackTarget == nullptr) trackTarget = players[index];
}

void CCTVStateObject::KillIfNoPole()
{
	Ray balanceRay(GetTransform().GetPosition(), Vector3(0, -1, 0));
	RayCollision balColl;
	g->Raycast(balanceRay, balColl, true, this);
	GameObject* closest = (GameObject*)balColl.node;
	if (closest->GetCollisionLayer() != tempStatic)
	{
		layer = standard;
		this->physicsObject->useGravity = true;
	}
}

GameObject* CCTVStateObject::RayToPlayer(const Vector3& toPNorm, RayCollision& rayColl)
{
	Ray camRay(GetTransform().GetPosition(), toPNorm);
	g->Raycast(camRay, rayColl, true, this);
	GameObject* closest = (GameObject*)rayColl.node;
	return closest;
}


void CCTVStateObject::SearchRotate(float sceneTime)
{
	GetPhysicsObject()->AddTorque(Vector3(0, 1, 0) * sinf(sceneTime));
}

void CCTVStateObject::TrackPlayer()
{
	if (trackTarget != nullptr)
	{
		Vector3 orth = GetTransform().GetMatrix().GetColumn(0).Normalised();
		Vector3 toP = (trackTarget->GetTransform().GetPosition() - GetTransform().GetPosition()).Normalised();
		toP.y = 0;
		if (Vector3::Dot(orth, toP) > 0.1)
		{
			GetPhysicsObject()->AddTorque(Vector3(0, -1, 0) * 0.75);
		}
		if (Vector3::Dot(orth, toP) < -0.1)
		{
			GetPhysicsObject()->AddTorque(Vector3(0, 1, 0) * 0.75);
		}

	}
}
