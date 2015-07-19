# Introduction #

Certain SilverTreeWML attributes can contain formulas. A formula is a calculation which will be run at the appropriate time in-game to calculate a result for some game logic.

As an example, suppose in the rules WML you wanted to say that damage was equal to the character's strength + 10, you would simply put,

damage=strength + 10

This means of course, that simple numbers, for instance, are legal formulas. So, you could just go,

damage=10

to make damage always equal to 10. Some WML attributes are usually just specified as numbers, but have the added flexibility of being formulas where desired.

## Details ##

Different formulas are used at different times, of course. For instance, the formula to determine the color of sunlight wouldn't depend on a character's strength. For this reason, different formulas have different sets of inputs, depending on their context. The input for light calculations is the time of day: 'hour', 'minute', and 'second'. If one tried to use 'strength' in a light calculation, it wouldn't be recognized (meaning it would be considered equal to 0).

Most formulas that calculate character statistics have characters as inputs. This means you can access all standard character statistics. This includes the basic attributes, strength, endurance, etc. hitpoints and max\_hitpoints, speed, and so on and so forth.

The `[rules]`/`[calculations]` section in data/rules.cfg for instance contains calculations of a large range of character stats based on other stats the character has.

## Functions ##

In addition to standard arithmetic, there are 'functions' which are available for more complex operations. A common example of such a function is the 'if' function. Suppose you wanted to make it so that damage was equal to 10, unless the character's strength was greater than 10, in which case it was equal to the character's strength. You could do this:

damage=if(strength > 10, strength, 10)

The 'if' function evaluates the first argument. If it is 'true' it will then evaluate to the second argument, otherwise it will evaluate to the third argument.

Another function is the rgb function, which given three numbers, will evaluate to an integer which merges the three numbers correctly to be given back as a value for lighting purposes. For instance, to make sun light always pure white,

sun\_light=rgb(100,100,100)

You can of course merge functions together. For instance, to make sun light pure white between 6 am and 6 pm and pure black otherwise,

sun\_light=if(hour >= 6 and hour <= 18, rgb(100,100,100), rgb(0,0,0))

The 'transition' function is used to map a value from one scale to another.

transition(value, min1, min2, max1, max2)

