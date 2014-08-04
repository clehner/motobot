# motobot

**A modular IRC conversation bot written in C.**

Based on [rochako](https://github.com/rochack/rochako)

# Features
- Modular structure
- Connects to IRC
- Responds using [markov chains](https://github.com/clehner/chains)
- Reads and writes channel activity to a log file
- Keeps track of karma, and allows querying it
- Report user ping times (this is satire)

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

- Set up user and systemd service (optional)
```
sudo make install
sudo useradd -r motobot -m -d /var/lib/motobot
sudo cp motobot.service /etc/systemd/system/
sudo systemctl enable motobot
sudo systemctl start motobot
```

# Todo
- Upload channel activity to couchdb
- /clone - multiple connections and derivative corpuses
- /join - move into other rooms
