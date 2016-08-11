# Vulkan Engine
## About
This is a Vulkan rendering engine that I'm working on.  It sucks, you probably
want to find something else.  But hell, if you like bad code you probably would
like to have instructions to:
## Build
It's pretty easy.  You need the following:
### SDL2
Window management.  Makes my life easier.
### Vulkan Loader and Debug Libraries
Grab those over at GitHub: https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers .  They have seperate build instructions..
### GLM
Excellent header-only math library very similar in syntax to GLSL.
### Got the libraries?
Next take a look at the Makefile that applies to you.  I have two Makefiles.
One for UNIX (although I've only tested on Linux...), and one for Windows,
but it will probably required msys2 to actually build correctly.  Take a look
at the paths to where the Makefiles expect the libraries to be.  Change them
to suit your preferences, or simply change the paths in the Makefiles.  Once
you've done that you can

````
cp Makefile.**** Makefile
make
````
And if you're using msys2:
````
./vktest.exe [optional arguments]
````
or, on Unix:
````
make test
````
## Find Something Broken?
Help me fix it please.  Fork, fix, submit pull request.  But it's my project,
so if I don't like your code, I probably won't accept it.

