This project uses LaTeX.
The recommended environment includes using Visual Studio Code with git from a linux environment.

To set it up on a linux environment with vscode installed:
1. $ sudo apt install texlive-full
(run the above on bash. This might take a while)
2. go to Extensions tab in vscode (Ctrl+Shift+X) and search for 'LaTeX Workshop' and install.
3. Reload vscode window.

How to use this environment:
1. The 'main' latex files (which include the rest) are called 'main.tex'.
2. The latex files will be compiled automatically when saving them, also you can run 'latex main.tex' to compile.
3. You can view the output .pdf file in vscode once you have 'LaTeX Workshop'.
4. It is recommended to split the vscode editor, where one side contains the '.tex' source and the other shows the output. To open split #2 (with default keybinds) press 'Ctrl+2'. Pressing 'Ctrl+1' and 'Ctrl+2' can also be used to quickly jump focus between splits.
