# Introduction #

SilverTree RPG has a fairly powerful language for specifying adventures. This language is very similar to the language used in Battle for Wesnoth, and so retains the same name, WML.

An adventure consists of a map, which has other possible maps one can enter (for instance, towns or caves, dungeons, and so forth). It also consists of various characters (NPCs) and items on the map, and a specification of how these NPCs and items behave.

The main adventure is currently defined in data/scenario.cfg and the contents look like this:

```
[scenario]
map="path/to/map"
...some other optional attributes...
  [party]
  ...One of these for each NPC 'party' on the map (and a special one for the PC party)...
  [/party]
  [settlement]
  ..One of these for each 'settlement' -- probably a cave or town -- on the map
  [/settlement]
  [event]
  ...Each of these defines a possible 'event handler' that will take some kind of action based on an event...
  [/event]
[/scenario]
```

# Scenario Attributes #

In addition to the map attribute, which gives the file path of the map the scenario uses, scenarios have a few simple attributes, all of which are optional with reasonable defaults:

hour/minute/second: the time of day the scenario starts at

border\_tile: the definition of the tile that will be used for tiles outside the map (height and terrain type, same format as map tiles are in)

ambient\_light: the color of ambient light to use when rendering the scenario

sun\_light: the color of sun light (directed light, depending on time of day) to use when rendering the scenario

party\_light: the color of the light that should come from the PC's party when rendering the scenario

party\_light\_power: the intensity of the light that should come from the PC's party when rendering the scenario

## Lighting and formulas ##

The lighting settings are actually all formulas. The game supports writing formulas for certain WML attributes that will be calculated when the game is run. See [SilverTreeFormula](SilverTreeFormula.md).

## Settlements ##
```
  [settlement]
  file="data/fort.cfg"
    [portal]
    xsrc="1"
    ysrc="1"
    xdst="2"
    ydst="2"
    [/portal]    
  [/settlement]
```

This creates a portal between the coordinates (1,1) on the current map and the coordinates (2,2) in the fort scenario.

Portals are two-way, so going to the coordinates (2,2) in the fort scenario will cause the player to leave the fort map.

## Parties ##
Parties are defined using a `[party]` tag. All party definitions use the following keys:
  * `id`
  * `money`
  * `x,y` starting coordinates of the party
  * `[character]` see below

To denote the player's party, use `controller="human"`

NPCs should define:
  * `allegiance` (good/evil)
  * `aggressive` (yes/no) If no, the party will not attack the player.
  * `destination_chooser` A location that serves as the party's destination when it moves.

### Characters ###
Keys that can be used for all characters:
  * `id`
  * `description` The character's name
  * `alignment` (lawful/chaotic)
  * `equipment` The weapons the party is carrying. If none, use `equipment="weapon_slot,shield_slot,armor_slot"`
  * `model` (the party's model to display on the map) OR `image` (a placeholder image where no model exists)
  * `portrait`
  * `level`
  * `[attributes]` elements are: agility,endurance,intelligence,perception,strength,will

Player characters (or ones that may join your party) also use:
  * `improvement_points` Number of points available to improve attributes
  * `bar_portrait` The portrait displayed in the bar below the map


## Events ##

See [Events](Events.md)

For scripted moves, see [ScriptedMoves](ScriptedMoves.md)

For tags that can be used in events, see [WmlCommands](WmlCommands.md)