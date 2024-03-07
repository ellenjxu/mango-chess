## Title of your project
Mango Magic

## Team members
Javier Garcia Nieto and Ellen Xu

## Project description
In short, we want to get two Pi's to talk to each other. Once that is working,
we have grand plans.

We want to create a magic trick (or several) that incorporate the Mango Pi.
One path is to use the Mango Pi as the cheat device. Examples:
 - A spectator takes a card and an accomplice sends the card to the magician through Bluetooth. The magician has a Pi in their pocket, which buzzes to indicate the card (this would be our baseline trick).
 - Everyone in the room takes cards in order. One of them is an accomplice. The cards are ordered in such a way that knowing the accomplice's card can predict everyone's. The Pi is used to communicate this card and compute the sequence.
 - Chess cheating: a second Mango Pi is connected via UART to a computer, which uses Stockfish to compute the next move based on secret commands that the player sends.

Anti bike theft: if a Pi senses motion and the other Pi (or your phone) is not nearby, it screams. This can also work with a hall sensor and a door.

## Hardware, budget
Hardware we will use:
 - Rotary encoder (from lab)
 - Piezo buzzer (from lab, or purchase)
 - 2xBluetooth module (as backup if on-board BT doesn't work https://store-usa.arduino.cc/products/bluetooth-low-energy-4-0-module-hm-10)
 - Battery bank (which we already have)

Total is around $35.

## Tasks, member responsibilities
Javier: mostly, get Bluetooth working and create a module Ellen can use. Magic expertise.
Ellen: use Javier's BT module, with her own peripheral drivers to get full tricks working

## Schedule, midpoint milestones
Do we have Bluetooth (of some form working)?
Can we get a simple message accross?

Bonus: can we get the Mango Pi working off a power bank?

## Resources needed, issues
Bluetooth is hard.
Other than that, should be straightforward.
