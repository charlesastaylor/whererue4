# WhereRUE4

Command line tool to find Unreal Engine 4 installations.

To build: run `build.cmd` (under a vs developer command prompt)

Usage: `whererue4.exe [version]`

- with no argument will print all found engine installs - both launcher and source builds
- specifying a version will print out the location of that specific (launcher) version, eg `whererue4.exe 4.27`

NB: Has approximately zero error handling. Don't look at me.