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
href="https://wg21.link/p1602: Make Me A Module</a>. The current
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

The message descriptions use `$metavariable` examples

### Handshake

The first message is a handshake:

`HELLO $version $compiler $ident`

The `$version` is a numeric value, currently `.  `$compiler` identifies
the compiler -- builders may need to keep compiled modules from
different compilers separate.  `$ident` is an identifer the builder
might use to identify the compilation it is communicting with.

Responses are:

`HELLO $version $builder`

A successful handshake.  The communication is now connected and other
messages may be exchanged.

`ERROR $message`

An unsuccesful handshate.  The communication remains unconnected.

### C++ Module Packets

`MODULE-REPO`

`MODULE-EXPORT $module`

`MODULE-IMPORT $module`

`MODULE-COMPILED $module`

`INCLUDE-TRANSLATE $header`

## Classes

FIXME:

### Client

### Server

### Resolver

### Packet

## Helpers

<a name="1">1</a>: or a small town in Wyoming <a name="2">2</a>: this
describes one common implementation technique.  The std itself doesn't
require such serializations, but the ability to create them is kind of
the point.  Also, 'compiler' is used where we mean any consumer of a
module, and 'build system' where we mean any producer of a module.
