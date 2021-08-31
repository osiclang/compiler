
## nerOSIC beta

**neurOSIC** is a attempt to create a possiblity of artificial intelligence in a pattern of a Brain supported by the OSIC Language.
Therefore we do not create agents with fixed agencies.
To archieve it we create something similar to nervs that can be found in lifeforms around us.

### The goal

The goal is to create an artificial intelligence by a pattern that is most similar to the natures one. So we can proof, if this approach creates unexpected pattern that might could lead to a new kind computing.

### What is missing

Currently there is an inner clock missing and the pattern just can evolve when a singal was send over an `reator()`.
Later there must be an possibility to provide single clocks for each cell, to create independ behaviors.

### types

- `reactor()`

This type has only one constrain to other "cells". It is the input type to send new informations.

- `actor()`

This type has only one constrain to other "cells". It is the output type to give singals to the outher world.

- `cell()`

This type propagates singals to other nero() types or even to a actor() type. This type of "cell" can have up to eight constrains to other types of every kind.

### signal types

A signal can be a type of this.

- `ReceiverType(Signal)`

It is just a simple signal that can be propergated to contrains that can handle this type.

- `ReceiverType(Good)` or `ReceiverType(Bad)`

It is a signal that can produced and will be propagated to every "cell" and can create a signal that the work was well done or not.

### Structure and rules of calculation

This is the very first step osic create a structure of about n neuro() types, n reactor() types, and even n actor() types. All of them are connected to their next neightbors.

The cell() types get a built-in types for `ReceiverType()` of the type `ReceiverType(Good)` and `ReceiverType(Bad)`. This forces the cell types to create other constrains until the system sends a `ReceiverType(Good)`.
This Singal means that the Task was passed succesfull.

The System will run, when a workset is given.
As an example there is a workset, that say:
```
When reator(1) is called, there should be a signal on actor(3)
```
The cells() types will now start to connect and rearrage the flow of a signal to other cells until the Signal is transferd to the right actor().
They try as long as all cells() will receive a proto("good").
If the signal is propagated to the wrong actor() or to many actors() where involed they always receive a `ReceiverType(Bad)`.
Once a `ReceiverType(Good)` is received the structure of the types will be stored.
In this case, they can repeat this action, every time the signal on the reactor() is received.

After that a signal will propergated again and the contraints wich are involed to reach the goal get one point.

What happen now, when a second workset is given that might could look like:
```
When reator(2) is called, there should be a signal on actor(4)
```
The System of cell() types have to try as long as it needs, until it is satisfied with an proto("good") just by rearraging the contraints with a lower score.

### cell() type rules
A cell() type can only have a maximum of 8 constrains to other cells.
If a cell() type have a contrain to a actor() he cannot have a constrain to a reactor() or vice versa.
A cell() type can propagate the given signal to a set of other constrains. So he have to choose, with contrains he is sending the signal. He also can choose independenly based on the contrain he receive.
Example:
```
cell(1){
    cell(1) -> cell(3)
    cell(2) -> cell(4)
    cell(3) -> cell(5)

    when received on node 1 -> send signal to 2,3
    when received on node 2 -> send signal to 3
    when received on node 3 -> send signal to 1,2
}
```


### Roadmap
- ( ) create a language setup to create a pattern of working nodes
- ( ) design a system to etablish a kind of learning process to reach a goal by try and error
- () design a storing of an created nodegrid if the workset was succesful completed
- ( ) create a posibilty to store a current nodegrid an the recreation of this nodegrid to a new created one

