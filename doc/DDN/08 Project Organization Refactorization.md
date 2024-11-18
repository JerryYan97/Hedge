# Hedge project organization refactorization

This design doc describes the new organization of the Hedge game engine. The purpose of it is to make Hedge engine similar to the UE setup, which has its game and engine stands side by side.

## Introduction to relating basic components

The basic components are still same as before.

* Engine: The Hedge game engine itself is a static library that contains code of the engine. This library is used by linking it into the game editor or the game application executable. It contains the entry point of the game executable and editor executable. The engine code itself has two modes: editor mode and game mode. When a developer builds the engine in the editor project, the engine would exclude the game template code. Otherwise, the engine library would include the game template symbols.

* Editor: The Hedge game engine editor is an executable. It has its own code that is separate from the engine's static library and needs to be linked to the engine library to build itself. The editor can be used to create a game project and package the game project into a game executable with assets. The editor can create two types of game. The first type is a debug mode game. This type of game is created in the same folder of the game project and it has a visual studio solution for the purpose of debugging the generated game. The second type is a release mode game, which is the final version that a player would play with.

* Game project: A game project contains all the assets and custom code that a game should have. These custom code will be compiled to relocatable objects and linked to the engine static libaray when the editor builds the game executable. The core of a game project is a `gameConfig.yml` file, which contains all the configuration information that a game project has.

* Game: A final game should be an executable with all relevant data. It is built from the game projects' custom code and the engine static library.

--- It looks like I don't need to make too many changes.
* Game project needs to have the source code of the game engine. Directly launch the debug mode game in the visual studio.
* I may don't need the `Open Project...` and `Save Project to...` options.
* Editor is responsible for the level editing and other resource management, but it's not necessary since all information should be able to be edited in hand.
* Editor still needs to be responsible for the final packaging.