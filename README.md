### Description
A simple water sort game.

### Environments and Pre-requisites
Environments:


Install dependencies:

- OpenGL
  - ```sudo apt-get update```
  - ```sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev```
- Raylib
  - ```tar -zxvf raylib-5.0_linux_amd64.tar.gz```
  - ```mv raylib-5.0_linux_amd64.tar.gz/ raylib/```

### Compilation and Run
Compile:

```make```

Run:
```
export LD_LIBRARY_PATH=./raylib/lib:${LD_LIBRARY_PATH}
make && ./main
```