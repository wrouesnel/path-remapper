# Path-Remap

`path-remap` provides VFS path-remapping functionality. This module was developed in order to handle dealing with
`nix` source builds on disconnected hosts, where `nixpkgs` build profiles may be badly behaved but the administrators
do not have the authority nor desire to modify the underlying build scripts and instead need to patch the build process
in place.

To this end the module is designed to provide per-process, per-syscall level path remapping via basic rules.

## Usage

The module creates a chardev `/dev/path-remap` which accepts ioctls to configure the remapping of paths. A path remap
specification is:

```
<pid>:<process-name>:<process sha256sum>:<source path>:<dest path>
```



## Security Implications

The functionality of this module is very similar to that of a rootkit, only it doesn't try to hide. It is however a good
example of why Nix builds need to be performed on trusted machines, by trusted users, and *signed* - since the build
output can be entirely manipulated by this sort of module and Nix only tracks inputs.

In this case, the intent and purpose of the module is to do things like replace TLS certificates for remote calls.