#pragma once
#include "PushdownState.h"

namespace NCL {
	namespace CSC8503 {
		class PushdownState;

		class PushdownMachine
		{
		public:
			PushdownMachine() {};
			PushdownMachine(PushdownState* initialState);
			~PushdownMachine();

			bool Update(float dt);

			PushdownState::PushdownMessage messages;
			void SetInitialState(PushdownState* st) { initialState = st; }

		protected:
			PushdownState* activeState;
			PushdownState* initialState;

			std::stack<PushdownState*> stateStack;
		};
	}
}

