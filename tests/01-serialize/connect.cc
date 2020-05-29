
// Test client connection handshake
// RUN: <<HELLO 0 TESTING REPO
// RUN: $subdir$stem | ezio -p OUT $src |& ezio -p ERR $src
// RUN-END:

// OUT-NEXT:^HELLO {:[0-9]+} TEST IDENT$
// OUT-NEXT:$EOF

// ERR-NEXT:Code:{:[0-9]+}$
// ERR-NEXT:Repo:REPO$
// ERR-NEXT:$EOF


// Cody
#include "cody.hh"
// C++
#include <iostream>

using namespace Cody;

int main (int, char *[])
{
  Client client;

  client.OpenFDs (0, 1);
  auto token = client.Connect ("TEST", "IDENT");

  std::cerr << "Code:" << token.GetCode () << '\n';
  std::cerr << "Repo:" << token.GetString () << '\n';
  
}
