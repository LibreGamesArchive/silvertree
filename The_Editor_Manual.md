## Introduction ##
When you open the SilverTree RPG editor, silvertreeedit, you will see a black window one quarter covered with green hexagonal tiling. You can use the scroll bars to move it. At the top, there are 4 buttons (rotate left, rotate right, tilt up, tilt down) that change viewing angle and zoom. Next to them are the save and open buttons. Above those, there are three drop down menus. On the right, there is a tool bar that allows you to edit the map.

## Top Drop Down Menus ##
### File ###
Open - Open a map.

Save - Save a map.

Derive... - Generate a map from the current map, or part of it.
#### New Map ####
Width - Width (x axis) of new map to be generated.

Height - Height (y axis) of new map.

Vertical scale - Multiplier for tallness (z axis) of new map.

Fuzziness - Unknown Value. Presumably to get softer features on the new map.
#### Area on Current Map ####
x1, y1 - the coordinates for the top corner of the 'box' to be derived.

x2, y2 - the coordinates for the bottom corner of the 'box' to be derived.

### Edit ###
Undo - Delete the last change you made.

Redo - Undo the undo.

### View ###
Zoom In - Come closer

Zoom Out - Go farther

## Right Tool Bar ##
Use the keys 0 to 9 to select cursor area.

Parties - Click on the map to add a party. Parties may consist of several units. See Also: SilverTreeWML

Height - Left click to raise terrain, right-click to lower. Middle click does both.

Picker - Right-click to copy a tile, left-click to paste it. This takes _all_ the attributes of a tile, including the scenery on it.

### [Terrain\_Types](Terrain_Types.md) ###

cobbles-snow _to_ cave - Select, click on map to lay down.

void - the end of the world. Use with discretion.

water, water-deep - waterfalls occur if terrain changes are steep enough. water-deep is impassible. (water effects do not show in the editor)

### [Feature\_Types](Feature_Types.md) ###

tree - 3D forest part.

house\_feature - a standard house/village. Impassible.

house\_passable\_feature - a standard house, but you can walk over it.

tower - quality fortification for your castle.

cave\_entrance - 3D entrance to your underground lair.

colour\_cube - a pretty box

fountain\_1 - a small fountain that sends a pretty spout of water into the air. (water effects do not show in the editor)