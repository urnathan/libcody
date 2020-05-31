// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// Test message encoding, both string quoting and continuation lines

// RUN: $subdir$stem |& ezio $src
// RUN-END:
// The ¯ is utf8-encoded as c2 af
// CHECK-NEXT: ^bob 'frob\20dob''\n¯\\'$
// CHECK-NEXT: ^2 \$
// CHECK-NEXT: ^3$
// CHECK-NEXT: $EOF

// Cody
#include "cody.hh"

using namespace Cody;

int main (int, char *[])
{
  MessageBuffer writer;

  writer.BeginLine ();
  writer.AppendWord ("bob");
  writer.AppendWord ("frob dob", true);
  writer.Append ("\n\xc2\xaf\\", true);
  writer.EndLine ();

  writer.PrepareToWrite ();
  while (int err = writer.Write (2))
    if (err != EAGAIN && err != EINTR)
      break;

  writer.BeginLine ();
  writer.Append ("2", true);
  writer.EndLine ();
  writer.BeginLine ();
  writer.Append ("3", true);
  writer.EndLine ();

  writer.PrepareToWrite ();
  while (int err = writer.Write (2))
    if (err != EAGAIN && err != EINTR)
      break;

  return 0;
}
