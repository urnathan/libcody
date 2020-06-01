// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2019-2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// Cody
#include "internal.hh"
// C
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Cody {

#if CODY_CHECKING
#if !CODY_LOC_MACRO
void AssertFailed (Location loc)
{
  HCF ("assertion failed", loc);
}
void Unreachable (Location loc)
{
  HCF ("unreachable reached", loc);
}
#else
void (AssertFailed) (char const *file_, unsigned line_)
{
  (HCF) ("assertion failed", file_, line_);
}
void (Unreachable) (char const *file_, unsigned line_)
{
  (HCF) ("unreachable reached", file_, line_);
}
#endif
#endif

void (HCF) (char const *msg
#if CODY_CHECKING
#if !CODY_LOC_MACRO
	  , Location const loc
#else
	  , char const *file_, unsigned line_
#endif
#endif
	  ) noexcept
{ // HCF - you goofed!
  __asm__ volatile ("nop");  // HCF - you goofed!

#if !CODY_CHECKING
  Location loc (nullptr, 0);
#elif CODY_LOC_MACRO
  Location loc (file_, line_);
#endif

  fprintf (stderr, "CODYlib: %s", msg ? msg : "internal error");
  if (char const *file = loc.File ())
    {
      char const *src = SRCDIR;

      if (src[0])
	{
	  size_t l = strlen (src);

	  if (!strncmp (src, file, l) && file[l] == '/')
	    file += l + 1;
	}
      fprintf (stderr, " at %s:%u", file, loc.Line ());
    }
  fprintf (stderr, "\n");
  raise (SIGABRT);
  exit (2);
}

void BuildNote (FILE *stream) noexcept
{
  fprintf (stream, "Version %s.\n", PACKAGE_NAME " " PACKAGE_VERSION);
  fprintf (stream, "Report bugs to %s.\n", BUGURL[0] ? BUGURL : "you");
  if (PACKAGE_URL[0])
    fprintf (stream, "See %s for more information.\n", PACKAGE_URL);
  if (REVISION[0])
    fprintf (stream, "Source %s.\n", REVISION);

  fprintf (stream, "Build is %s & %s.\n",
#if !CODY_CHECKING
	   "un"
#endif
	   "checked",
#if !__OPTIMIZE__
	   "un"
#endif
	   "optimized");
}

}
