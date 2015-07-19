# Events #

The `[event]` tag is used to capture various events that occur in-game. An event should have one or more child elements which contain WmlCommands. Each of these commands will be executed when the event is triggered.

Each event has a set of inputs passed to the event that all the formulas within the event can reference.

All events receive certain inputs:

  * world -- the game world the event takes place in.
  * var -- all game variables.

`[event]` has three possible attributes:

filter: a formula which runs on the event's input. The event handler will only be run if the filter evaluates to true.

first\_time\_only: if set, the event handler will only be run once.

event: the type of the event being caught. The events currently supported are:

## start ##

Fired when a scenario starts (including when a game is loaded; use first\_time\_only if you only want it to be handled once)

## begin\_move ##

Fired whenever a party starts moving.

Inputs:
  * party: the party which will move

## end\_move ##

Fired whenever a party completes a move.

Inputs:
  * party: the party which moved

## sight ##

Fired when a party has line of sight with an npc.

Inputs:
  * pc: the player's party
  * npc: the npc party which was sighted

## tick ##

Fired every minute.

Inputs: standard

## encounter ##
Fired whenever the PC party encounters an NPC party. (Encounters usually happen when the player attempts to move onto the npc's hex)

Inputs:
  * pc: the player's party
  * npc: the npc party which was encountered

You can use an `[encounter]` tag inside an NPC party definition to define an event handler for this kind of event as a convenience.
## win\_battle ##
When a battle against the filtered party is won.

Inputs:
  * pc: the player's party
  * npc: the npc party which was defeated

## finish\_script ##
If there is a movement script running (a set of scripted movements created with the execute\_script WML command) will be fired when the script finishes.


Inputs:
  * script: name of the script

See [ScriptedMoves](ScriptedMoves.md) for more details.