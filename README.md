# Project6-GUI-in-C

## Steps to run Task Manager:
1.  `brew install gtk+3`
2.  `brew install pkg-config`
3.  `pkg-config --cflags gtk+-3.0`
4.  Take the output of Step 3, and include it in your `c_cpp_properties.json` file in the .vscode folder. When doing this step, remove the -I from the beginning of each path. Your includePath should something look like this:

![Yash's c_cpp_properties.json file](http://url/to/img.png)

