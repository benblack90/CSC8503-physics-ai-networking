# Please note this project was created with CMake, and will thus need re-cmaking on your end. Delete the cache first before running cmake. 

## FOR ANYONE REVIEWING THIS CODE 

This C++ project starts from code given to us by Dr Rich Davison at Newcastle University. Naturally, I want to draw your attention to code I actually wrote rather than his (not that it isn't brilliant!).

All the code in Block.h, CCTVStateObject.h/cpp, EvilGoose.h/cpp, Menu.h, Player.h, SpawnGoal.h, and the majority of TutorialGame.h/cpp is my own. 
In TutorialGame.cpp, a lot of the network code is heavily altered, but not originated by me: NetworkedGame.ccp shows what we were given, so any
additional methods you see are my own. In particular, the methods for receiving and processing packets on that class, and the custom PlayerObj and Client packets
were wholly written by me. 

All the gameplay code was written by me, with the exception of some of the methods for placing spheres and cubes, the methods for UpdateKeys, and the method for LockedObjectMovement; which were all heavily altered, but not originated by me. 

In CSC8503CoreClasses, PhysicsSystem, CollisionDetection, and all the classes in AI saw a lot of adaptation by me, or code that was filled in over the course of tutorials, including quad-trees, different collision types, state machines, pathfinding, and behaviour trees
(which only half-counts, in my eyes!).
The parts I wrote in their entirety, that weren't included in any tutorials: in CollisionDetection, I wrote the GJK and related methods. I also implemented collision layers, and integrated them into the physics system.
