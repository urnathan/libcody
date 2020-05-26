// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// RUN: $subdir$stem |& ezio $src
// RUN-END:
// CHECK-NEXT: ^bob 'frob dob''\n\80\\'$
// CHECK-NEXT: ^+2$
// CHECK-NEXT: ^-3$
// CHECK-NEXT: $EOF

// Cody
#include "cody.hh"
// C
#include <cstdlib>

using namespace Cody;

int main (int , char *[])
{
  MessageBuffer writer;

  writer.Append ("bob");
  writer.Space ();
  writer.Append ("frob dob", true);
  writer.Append ("\n\x80\\", true);
  writer.Eol ();
  writer.Eom ();
  while (!writer.Write (2))
    continue;

  writer.Append ("2", true);
  writer.Eol ();
  writer.Append ("3", true);
  writer.Eol ();
  writer.Eom ();
  while (!writer.Write (2))
    continue;

  return 0;
}
