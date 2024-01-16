#pragma once
#include "PushdownMachine.h"
#include "PushdownState.h"
#include "Window.h"
#include "TutorialGame.h"

namespace NCL
{
	namespace CSC8503
	{
		class MainMenu : public PushdownState
		{
			PushdownMessage OnUpdate(float dt, PushdownState** newState) override
			{
				PushdownMessage m;
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM1))
				{
					m.result = PushdownResult::Pop;
					return m;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM2))
				{
					m.message = 'r';
					m.result = PushdownResult::Pop;
					return m;
				}
				if(Window::GetKeyboard()->KeyPressed(KeyCodes::NUM3))
				{
					m.message = 'q';
					m.result = PushdownResult::Pop;
					return m;
				}
				m.result = PushdownResult::NoChange;
				return m;
			}
			void OnAwake() override
			{
				Debug::Print("1 Resume game", { 40,48 });
				Debug::Print("2 Restart game", { 40,52 });
				Debug::Print("3 Quit game", { 40, 56 });
			}
		};


		class WinnerMenu : public PushdownState
		{
			PushdownMessage OnUpdate(float dt, PushdownState** newState) override
			{
				PushdownMessage m;
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM1))
				{
					m.message = 'r';
					m.result = PushdownResult::Pop;
					return m;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM2))
				{
					m.message = 'q';
					m.result = PushdownResult::Pop;
					return m;
				}
				m.result = PushdownResult::NoChange;
				return m;
			}
			void OnAwake() override
			{
				Debug::Print("U won!!", { 40,48 });
				Debug::Print("1 Restart game", { 40,52 });
				Debug::Print("2 Quit game", { 40, 56 });
			}
		};


		class MultiplayerMenu : public PushdownState
		{
			PushdownMessage OnUpdate(float dt, PushdownState** newState) override
			{
				PushdownMessage m;
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM1))
				{
					m.message = 's';
					m.result = PushdownResult::Pop;
					Debug::UpdateRenderables(dt);
					return m;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM2))
				{
					m.message = 'c';
					m.result = PushdownResult::Pop;
					Debug::UpdateRenderables(dt);
					return m;
				}
				m.result = PushdownResult::NoChange;
				return m;
			}
			void OnAwake() override
			{
				Debug::Print("1 Start as server", { 40,52 });
				Debug::Print("2 Start as client", { 40, 56 });
			}
		};

		class StartMenu : public PushdownState
		{
			PushdownMessage OnUpdate(float dt, PushdownState** newState) override
			{
				PushdownMessage m;
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM1))
				{
					m.message = '1';
					m.result = PushdownResult::Pop;
					Debug::UpdateRenderables(dt);
					return m;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM2))
				{
					*newState = new MultiplayerMenu();
					m.result = PushdownResult::Push;
					Debug::UpdateRenderables(dt);
					return m;
				}
				m.result = PushdownResult::NoChange;
				return m;
			}
			void OnAwake() override
			{
				Debug::Print("Welcome to Staredown - Eyes Of The Goose", { 15,48 });
				Debug::Print("1 Start single player", { 40,52 });
				Debug::Print("2 Start multiplayer", { 40, 56 });
			}
		};

	
	}
}
