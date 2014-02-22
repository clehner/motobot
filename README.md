# motobot

**A modular IRC conversation bot written in C.**

Based on [rochako](https://github.com/rochack/rochako)

# Features
- Modular structure
- Connects to IRC
- Responds using [markov chains](https://github.com/clehner/chains)
- Reads and writes channel activity to a log file

# Installation
- Get dependencies
```
apt-get install libircclient-dev
```

- Compile
```
make
```

- Make a config file
```
cp config.def.ini config.ini
vi config.ini
```

- Run the program
```
./motobot config.ini
```

# Todo
- Upload channel activity to couchdb
- /clone - multiple connections and derivative corpuses
- /join - move into other rooms
- /karma
