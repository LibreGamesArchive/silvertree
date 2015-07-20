# SilverTree - A 3D turn-based RPG engine and adventure

The SilverTree RPG project aims at producing an open source single player
3D RPG with a hybrid real-time/turn-based system.

SilverTree intends to include a wide range of adventuring activities, in
addition to combat and looting. In the wilderness tracking and stealth can
provide the all-important element of surprise in combat, or allow the player
to avoid more powerful foes. Literacy allows characters to use books and
manuscripts to study new skills.

Grinding will be minimized. Less experience is gained each time the same type
of enemy is defeated, and healing is not easily obtained. Most enemies will
try to flee from the PC when they have little chance of survival.

SilverTree will allows play as several different types of heros. The PC's
response to moral dilemmas will be measured on several scales such as Honest
*vs* Deceptive, and NPCs will react accordingly, but there is not only one 
right answer.

## Building

Type './configure' and then 'make' to build the game.

Notable dependencies are Boost, SDL, SDL_ttf, and SDL_image, and OpenGL. The
editor depends on Qt 4.

## Running

To play the game type './silvertreerpg'.

To run the editor (on the main scenario file) type './editorsilvertree-editor
data/scenario.cfg' .

## Controls

You move a party of characters around a hexagonally tiled map. Assuming north
is toward the top of the screen, moving north and south is accomplished by
pressing the up and down arrow. Moving in a diagonal direction involves
pressing the left or right arrow key, and holding it while you also press the
up or down arrow.

For instance, press left and down to move south-west.

You move your party one hex at a time. Once your party begins moving from one
hex to another, you've committed to that move and won't have control over your
party again until the move is complete.  Other parties will only move while
you are moving or resting.  Whenever your party isn't carrying out your
commands, the game automatically pauses until you issue a new command.

You can also control the camera. Press 'z' to zoom in, and 'x' to zoom out.
Hold control and press the arrow keys to rotate and tilt the camera.  As
alternate keys, 'p' and 'l' will tilt the camera, while '<' and '>' will
rotate the camera.

## Character status

Press 's' to enter the party's status screen. The characters in your party are
displayed at the top. You can click on a party member to go to that
character's status screen. From here you can upgrade their attributes, learn
skills, and change their equipment.

At the start of the game, your main character has many unassigned attribute
points. It is recommended that you immediately build your character by going
to the status screen.

## Fatigue

In the top-left of the screen on the main display are two numbers listed for
each character. For instance, 0/85. The '0' in this example indicates the
character's fatigue, and the '85' their stamina. As the party performs
actions, fatigue increases. Once fatigue is greater than stamina, the
character begins to tire and becomes slower, and suffers penalties in combat.

## Resting and running

Hold space to make your party rest. Fatigue will be removed as the party
rests.

Hold shift while moving to make your party run. The party moves at double
speed when running, however fatigue accumulates at four times the normal rate.

## Combat

When you encounter a hostile party, combat will be initiated on a combat map.

* In combat, every action has an "action cost."
* The unit performs the action, and then is "unready" until the "action cost"
can be paid off.
* "Unready" units can only stand and defend themselves.
* The "action cost" is paid off at the rate of 1 action point per second of
"battle time".
* When a unit pays off it's previous "action cost," it becomes "ready" and can
act again.
* If more than one unit is "ready" during the same second of "battle time" the
unit with the highest initiative goes first.

## More info

Visit http://www.silvertreerpg.org/ to check out the latest in Silver Tree
development.

We are currently looking for assistance in many areas of development.

David White
dave@whitevine.net
