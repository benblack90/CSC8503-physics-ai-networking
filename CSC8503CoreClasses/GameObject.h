#pragma once
#include "Transform.h"
#include "CollisionVolume.h"
#include "PhysicsObject.h"

using std::vector;

namespace NCL::CSC8503 {
	class NetworkObject;
	class RenderObject;

	enum CollisionLayer
	{
		staticObj = 1,
		tempStatic = 2,
		collectable = 4,
		player = 8,
		enemy = 16,
		standard = 32,
		zone = 64
	};

	class GameObject	{
	public:
		GameObject(const std::string& name = "", CollisionLayer layer = standard, bool useGravity = true);
		~GameObject();

		void SetBoundingVolume(CollisionVolume* vol) {
			boundingVolume = vol;
		}

		const CollisionVolume* GetBoundingVolume() const {
			return boundingVolume;
		}

		bool IsActive() const {
			return isActive;
		}

		void SetActive(bool active) { isActive = active; }

		Transform& GetTransform() {
			return transform;
		}

		RenderObject* GetRenderObject() const {
			return renderObject;
		}

		PhysicsObject* GetPhysicsObject() const {
			return physicsObject;
		}

		NetworkObject* GetNetworkObject() const {
			return networkObject;
		}

		void SetNetworkObject(NetworkObject* n) { networkObject = n; }

		void SetRenderObject(RenderObject* newObject) {
			renderObject = newObject;
		}

		void SetPhysicsObject(PhysicsObject* newObject) {
			physicsObject = newObject;
			physicsObject->useGravity = useGravity;
		}

		const std::string& GetName() const {
			return name;
		}

		virtual void OnCollisionBegin(GameObject* otherObject) {
			//std::cout << "OnCollisionBegin event occured!\n";
		}

		virtual void OnCollisionEnd(GameObject* otherObject) {
			//std::cout << "OnCollisionEnd event occured!\n";
		}

		bool GetBroadphaseAABB(Vector3&outsize) const;

		void UpdateBroadphaseAABB();

		void SetWorldID(int newID) {
			worldID = newID;
		}

		int		GetWorldID() const {
			return worldID;
		}

		CollisionLayer GetCollisionLayer() { return layer; }
		void SetCollisionLayer(CollisionLayer layer) { this->layer = layer; }

	protected:
		Transform			transform;

		CollisionVolume*	boundingVolume;
		PhysicsObject*		physicsObject;
		RenderObject*		renderObject;
		NetworkObject*		networkObject;

		bool		isActive;
		int			worldID;
		bool useGravity;
		std::string	name;
		CollisionLayer layer;

		Vector3 broadphaseAABB;
	};
}

