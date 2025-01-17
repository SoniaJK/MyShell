Project Authors: Sonia Kanchi & Arjun Anand

Designed and implemented a command-line shell in C, resembling bash/zsh that provides interactive and batch modes; Key features include POSIX stream IO, directory manipulation, process management, and inter-process communication

myShell takes up to one argument: 
1. When given one argument, it will read commands from the specified file. 
2. When given no argument, it will read commands from standard input.

When running in interactive mode, mysh will print a prompt to indicate that it is ready to read input.
The commands cd, pwd, which, and exit are implemented by mysh itself.

Functions of myShell:

check_exit_status():
- called to check if child processes ended 
- used in if state in then/else 

dic_traversal:
- traverses through pathnames and 3 given directories 
- checks for wildcards

void new_shell():
- prints mysh> 
- checks for pipes+redirections
- calls run_command()

Run command:
- checks for builtin; cd, pwd, which, exit
- calls on dic_traversal
//is wildcard handled here or only in dic_traversal??

Batch Mode:
- Opens and reads a file line by line
- splits each line into tokens and executes commands
- Executes commands from the file
//should it call 

Split line:
- takes string line and breaks it into tokens
- dynamically allocates memory to store the tokens in an array of character pointers (char **tokens)
- reallocates memory as needed to accommodate more tokens 

Main Method:
- handles interactive vs. Batch Mode

Notes:
- Hidden files: Patterns that begin with an asterisk, such as *.txt, will not match names that begin with a period
- Pipes and redirection can both override the standard input or output of a subprocess(in the event that both are specified, the redirection takes precedence)

Issues:
- handling wildcard in the last token of a pathname
- handling built in commands? (ls, cat, echo,...)
- pipeline overriding redirection


Testing:
1) then/else
  > then/else with no previous command, make sure it doesn't run
  > shoudln't execute if within a pipe: 
    > foo | else baz
2) built-in: cd, pwd, which, exit
  > handle exit if it recieves and arguments after it
3) pathnames in directories
4) pathnames w/ wildcards:
  > /user/bin/*.txt
5) Redirection 
  > cat < tesfile.txt (should print out)
6) Redirection w/ pipelines
7) pipes:
  > cat myscript.sh | ./mysh
8) make sure only accepts upto one argument
9) edge case: handle trailing whitespace
  "mysh> ls "

Batch mode with no specified file:
$ cat myscript.sh | ./mysh
hello
$

Interactive mode:
$ ./mysh
Welcome to my shell!
mysh> echo hello
hello
mysh> cd subsubdir
mysh> pwd
/current/path/subdir/subsubdir
mysh> cd directory_that_does_not_exist
cd: No such file or directory
mysh> cd ../..
mysh> exit
