# Myshell
An unix shell running on Ubuntu 14.04.
This is a C program that acts as an interactive shell program, named as myshell.
This program supports the following features:
1. The program (myshell) accepts command from user and executes the corresponding program with the given argument list.
2. It is able to locate and execute any valid program (i.e. compiled programs) by giving an absolute path (starting with /) or a relative path (starting with ./) or by searching directories under the $PATH environment variable.
3. It has three built-in commands:
   exit command that terminates the myshell program; once the program starts, it continuously accepts commands from user until receiving the exit command.
   timeX command that prints out the process statistics of a terminated child process.
4. It supports two operators: & (run as background job) and | (pipe).
5. The myshell process can not be terminated by Cltr-c (SIGINT signal). 
It's a practise of processes controlling and signal handling of linux operating system.
