# Space Invaders!
My take on the classic arcade game, originally developed by Tomohiro Nishikado in 1987.
Initially made for a first year assignment at Lancaster University.

## Compile & run
In a terminal window enter
```
c99 spaceinvaders.c -o spaceinvaders -lncurses
./spaceinvaders
```
Recommended window size of minimum 130 columns and 30 rows.
**< >** arrows to move left and right and **^** to shoot, **q** to quit.

## Known quirks
There are a few small kinks that have yet to be sorted out.
First, since the number of invaders change with screen size, the game becomes
more difficult to play as the screen size decreases; fewer enemies in a blockade
means that it you'll get less points before next blockade appears, which moves
faster.
Secondly, the random bomb dropping is not truly random when a column of invaders
is gone as it will favor the following the closest column to the right with
invaders still in it.
