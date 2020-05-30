
// Test server message round tripping
/*
  RUN:<<HELLO 0 TEST IDENT \
  RUN:<<MODULE-REPO \
  RUN:<<MODULE-EXPORT bar \
  RUN:<<MODULE-IMPORT foo \
  RUN:<<NOT A COMMAND \
  RUN:<<INCLUDE-TRANSLATE baz.frob \
  RUN:<<INCLUDE-TRANSLATE ./quux \
  RUN:<<MODULE-COMPILED bar
*/
// RUN: $subdir$stem | ezio -p OUT $src |& ezio -p ERR $src
// RUN-END:

// These all fail because there's nothing in the server interpretting stuff
/*
  OUT-NEXT: ^ERROR 'unknown request'	\
  OUT-NEXT: ^ERROR 'unknown request'	\
  OUT-NEXT: ^ERROR 'unknown request'	\
  OUT-NEXT: ^ERROR 'unknown request'	\
  OUT-NEXT: ^ERROR 'unrecognized request 
  OUT-NEXT: ^ERROR 'unknown request'	\
  OUT-NEXT: ^ERROR 'unknown request'	\
  OUT-NEXT: ^ERROR 'unknown request'
*/
// OUT-NEXT:$EOF

// ERR-NEXT:$EOF


// Cody
#include "cody.hh"
// C++
#include <iostream>

using namespace Cody;

int main (int, char *[])
{
  Server server (0, 1);

  while (int e = server.Read ())
    if (e != EAGAIN && e != EINTR)
      break;

  if (server.ParseRequests ())
    server.ProcessRequests ();
  server.WriteResponses ();

  while (int e = server.Write ())
    if (e != EAGAIN && e != EINTR)
      break;
}