For instance, a healer that charges 1 trinka per percent of health restored would have a healing\_cost=transition(hitpoints, max\_hitpoints, 0, 0, 100) (we're of course inverting the scale in the process in this example.)

## Objects and Lists ##

It is also possible for formulas to have 'objects' as inputs. An object is an input that contains other values. For instance, a party is an input. When an 'encounter' event is fired, the input to all formulas in the event handler contains 'pc' which is an object referring to the character's party, and 'npc', which is an object referring to the NPC party the PC has encountered.

For instance, to make an event occur only if the PC has more than 1000 gold, one might put this filter inside the [event](event.md):

filter="pc.money > 1000"

'pc' is an input that contains many inputs, including 'money'. The '.' accesses an input within another input.

Some inputs may also be lists. A list contains multiple inputs in a sequence. For instance, a party contains an input called 'members' which is a list of all the members of the party, each of which is itself a character input, which contains all the characters attributes -- strength, endurance, etc.

List elements may be referenced by using this notation: pc.members.0, party.members.1 etc.

A list can be operated on by several functions, which are documented in the function list.


# Function list #

### abs(int) ###
Evaluates the absolute value of an integer.

### choose(list, formula) ###
Evaluates 'formula' for each item in the list. Will evaluate to the item which 'formula' gave the highest value.
  * To give back the character with the highest strength, `choose(pc.members, strength)`. Note that this is different to `max(map(pc.members, strength))` because choose will return the actual character. max+map will return the character's strength, not the character themselves. This is very useful for finding a character with the highest or lowest of a certain attribute.

### color\_transition(...) ###
Changes value depending on the first input.
  * `ambient_light="color_transition(hour*60+minute,0,rgb(0,0,50), 12*60,rgb(40,70,100), 24*60,rgb(0,0,50))"` Changes the lighting from `rgb(0,0,50)` at midnight to `rgb(40,70,100)` at noon and back.

### count(list, element) ###
Counts the number of times an element appears in a list.
  * `count([1,1,2,3,5],1) = 2`
  * `count([1,2,3,4],5) = 0`

### distance(loc1,loc2) ###
Returns the distance between two hexes. Both inputs must be locations.

### element(list, position) ###
Returns a certain element from a list.
  * `element(list,2)` is equivalent `list.2`

### filter(list, formula) ###
Will run 'formula' on each item in the list. Will evaluate to a list which only contains items the formula was true for.
  * `size(filter(pc.members, hitpoints < max_hitpoints)) > 0` -- tells you if any of the characters in the party are injured.

### find(list,condition) ###
Returns the first element of the list that matches the condition. Returns null if no element is found.

### head(list) ###
The first item in the list
  * `head(npc.members)` -- the first character in the NPC's party (i.e. the party leader).

### if(condition, then, else) ###
Evaluates `then` if `condition` is true, `else` if `condition` is false.
  * damage=if(strength > 10, strength, 10) -- Damage is equal to 10, unless the character's strength iss greater than 10, in which case it is equal to the character's strength.

### index(list, element) ###
Returns the index of the first time an element appears in a list. If there is no such element, it evaluates to -1.
  * `index([3,2,1,2,3],2) = 1`
  * `index([1,2,3,4],5) = -1`

### loc(x,y) ###
Returns a location generated by two coordinates.

### map(list, formula) ###
Will run 'formula' on each item in the list, and evaluate to a new list which contains the same number of items as in 'list', with the formulas run on each item.
  * `map(pc.members, hitpoints)` -- will give a list back with the number of hitpoints each party member has. This is more useful in conjunction with other functions.

### max(list); min(list) ###
Evaluates to the maximum or minimum item in the list (which all must be numbers)
  * `max(map(pc.members, strength))` -- finds the strength of the strongest character in the party

### null() ###
Evaluates to `null`.

### remove(list, object); remove(list, list) ###
If any elements of list are found in the second argument, they are removed from list1.
  * `remove([4,5], 4) = [5]`
  * `remove([4,5,4], [4]) = [5]`
  * `remove([2,7,8,9], [2,8]) = [7,9]`

### rgb(red,green,blue) ###
Combines three integers for lighting purposes. Values of `red`,`green`,`blue` range from 0 to 100
  * `rgb(100,100,100)` is pure qhite light

### size(list) ###
Number of items in the list
  * `size(pc.members)` -- number of characters in the PC's party

### sort(list, formula) ###
Evaluates to a list sorted according to the comparison 'formula' for each item 'a' and its successor 'b'.
  * Sorting characters according to initiative would be done by `sort(pc.members, a.initiative > b.initiative)`.

### sum(list) ###
Evaluates to the sum of the items in the list (which all must be numbers)
  * `sum(map(pc.members, max_hitpoints - hitpoints))` -- finds the total damage the characters in the party have taken (perhaps to calculate healing costs)

### transition(value, min1, min2, max1, max2) ###
Maps a value from one scale to another.
  * `healing_cost=transition(hitpoints, max_hitpoints, 0, 0, 100)` -- A healer that charges 1 trinka per percent of health restored

### wave(int) ###
  * `wave(x) = sin(2*Pi*(x%1000/1000))*1000`



# Operator list #
  * not
  * where (see below)
  * or
  * and
  * =
  * !=
  * <
  * >
  * <=
  * >=
  * +
  * -
  * `*`
  * /
  * %
  * ^
  * d (see below)
  * .

### where ###
Sets the value of a variable for a certain formula
  * `(x*2 where x=5)` = 10

### (num\_rolls)d(faces) ###
Dice roll (used to generate random numbers). Rolls the a die with `(faces)` sides `num_rolls` number of times, adding the results.
  * `3d6` ranges from 3 and 18 inclusive