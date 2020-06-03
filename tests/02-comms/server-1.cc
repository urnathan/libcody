
// Test server message round tripping
/*
  RUN:<<HELLO 1 TEST IDENT ;
  RUN:<<MODULE-REPO ;
  RUN:<<MODULE-EXPORT bar ;
  RUN:<<MODULE-IMPORT foo ;
  RUN:<<NOT A COMMAND ;
  RUN:<<INCLUDE-TRANSLATE baz.frob ;
  RUN:<<INCLUDE-TRANSLATE ./quux ;
  RUN:<<MODULE-COMPILED bar ;
  RUN:<<MODULE-IMPORT ''
*/
// RUN: $subdir$stem | ezio -p OUT1 $src |& ezio -p ERR1 $src

// These all fail because there's nothing in the server interpretting stuff
/*
  OUT1-NEXT: ^HELLO 1 default	;
  OUT1-NEXT: ^MODULE-REPO cmi.cache	;
  OUT1-NEXT: ^MODULE-CMI bar.cmi	;
  OUT1-NEXT: ^MODULE-CMI foo.cmi	;
  OUT1-NEXT: ^ERROR 'unrecognized\_\'NOT\_
  OUT1-NEXT: ^INCLUDE-TEXT	;
  OUT1-NEXT: ^INCLUDE-TEXT	;
  OUT1-NEXT: ^OK
  OUT1-NEXT: ^ERROR 'malformed\_
*/
// OUT1-NEXT:$EOF
// ERR1-NEXT:$EOF

/*
  RUN:<<HELLO 1 TEST IDENT
  RUN:<<MODULE-REPO
*/
// RUN: $subdir$stem | ezio -p OUT2 $src |& ezio -p ERR2 $src
/*
  OUT2-NEXT: ^HELLO 1 default
*/
// OUT2-NEXT:$EOF
// ERR2-NEXT:$EOF

// RUN-END:

// Cody
#include "cody.hh"
// C++
#include <iostream>

using namespace Cody;

int main (int, char *[])
{
  Resolver r;
  Server server (&r, 0, 1);

  while (int e = server.Read ())
    if (e != EAGAIN && e != EINTR)
      break;

  server.ProcessRequests ();
  if (server.GetResolver () != &r)
    std::cerr << "resolver changed\n";
  server.PrepareToWrite ();

  while (int e = server.Write ())
    if (e != EAGAIN && e != EINTR)
      break;
}
