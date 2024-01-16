#pragma once
#include "BehaviourNode.h"

typedef std::function<BehaviourState(float, BehaviourState)> BehaviourActionFunc;

class BehaviourAction : public BehaviourNode	{
public:
	BehaviourAction(const std::string& nodeName, BehaviourActionFunc f) : BehaviourNode(nodeName) {
		function = f;	//sets custom function
	}
	BehaviourState Execute(float dt) override {
		currentState = function(dt, currentState);	//calls custom function
		return currentState;
	}
protected:
	BehaviourActionFunc function;
};