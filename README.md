# Project6-GUI-in-C

Ensure that you have already cloned this repo into the right folder in data ...

## Setup Steps
1. Install XQuartz - https://www.xquartz.org/
2. Once downloaded, you might have to log out of your computer then log back in.
3. Open XQuartz > Settings > Security
4. Make sure "Allow connections from network clients" is checked
5. ssh into data using XQuartz terminal by doing the following
```bash
ssh -X username@data.cs.purdue.edu
```
6. Ensure that your `.vscode/c_cpp_properties.json` file looks something like this (the paths will not be the same, but it should be populated with something):
![c_cpp_properties.json](https://github.com/Arty5654/Project6-GUI-in-C/blob/main/includePath.png?raw=true)

## Steps to run Task Manager:
1. Ensure you logged into XQuartz and are still connected
2. Compile the program by running the following command (We have a Makefile):
```bash
make
```
3. Run the Task Manager using the following command:
```bash
./mytaskmanager
```
