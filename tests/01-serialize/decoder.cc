// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// RUN: <<bob 'frob dob''\nF\61\\'
// RUN: $subdir$stem |& ezio $src
// CHECK-NEXT: ^line:0 token:0 'bob'
// CHECK-NEXT: ^line:0 token:1 'frob dob$
// CHECK-NEXT: ^Fa\'$
// CHECK-NEXT: $EOF

/* RUN: <<line-1 token:1 \
   RUN: <<'line 2' \
   RUN: <<
*/
// RUN: $subdir$stem |& ezio -p CHECK2 $src
// CHECK2-NEXT: line:0 token:0 'line-1'
// CHECK2-NEXT: line:0 token:1 'token:1'
// CHECK2-NEXT: line:1 token:0 'line 2'
// CHECK2-NEXT: $EOF

// RUN: <<'
// RUN: $subdir$stem |& ezio -p CHECK3 $src
// CHECK3-NEXT: error:
// CHECK3-NEXT: line:0 token:0 '''
// CHECK3-NEXT: $EOF
// RUN-END:

// Cody
#include "cody.hh"
// C
#include <cstdlib>
#include <cstdio>
#include <cstring>

using namespace Cody;

int main (int, char *[])
{
  MessageBuffer reader;

  while (int e = reader.Read (0))
    if (e != EAGAIN && e != EINTR)
      break;

  std::vector<std::string> tokens;
  for (unsigned line = 0; ; line++)
    {
      int e = reader.Tokenize (tokens);
      if (e < 0)
	break;
      if (e)
	fprintf (stderr, "error:%s\n", strerror (e));
      for (unsigned tok = 0; tok != tokens.size (); tok++)
	{
	  auto &token = tokens[tok];
	  
	  fprintf (stderr, "line:%u token:%u '%.*s'\n",
		   line, tok, int (token.size ()), token.c_str ());
	}
    }
  return 0;
}
