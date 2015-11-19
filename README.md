# Compact pwd

Find the shortest yet unique working directory path.

![](./preview.png)

`cpwd` checks each directory for similarly named directories and files and cuts out characters from the back, middle, or front until the shortest unique name is found. Chomped characters from the back and middle are replaced with `*` to be compatible with zsh's auto complete.
