# Event system

The hedge uses an immediate event system, which is written in the `HEvent.h` file. The system is managed by the `HEventManager`. Any objects or entites can register an event to listen or send an event to its listeners by using `HEventManager`'s `RegisterListener(...)` and `SendEvent(...)`.

Only an entity can be registered as a listener to a set of events. And the entity needs to implement its `OnEvent(...)` function to handle the event that it is interested in.

An event in the hedge has a name (AKA the type of the event) and an argument map. The key of the map is the name of the argument and the value of the map can be any type of data. One thing that needs to be noted is that we use `crc32(...)` to translate our string to the hash number to pass around. 

A game or entity can send its own event to the event manager at anytime. But the engine also has some of its bulit-in events for convenience.

## Engine built-in event types and arguments

### GUI events

GUI events would be sent out from the editor or the game template's `SendIOEvents(...)` at the beginning of each frames.

* MOUSE_MIDDLE_BUTTON: IS_DOWN (`bool`), POS (`HFVec2`).
* KEY_W: IS_DOWN (`bool`).
* KEY_S: IS_DOWN (`bool`).
* KEY_A: IS_DOWN (`bool`).
* KEY_D: IS_DOWN (`bool`).

## Reference
1. Game Engine Architecture
2. Cherno -- Hazel game engine