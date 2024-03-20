## Project title

Mango Chess

## Team members

Javier Garcia Nieto and Ellen Xu

## Project description

Using a MangoPi and Bluetooth communication to enable Stockfish chess cheating!

One player has a "Hand" MangoPi in their pocket, which is talking to the "Brain" MangoPi running a Stockfish engine. The player encodes the game state using a rotary encoder, and receives buzzes encoding the next move from Stockfish. The Brain Pi also runs a live chess GUI which streams the current game.

## Member contribution

Javier:

- Bluetooth JNXU protocol
- ...

Ellen:

- Stockfish engine (computer running Stockfish <-> pi via uart)
- Brain Pi ('brain.c`)
- Chess GUI

## References

We were inspired to do something cool with Bluetooth and the fact that the Pi is a "tiny, tiny computer" to do some magic. Eventually we came up with the idea to allow Pis to talk to one another and act as an accomplice for chess.

No external material was used, other than an external Arduino bluetooth module.

## Self-evaluation

The Pis are able to talk to one another! We were able to get Bluetooth communication with two Pis, the engine running on the computer, monitor displaying the GUI, and two peripheries all working live for the chess game. We didn't end up pursuing other tricks for Mango magic, but went deeper on the chess idea and are happy with how it turned out.

Moments we're proud of:

- bluetooth connecting the two pis!!
- getting stockfish working! and seeing the moves appear live (e2e4 d7d5 e4xe5 moment... :D)
- rotary encoder + cursor logic for highlighting which piece is being moved

Any trying or heroic moments you would like to share? Of what are you particularly proud:
the effort you put into it? the end product? the process you followed?
what you learned along the way? Tell us about it!

## Photos

You are encouraged to submit photos/videos of your project in action.
Add the files and commit to your project repository to include along with your submission.
