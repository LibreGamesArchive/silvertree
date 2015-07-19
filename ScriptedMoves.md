# Scripted Moves #

Scripted moves allow you to move parties via WML.

Generally if you want to use a script to move some characters, and then when they have moved, say something, you would run the scripted\_moves command, and then the execute\_script command. Then you would catch finish\_script and add the commands to make the characters speak.


## scripted\_moves ##
`[scripted_moves]`

Will give a party a script of moves to execute. The party will move from location to location according to the script.

  * filter: filter which determines if a party is the party to apply the script to or not.
  * `[loc]`: a waypoint in the script. Contains 'x' and 'y' which are the location to move to.

Sample:
```
[scripted_moves]
filter="id = 'vurgans'"
  [loc]
  x="42"
  y="48"
  [/loc]
  [loc]
  x="42"
  y="42"
  [/loc]
[/scripted_moves]
```

This script will move Vurgans to the coordinates (42,48) and then to (42,42)

## execute\_script ##
`[execute_script]`

Will freeze all parties in the world, except those who have scripted moves set. Those which do have scripted moves set will move until they have completed their scripted moves. When all parties have completed their scripted moves, the event 'finish\_script' will be generated, with the name of the script, and then play will continue as normal.

  * script: The name of the script. This will be given to finish\_script.

Sample:
```
[execute_script]
script="vurgans_breakup_fight"
[/execute_script]
```


## finish\_script ##
`event="finish_script"`

The finish\_script event will be fired as soon as all units complete their moves.

Sample:
```
[event]
event="finish_script"
filter="script = 'vurgans_breakup_fight'"
[dialog]
npc="find(world.parties, id = 'vurgans')"
text="I think that is enough! Are you trying to beat the poor lad to death?"
[/dialog]
...
[/event]
```