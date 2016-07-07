## Fkey3 (MIREX 16 submission)

Algorithm for automatic key detection in EDM.

### Dependencies

In order to run or compile the algorithm you need.

- Python >= 2.6
- Numpy >= 
- Essentia (http://essentia.upf.edu/)

We are actually using our own fork of essentia, which you can find here:

*https://github.com/angelfaraldo/essentia*

However, if you just prefer to use the latest essentia-master, you can copy the files in the "essentia" folder in this repository onto "/essentia/src/algorithms/tonal" and then build and compile. They are exclusive algorithms and will not overwrite any of the default ones.

Detailed instructions to build essentia can be found on the essentia site:

*http://essentia.upf.edu/documentation/installing.html*

### Compiling as executable

The script can be compiled onto an executable using pyinstaller. You can install it via pip,

$ pip install pyinstaller

and then, in the Fkey3 folder, generate the executable with the following command:

$ pyinstaller Fkey3.py

The new Fkey3 executable will be located in './dist/Fkey3'


### Extras

Additionally, we provide a simple python script to evaluate the results of the algorithm according to the MIREX competition.
