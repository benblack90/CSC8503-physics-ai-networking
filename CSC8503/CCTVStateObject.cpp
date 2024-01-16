#pragma once
#include "CCTVStateObject.h"
#include "PlayerObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"
#include "Debug.h"
#include "Ray.h"
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
			this->SearchRotate(sceneTime, dt);
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
		//eliminate the slight possiblity our pole has been knocked out from under us, and somehow hasn't hit us
		Ray balanceRay(GetTransform().GetPosition(), Vector3(0, -1, 0));
		RayCollision balColl;
		g->Raycast(balanceRay, balColl, true, this);
		GameObject* closest = (GameObject*)balColl.node;
		if (closest->GetCollisionLayer() != tempStatic)
		{
			layer = standard;
			this->physicsObject->useGravity = true;
		}

		sceneTime += dt;

		Vector3 camForward = -GetTransform().GetMatrix().GetColumn(2).Normalised();
		float dotProd;
		for (int i = 0; i < 2; i++)
		{
			if (players[i] != nullptr)
			{
				Vector3 toP = (players[i]->GetTransform().GetPosition() - GetTransform().GetPosition());
				Vector3 toPnorm = toP.Normalised();

				PlayerObject* closestP;
				Ray camRay(GetTransform().GetPosition(), toPnorm);
				RayCollision rayColl;
				g->Raycast(camRay, rayColl, true, this);
				GameObject* closest = (GameObject*)rayColl.node;
				
				dotProd = Vector3::Dot(camForward, toPnorm);
				if (dotProd > 0.9 && toP.Length() < 150)
				{					
					if (closest != nullptr)
					{
						if (closest->GetCollisionLayer() & player)
						{
							closestP = (PlayerObject*)closest;
							closestP->SetSpotted(true);
							Debug::DrawLine(GetTransform().GetPosition(), rayColl.collidedAt, { 1,0,0,1 }, 0.01f);
						}
							
					}
					//track the first player you can see inside the radius
					if (trackTarget == nullptr) trackTarget = players[i];
					
				}
				//if you lose sight of the target, unspot them, and go back to tracking nothing
				if (dotProd < 0.9  || toP.Length() > 150)
				{

					if (players[i] == trackTarget) trackTarget = nullptr;
				}
				//if the player is behind something, unspot and untrack them
				if (closest != nullptr)
				{
					if (closest->GetCollisionLayer() != player)
					{
						if (players[i] == trackTarget) trackTarget = nullptr;
					}

				}
								
			}
		}
		stateMachine->Update(dt);
	}
}


void CCTVStateObject::SearchRotate(float sceneTime, float dt) {
	GetPhysicsObject()->AddTorque(Vector3(0, 1, 0) * sinf(sceneTime));
}

void CCTVStateObject::TrackPlayer()
{
	//always be checking for the nullpointer, lest it haunt you!!
	if (trackTarget != nullptr)
	{
		//we're caclulating which way to turn via a vector orthogonal to forward & up: 
		//this allows us to see if we're too far left or right as it'll be either side of zero
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
