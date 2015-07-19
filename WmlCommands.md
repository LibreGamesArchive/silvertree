# award #

The award tag is used to give the player an award.

  * party: party to give award to
  * characters: party characters to which the award is given
  * experience: experience given
  * money: money given

# battle #

The battle tag is used to start a battle.
  * loc: location of the battle (used to generate a random map)
  * pc\_party: player's party
  * pc\_chars: party members involved in this battle
  * npc\_party: enemy party
  * npc\_chars: enemy party members involved in this battle
  * `[on_victory]`: all members of this tag will be executed if the player wins
  * `[on_defeat]`: all members of this tag will be executed if the player loses

# debug #

Will evaluate the string formula and output it to the console, with the word 'DEBUG' in front.

  * text: The text to output on the console.

# debug\_console #

Will suspend the game, and open a prompt on the game's terminal. On this prompt you can type in any formula and it will be evaluated in the context of the event, and the result displayed. This is, of course, entirely for debugging purposes and should not be used in user-facing scenarios.

# destroy\_party #

Will destroy all the parties that match the given filter.

  * filter: The formula which each party is tested against. Those which evaluate to true will be destroyed.

# dialog #
  * npc: the character speaking. `npc="pc"` or `npc="npc"` may be used in events. Otherwise, `find(world.parties, id = 'vurgans')` or similar code is used.

# execute\_script #

See [ScriptedMoves](ScriptedMoves.md)

# if #

Will evaluate the given condition. If it is true, all the commands within all children named `[then]` will be evaluated, otherwise all the commands within all children named `[else]` will be evaluated.

  * condition: The formula which is evaluated to decide between executing `[then]` or `[else]`

# inventory #

This tag allows giving items to a party.

  * party: Party recieving the items. Optional, defaults to `pc`
  * acquire: Comma-separated list of items to be given to the party

# modify\_objects, set #

Will modify the objects given by 'objects'.

  * objects: Formula which evaluates to an object or a list of objects which will be modified
  * (all other attributes): the attribute to set, with the formula used to set it. Takes all inputs of the event plus 'object' which refers to the object being modified.

('set' is an alias for 'modify\_objects')

# party\_chat #

Displays a message from one of the characters in the PC party. This message will be displayed on the screen for a short amount of time, and should generally be non-critical to progress of the game.

  * text: A text string which contains the message to display.
  * speaker: The character who speaks. This may be null, in which case no message will be displayed.
  * delay: The number of milliseconds to delay the speech. This is useful if you want to make a sequence of speech by different characters.

# quit #

Ends the game.

# scripted\_moves #

See [ScriptedMoves](ScriptedMoves.md)

# shop #

  * pc: player party doing the shopping
  * items: id's of all items sold in this shop
  * cost: cost (in percent of the default) of all items


# while #

  * condition: condition for running the loop
  * (everything else): will be run repeatedly until `condition` is false