A Unix Shell

Objectives
The objectives of this project are to learn how a Unix shell works, to write a simple shell, to create processes, to handle signals, to use Unix system calls and C library function calls, and to become a better programmer.
The Assignment
You will write a simple shell in C with some "built-in" commands.

Overview
What you need to do is to have a loop that will print a prompt (consisting of a "prefix," which is initially a blank (space), followed by ' [cwd]> ', where cwd is the "current working directory," and will execute commands until the exit command is entered. In the loop you should check if the command is one of your built-in commands (given later) or a program in the search paths. You also need to support passing arguments to the commands (i.e., you will need to build an argv array). If the command entered is neither one of your built-in commands nor in the search paths, you should print "command: Command not found." where command is the command name that was entered.

Build-in Commands

exit - obvious!
which - same as the tcsh one (hint: you can write a function to do this and use it for finding the command to execute) [Review get_path_main.c in shell skeleton code]
cd - chdir(2) to directory given; with no arguments, chdir to the home directory, with a '-' as the only argument, chdirs to directory previously in, the same as tcsh does.
pwd - print the current working directory.
list - with no arguments, lists the files in the current working directory one per line. With arguments, lists the files in each directory given as an argument, with a blank line then the name of the directory followed by a : before the list of files in that directory. You will need to use opendir(3) and readdir(3). (Hint: read their respective man pages carefully)
pid - prints the pid of the shell
kill - When given just a pid sends a SIGTERM to it with kill(2). When given a signal number (with a - in front of it) sends that signal to the pid. (e.g., kill 5678, kill -9 5678).
prompt - When run with no arguments, prompts for a new prompt prefix string. When given an argument make that the new prompt prefix. For instance, let's assume cwd is /usa/sllber.

printenv - Should work the same as the tcsh built-in one. When ran with no arguments, prints the whole environment. (This can be done in 2 lines of code, a printf() inside of a while loop, not counting a variable declaration). When ran with one argument, call getenv(3) on it. When called with two or more args print the same error message to stderr that tcsh does.
setenv - Should work the same as the tcsh built-in one. When ran with no arguments, prints the whole environment, the same as printenv. When ran with one argument, sets that as an empty environment variable. When ran with two arguments, the second one is the value of the first. When ran with more args, print the same error message to stderr that tcsh does. You can use the setenv(3) function for this command. Special care must be given when PATH and HOME are changed. When PATH is changed, be sure to update your linked list for the path directories (and free() up the old one). When HOME is changed cd with no arguments should now go to the new home. Provide your own test runs to show this works.
