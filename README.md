# Project6-GUI-in-C

## Steps to run Task Manager:
1.  Install GTK Development Libraries:
```bash
brew install gtk+3
```
2. Install PKG Config Helper:
```bash
brew install pkg-config
```
3. Run the following command:
```
pkg-config --cflags gtk+-3.0
```
4. Take the output of Step 3, and include it in your `c_cpp_properties.json` file in the .vscode folder. When doing this step, remove the -I from the beginning of each path. Your includePath should something look like this:
![Yash's c_cpp_properties.json file](https://github.com/Arty5654/Project6-GUI-in-C/blob/main/includePath.png?raw=true)
5. Code away, and once you are ready to compile, run the following command:
```bash
gcc -o main main.c `pkg-config --cflags --libs gtk+-3.0`
```
6. Run the Task Manager through `./main`
