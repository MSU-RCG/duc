
[![Build Status](https://travis-ci.org/zevv/duc.svg?branch=master)](https://travis-ci.org/zevv/duc)

![Duc](/img/duc.png) 


### Introduction

Duc is a small library and a collection of tools for inspecting and visualizing
disk usage. 

Duc maintains a database of accumulated sizes of directories of your file
system, and allows you to query this database with some tools, or create fancy
graphs showing you where your bytes are.

Duc scales quite well, it has been tested on systems with more then 500 million
files and several petabytes of storage. 

![Example](/img/example.png) 


### Install

Duc depends on the Tokyo Cabinet [1] database library, and on Cairo [2] and
Pango [3] for writing graphs. The ncurses library is required for the curses
user interface.

1. http://fallabs.com/tokyocabinet/
2. http://cairographics.org/
3. http://www.pango.org/

Duc uses the GNU Autoconf system for compiling if you do not have a
pre-built package. 

Building and installing on Debian or Ubuntu:

```
$ sudo apt-get install libncurses5-dev libcairo2-dev libpango1.0-dev libtokyocabinet-dev libtool autoconf automake build-essential
$ autoreconf --install
$ ./configure
$ make
$ sudo make install
$ sudo ldconfig
```

On RHEL or CentOS systems, you need to do:

```
$ sudo yum install pango-devel cairo-devel tokyocabinet-devel libtool
$ autoreconf --install
$ ./configure
$ make
$ sudo make install
$ sudo ldconfig
```


### Usage

Duc comes with a command line tool called `duc`, which is used to create,
maintain and query the disk usage database.  run `duc help` to get a list of
available commands. `duc help <subcommand>` describes the usage of a specific
subcommand.


#### A note on file size and disk usage

The concepts of 'file size' and 'disk usage' can be a bit confusing. Files on
disk have an apparent size, which indicates how much bytes are in the file from
the users point of view; this is the size reported by tools like `ls -l`. The
apparent size can be any number, from 0 bytes up to several TB.  The actual
number of bytes which are used on the filesystem to store the file can differ
from this apparent size for a number of reasons: disks store data in blocks,
which cause files to always take up a multiple of the block size, files can
have holes ('sparse' files), and other technical reasons. This number is always
a multiple of 512, which means that the actual size used for a file is almost
always a bit more then its apparent size.

Duc has two modes for counting file sizes:

- 'apparent size': this is the size as reported by `ls`. This number indicates
  the file length, which is usually smaller then the actual disk usage. In this
  mode, directories themselves do not have a size.

- 'actual size': this is the size as reported by `du` and `df`. The actual file
  size tells you how much disk is actually used by a file, and is alwasys a
  multiple of 512 bytes. In this mode, the blocks used to store the directory
  information are counted as well.

The default mode used by duc is to use the 'actual size'. Most duc commands to
report disk usage (`duc ls`, `duc graph`, `duc gui`, etc) have an option to
change between these two modes (usually the `-a`), in the gui tool use the 'a'
key to toggle.


#### Options and configuration

Each duc subcommand can take a number of configuration options to alter its
behaviour; run `duc help <subcommand>` to find out the available options for
each subcommand.

Duc allows any option to be placed either on the command line or in a
configuration file. Options on the command line are preceded by a
double-leading-dash ("--option"), some options have a corresponding short
option which can be used as well with a single leading dash. ("-o")

At startup duc tries to read its configuration from three locations in this
particular order: `/etc/ducrc`, `~/.ducrc` and `./.ducrc`.

The configuration file consists of sections and parameters. The section names
correspond to the duc subcommands for which the parameters in that section
apply. A section begins with the name of the section in square brackets and
continues until the next section begins.Sections contain parameters, one per
line, which consist of a single option name for boolean flags, or a option name
and a value for options which take a value.

The following sample configuration defines default parameters for the `duc ls`
and `duc gui` commands.

```
[ls]
recursive
classify
color

[gui]
fuzz 0.7
palette rainbow
levels 4
```

#### Creating the index

Duc needs an index file of the file system before it is able to show any
information.  To create the index, run the `duc index` command. For example, to
create an index of your home directory run `duc index ~`

```
$ duc index /usr
Skipping lost+found: Permission denied
Indexed 333823 files and 48200 directories, (35.0GB total) in 1 seconds
```

The default location of the database is `$HOME/.duc.db`. To use a different
database location, use the DUC_DATABASE environment variable or specify the
database location with the --database argument.

You can run `duc index` at any time later to rebuild the index.


#### Querying the index

`duc info` shows a list of available directory trees in the database, and the time
and date of the last scan.

```
Available indices:
 2014-05-14 19:06:27   27.7G /var
 2014-05-14 19:06:30   35.0G /usr
 2014-05-14 19:06:49    6.3G /
```

Use the `duc ls` command to see the disk usage of a directory. A specific path
can be specified as command line argument, the current directory is used if omitted.

`dus ls` has some options similar to the normal `ls` program. For example, try
`duc ls -Fcg` for a more friendly output.

```
$ duc ls -Fcg
  4.7G lib/                 [+++++++++++++++++++++++++++++++++++++++++++]
  3.1G share/               [++++++++++++++++++++++++++++               ]
  2.7G src/                 [++++++++++++++++++++++++                   ]
814.9M bin/                 [+++++++                                    ]
196.6M include/             [+                                          ]
 66.6M x86_64-w64-mingw32/  [                                           ]
 59.9M local/               [                                           ]
 38.8M i686-w64-mingw32/    [                                           ]
 20.3M sbin/                [                                           ]
 13.6M lib32/               [                                           ]
 13.3M libx32/              [                                           ]
```

or use -RF for the tree view:

```
$ duc ls -RF /etc/logcheck
  1.1K ╰┬─ ignore.d.server
   653  │  ╰┬─ rsyslog
   202  │   ├─ hddtemp
   115  │   ├─ ntpdate
    89  │   ├─ lirc
    79  │   ╰─ libsasl2-modules
    89  ├─ ignore.d.workstation
    89  │   ╰─ lirc
    89  ╰─ ignore.d.paranoid
    89      ╰─ lirc
```

For a graphical representation of the disk usage, use the command `duc graph`.
This will create a .png image with the graph of the requested directory.


#### Graphical user interface

Run `duc gui` for an interactive graphical overview of your disk usage. Duc
represents files and directories in a sunburst graph, which can be navigated by
mouse: clicking on a directory redraws the graph from the perspective of the
selected directory. Click in the center of the graph to go up one directory in
the tree.


Mouse buttons:
```
left        descent into directory
right       go up one directory
wheel       set graph depth
```
Key bindings:

```
+           increase maximum graph depth
-           decrease maximum graph depth
0           Set default graph depth
a           Toggle between apparent and actual disk usage
p           toggle palettes
f           toggle graph fuzz
backspace   cd ..
```

![size](img/palette-size.png)
![rainbow](img/palette-rainbow.png)
![greyscale](img/palette-greyscale.png)
![monochrome](img/palette-monochrome.png)


#### Ncurses console user interface

Run 'duc ui' for the interactive console browser.

Key bindings:

```
k, up, pgup:     move cursor up
j, down, pgdn:   move cursor down
h, left:         go up to parent directory (..)
l, right, enter: descent into selected directory
a:               toggle between actual and apparent disk usage
b:               toggle between exact and abbreviated sizes
g:               toggle graph
q, escape:       quit
```

![ui](img/ui.png)


#### CGI interfacing

The `duc` binary has support for a rudimentary CGI interface, currently only
tested with apache.  The CGI interface generates a simple HTML page with a list
of indexed directories, and shows a clickable graph for navigating the file
system.

Configuration is done by creating a simple shell script as .cgi in a directory
which is configured for CGI execution by your web server (usually
`/usr/lib/cgi-bin`). The shell script should simply start duc, and pass the
location of the database to offer.

An example duc.cgi script would be

```
#!/bin/sh
/usr/local/bin/duc cgi -d /home/jenny/.duc.db
```

* Make sure the database file is readable by the user (usually www-data)
* Debuggin is best done by inspecting the web server's error log
* Make sure the .cgi script has execute permissions (`chmod +x duc.cgi`)

The current CGI configuration is not very flexible, nor secure. Use at your own
risk!


#### TL;DR

```
$ duc help
$ duc help index
$ duc info
$ duc index /usr
$ duc ls /usr/bin
$ duc gui /usr
$ duc graph -o /tmp/usr.png /usr
```


### History

Duc is the replacement for Philesight[1], which I wrote a few years ago but has
some shortcomings (slow indexing, large database) which I felt were not simple
to fix.

Instead of Ruby, Duc is written in plain C, which is probably as fast as it
will get. Duc is about ten times faster then Philesight when indexing, with a
database size which is about eight times smaller.

1. http://zevv.nl/play/code/philesight/


### Authors

* Ico Doornekamp <duc@zevv.nl>
* John Stoffel <john@stoffel.org>


### License

This package is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 dated June, 1991. This package is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

