# libCODY: COmpiler DYnamism<sup>a href="#1">1</a></sup>

Copyright (C) 2020 Nathan Sidwell, nathan@acm.org

libCODY is an implementation of a comminication protocol between
compilers and build systems.

**WARNING:**  This is preliminary software.

In addition to supporting C++modules, this may also support LTO
requirements and could also feed deal with generated #include files
and feed the compiler with prepruned include paths and whatnot.  (The
system calls involved in include searches can be quite expensive on
some build infrastuctures.)

* Client and Server objects
* Direct connection for in-process use
* Testing with Joust (that means nothing to you, doesn't it!)


## Problem Being Solved

The origin is in C++20 modules:
```
import foo;
```

At that import, the compiler needs<sup><a href="#2">2</a></sup> to
load up the compiled serialization of module `foo`.  (a) where is that
file and (b) does it even exist?  Unless the build system already
knows the dependency graph, this might be a completely unknown module.
Now, the build system knows how to build things, but it might not have
complete information about the dependencies.  The ultimate source of
dependencies is the source code being compiled, and specifying the
same thing in multiple places is a recipe for build skew.

Hence, a protocol by which a compiler can query a build system.  This
was originally described in <a
href="https://wg21.link/p1184r1">p1184r1:A Module Mapper</a>.  Along
with a proof-of-concept hack in GNUmake, described in <a
href="https://wg21.link/p1602">p1602:Make Me A Module</a>. The current
implementation has evolved and an update to p1184 will be forthcoming.

## Packet Encoding

The protocol is transactional.  The compiler sends a block of one or
more requests to the builder, then waits for a block of responses to
all of those requests.  If the builder needs to compile something to
satisfy a request, there may be some time before the response.  A
builder may service multiple compilers concurrently, and it'll need
some buffering scheme to deal with that.

When multiple requests are in a transaction, the responses will be in
corresponding order.

Every request has a response.

Requests and responses are user-readable text.  It is not intended as
a transmission medium to send large binary objects (such as compiled
modules).  It is presumed the builder and the compiler share a file
system, for that kind of thing.

Messages characters are encoded in UTF8.

Messages are a sequenc of octets ending with a NEWLINE (0xa).  The lines
consist of a sequence of words, separated by WHITESPACE (0x20 or 0x9).
Words themselves do not contain WHITESPACE.  Lines consisting solely
of WHITESPACE (or empty) are ignored.

To encode a block of multiple messages, non-final messages end with a
single word of SEMICOLON (0x3b), immediately before the NEWLINE.  Thus
a serial connection can determine whether a block is complete without
decoding the messages.

Words containing characters in the set [-+_/%.A-Za-z0-9] need not be
quoted.  Words containing characters outside that set should be quoted.

Quoted words begin and end with APOSTROPHE (x27). Within the quoted
word, BACKSLASH (x5c) is used as an escape mechanism, with the
following meanings:

* NEWLINE: \n
* TAB: \t
* SPACE: \_
* APOSTROPHE: \'
* BACKSLASH: \\

Characters in the range [0x00, 0x20) and 0x7f are encoded with one or
two lowercase hex characters.  Octets in the range [0x80,0xff) are
UTF8 encodings and passed as such.

Decoding should be more relaxed.  Unquoted words containing characters
in the range [0x20,0xff] other than BACKSLASH or APOSTROPHE should be
accepted.  In a quoted sequence \ followed by one or two lower case
hex characters decode to that octet.  Further, words can be
constructed from a mixture of abutted quoted and unquoted sequences.
For instance `FOO'\_'bar` would decode to the word `FOO bar`.

Notice that the block continuation marker of `;` is not a valid
encoding of the word `;`, which would be `';'`.

It is recommended that words are separated by single SPACE characters.

## Messages

The message descriptions use `$metavariable` examples.

All messages may result in an error response:

`ERROR $message`

The message is a human-readable string.

### Handshake

The first message is a handshake:

`HELLO $version $compiler $ident`

The `$version` is a numeric value, currently `.  `$compiler` identifies
the compiler &mdash; builders may need to keep compiled modules from
different compilers separate.  `$ident` is an identifer the builder
might use to identify the compilation it is communicting with.

Responses are:

`HELLO $version $builder`

A successful handshake.  The communication is now connected and other
messages may be exchanged.

`ERROR $message`

An unsuccesful handshake.  The communication remains unconnected.

There is nothing restricting a handshake to its own message block.  Of
course, if the handshake fails, subsequent non-handshake messages in
the block will fail (producing an error response).

### C++ Module Messages

A set of messages are specific to C++ modules

#### Repository

* `MODULE-REPO`

Request the module repository location (if any).  The expected response is:

`MODULE-REPO $directory`

All non-absolute CMI file names are relative to the repository.  (CMI
file names are usually relative names.)

#### Exporting

A compilation of a module interface, partition or header unit can
inform the builder with:

`MODULE-EXPORT $module`

This will result in a response naming the Compiled Module Interface
file to write:

`MODULE-CMI $cmi`

The `MODULE-EXPORT` request does not indicate the module has been
successfully compiled.  At most one `MODULE-EXPORT` is to be made, and
as the connection is for a single compilation, the builder may infer
dependency relationships between the module being generated and import
requests made.

Successful compilation of an interface is indicated with a subsequent:

Named module names and header unit names are distinguished by making
the latter unambiguously look like file names.  Firstly, they must be
fully resolved according to the compiler's usual include path.  If
that results in an absolute name file name (beginning with `/`, or
certain other OS-specific sequences), all is well.  Otherwise a
relative file name must be prefixed by `./` to be distinguished from a
similarly named named module.  This prefixing must occur, even if the
header-unit's name contains characters that cannot appear in a named
module's name.

`MODULE-COMPILED $module`

FIXME: do we need the module here?

request.  This indicates the CMI file has been written to disk, so
that any other compilations waiting on it may proceed.  A single
response:

`OK`

is expected.  Compilation failure can be inferred by lack of a
`MODULE-COMPILED` request.  It is presumed the builder can determine
this, as it is also responsible for launching and reaping the compiler
invocations themselves.

#### Importing

Importation, inculding that of header-units, uses:

`MODULE-IMPORT $module`

This response with a `MODULE-CMI` of the same form as the
`MODULE-EXPORT` request.  Should the builder have to invoke a
compilation to produce the CMI, the response should be delayed until
that occurs.  If such a compilation fails, an error response should be
provided to the requestor &mdash; which will then presumably fail in
some manner.

#### Include Translation

Include translation can be determined with:

`INCLUDE-TRANSLATE $header`

The header name, `$header`, is the fully resolved header name, in the
above-mentioned unambigous filename form.  The response will be:

`INCLUDE-TEXT`

to indicate include translation should not occur (the usual textual
inclusion occurs).  Or:

`INCLUDE-IMPORT`

to indicate the include directive should be replaced by an import
declaration of the resolved header-unit.  Finally `MODULE-CMI`
response also indicates include translation should occur, and provides
the name of the CMI to read, this possibly elides a subsequent
`MODULE-IMPORT` request.

## Classes

FIXME:

### Client

### Server

### Resolver

### Packet

## Helpers

# Future Directions

* Current Directory syncing?  There is no mechanism to check the
builder and the compiler have the same working directory.  Perhaps
that should be addressed.

* Include path canonization and/or header file lookup

* Generated header file lookup/construction

* Link-time compilations


<a name="1">1</a>: or a small town in Wyoming <a name="2">2</a>: this
describes one common implementation technique.  The std itself doesn't
require such serializations, but the ability to create them is kind of
the point.  Also, 'compiler' is used where we mean any consumer of a
module, and 'build system' where we mean any producer of a module.
