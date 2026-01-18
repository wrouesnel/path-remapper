# path-remapper

`path_remapper` is a Linux Kernel Module (LKM) which provides utilities to
dynamically intercept and redirect paths in the linux virtual filesystem
system-wide.

The intended purpose of this module is to allow for an extremely low level 
modification of the view of a filesystem by processes which do not otherwise 
provide adequate configuration support: notably the intent of its development
was to allow re-mapping the paths which certain build systems look for TLS
CA certificates.

## How does it work?

This module employs certain root-kit like techniques to hook and patch the system
call tables using ftrace. This in every way, a hack although it does not attempt
to hide itself and you can just unload the module. Once installed, attempts to
open configured paths will instead be redirected to their alternatives.

## Configuration

Because the module hooks VFS syscalls, configuration is provided by a detected
file on the filesystem, defaulting to `/etc/path_remapper`. When this file
exists, and it's contents are valid, the module will update it's configuration
everytime the file is closed with an open write command (this is ultimately
just mimicking inotify).

### Configuration Format

The module is configured with a plain text table with the following format:

```
<delimiter><prefix><delimiter><filepath glob><delimiter><remapped path><delimiter><options><delimiter><process exe glob><delimiter><process commandline glob><delimiter>\n
```

The file is line-delimited, but configuration is dynamically delimited. The delimiter for any given line is determined
by the first character on that line, and must also be the last character before the newline. e.g.

```
:/nix/:/nix/**/cacerts:/etc/ssl/certs/java/cacerts:read:*:cat:

```

The above configuration states that any file under `/nix/` should be pattern matched for configuration, and then the
absolute path glob matched for `cacerts` and `open` and `stat` commands redirected to `/etc/ssl/certs/java/cacerts`...
but only if the process cmdline is `cat` for read operations only.

Explanation of parameters:

* `delimiter` should be a single ASCII character which will be used to split up the fields of the subsequent line.
* `prefix` the fixed prefix that the given line should match. This is used to quickly discard pattern matching on filepaths.
* `filepath glob` a glob matching statement which resolves to the absolute path of a file to be remapped to the target.
* `remapped target` an absolute path to the file to redirect the path to
* `options` is a list of options to support for the path. See below.
* `process exe glob` a glob which matches the path of the executable image if this remap should be applied
* `process commandline glob` a glob which matches the command line (as seen in `/proc/cmdline`) for the process for this remap to be applied

### Options

There are a number of possible modes of path-remapping which affect what calls are redirected. The default is simply
`read`: any call to open the file for reading causes the syscall to be redirected to the target. This also redirects
stat to return the size of the target. In default `read` mode, the matched path must actually exist in order for the
redirect to succeed.

`write` mode will also allow the target file to be opened for writing. In the intended use-case this is probably not
what you want. When `write` is _disabled_, opening a matched file will be opened as normal for writing. This *includes*
read-write mode, so it is possible for a process to open a file for read-write as normal but have the content replaced
on closed with the remap.

`allow_no_exist` mode does not require the target file to exist to be opened. This effectively means a remap is providing 
a hidden path which will always exist (provided the target does).

It should be noted that the consequences of many option combinations on a process are hard to predict: what's happening
is some very weird magic under the hood, which is hopefully really just like if you had a race condition for file
writes.