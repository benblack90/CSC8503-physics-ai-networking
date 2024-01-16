#pragma once

namespace NCL {
	namespace CSC8503 {

		class PushdownState
		{
		
		public:
			enum PushdownResult {
				Push, Pop, NoChange
			};
			struct PushdownMessage
			{
				PushdownResult result;
				char message;
			};
			
			PushdownState()  {
			}
			virtual ~PushdownState() {}

			virtual PushdownMessage OnUpdate(float dt, PushdownState** pushFunc) = 0;
			virtual void OnAwake() {}
			virtual void OnSleep() {}
			
		protected:
		};
	}
}