# fzc
Simple Fuzzy Finder write in C 

## Compile

curses , ncurses based
define option ```-DFZ_BIN_MAIN``` 

ex)
```sh
gcc -o fz -DFZ_BIN_MAIN fz.c -lncurses
```

## Usage
* ```-d``` option: directory 

```sh
alias fzcd='cd `fz -d`'
```

* ```-e``` option:  ENV 

```sh
export FZ_BASE_PATH=/home/user/Python-3.8.8
alias fzvim='vim `fz -e`'
```



