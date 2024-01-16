#pragma once
#include "GameObject.h"
#include "string"
#include "NetworkObject.h"

namespace NCL
{
	namespace CSC8503
	{
		class PlayerObject : public GameObject
		{
		public:
			bool change;

			PlayerObject(std::string name, int networkID)
				:GameObject(name, player)
			{
				//IDs for players should be 0 and 1
				networkObject = new NetworkObject(*this, networkID);
				score = 0;
				win = false;
				spotted = false;
				spotTimer = 5.0f;
			}
			void OnCollisionBegin(GameObject* otherObject) override
			{
				if (otherObject->GetName() == "heist")
				{
					otherObject->SetActive(false);
					AdjustScore(1);
				}
			}

			int GetScore() { return score; }
			void AdjustScore(int sc) { 
				score += sc; 
				change = true;
			}
			void SetScore(int sc) { score = sc; }
			void SetSpotted(bool spotted) { 
				this->spotted = spotted; 
				change = true;
			}
			bool GetSpotted() { return spotted; }
			void SetWin(bool win) { 
				this->win = win; 
				change = true;
			}
			bool GetWin() { return win; }
			void SetSpotTimer(float st) { 
				spotTimer = st;

			}
			void AdjustSpotTimer(float st) { 
				spotTimer -= st; 

			}
			float GetSpotTimer() { return spotTimer; }

			
		protected:
			int score;
			bool spotted;
			bool win;
			float spotTimer;
			

		};
	}
}
