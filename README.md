# NoGo-Framework

Framework for NoGo and similar games (C++ 11)

## Basic Usage

To make the sample program:
```bash
make # see makefile for details
```

To run the sample program:
```bash
./nogo # by default the program runs 1000 games
```

To specify the total games to run:
```bash
./nogo --total=1000
```

To display the statistic every 1 episode:
```bash
./nogo --total=1000 --block=1 --limit=1
```

To specify the total games to run, and seed the player:
```bash
./nogo --total=1000 --black="seed=12345" --white="seed=54321"
```

To save the statistic result to a file:
```bash
./nogo --save=stat.txt
```

To load and review the statistic result from a file:
```bash
./nogo --load=stat.txt
```

## Advanced Usage

To specify custom player arguments (need to be implemented by yourself):
```bash
./nogo --total=1000 --black="search=MCTS timeout=1000" --white="search=alpha-beta depth=3"
```

To launch the GTP shell and specify program name for the GTP server:
```bash
./nogo --shell --name="MyNoGo" --version="1.0"
```

To launch the GTP shell with custom player arguments:
```bash
./nogo --shell --black="search=MCTS simulation=1000" --white="search=alpha-beta depth=3"
```

To launch MCTS:
This is default settings. Tweak the parameter below if you want to.
```bash
./nogo --black="search=MCTS time=900 RAVE=0.5"
```

If time = -1
```bash
./nogo --black="Search=MCTS time=-1 C=10 ttime=300"
```
Where C is a constant that alloted time = remaining_time / C, and total time for a player is ttime.

If time = -2
```bash
./nogo --black="Search=MCTS time=-1 C1=10 C2=10 ttime=300"
```
think_time = remaining_time / (C1 + max(C2 - ply, 0)), where ply is the number of move.

## Author

[Computer Games and Intelligence (CGI) Lab](https://cgilab.nctu.edu.tw/), NYCU, Taiwan
