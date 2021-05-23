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
* ```-d``` option: directory search mode

```sh
alias fzcd='cd `fz -d`'
```
![fzcd](https://user-images.githubusercontent.com/44718643/119250573-f524e580-bbdb-11eb-8cac-5361e496c8b4.gif)

* ```-e``` option:  ENV mode

```sh
export FZ_BASE_PATH=/home/user/Python-3.8.8
alias fzvim='vim `fz -e`'
```

![fzvim](https://user-images.githubusercontent.com/44718643/119250585-0a9a0f80-bbdc-11eb-87aa-c5fc9bc82d6a.gif)



