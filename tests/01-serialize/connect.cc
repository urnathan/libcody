
// Test client connection handshake
// RUN: <<HELLO 0 TESTING
// RUN: $subdir$stem | ezio -p OUT $src |& ezio -p ERR $src
// RUN-END:

// OUT-NEXT:^HELLO {:[0-9]+} TEST IDENT$
// OUT-NEXT:$EOF

// ERR-NEXT:Code:{:[0-9]+}$
// ERR-NEXT:Version:0$
// ERR-NEXT:Agent:TESTING$
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
  auto const &v = token.GetVector ();
  std::cerr << "Version:" << v[0] << '\n';
  std::cerr << "Agent:" << v[1] << '\n';
  
}
