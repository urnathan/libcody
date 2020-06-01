// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

#include "cody.hh"

#define CODY_LOC_BUILTINS (__GNUC__ >= 10)
// C++
#if !CODY_LOC_BUILTINS
#include <source_location>
#endif
// C
#include <cstdio>

namespace Cody {

#if CODY_CHECKING
class Location
{
protected:
  char const *file;
  unsigned line;

public:
  constexpr Location (char const *file_
#if CODY_LOC_BUILTINS
		      = __builtin_FILE ()
#endif
		      , unsigned line_
#if CODY_LOC_BUILTINS
		      = __builtin_LINE ()
#endif
		      )
    :file (file_), line (line_)
  {
  }

#if !CODY_LOC_BUILTINS
  constexpr Location (source_location loc == source_location::current ())
    :file (loc.file ()), line (loc.line ())
  {
  }
#endif

public:
  constexpr char const *File () const
  {
    return file;
  }
  constexpr unsigned Line () const
  {
    return line;
  }
};
#endif

void HCF [[noreturn]]
(
 char const *msg
#if CODY_CHECKING
 , Location const = Location ()
#endif
 ) noexcept;

#if CODY_CHECKING
void AssertFailed [[noreturn]] (Location loc = Location ());
void Unreachable [[noreturn]] (Location loc = Location ());

// Oh for lazily evaluated function parameters
#define Assert(EXPR, ...)						\
  (__builtin_expect (bool (EXPR __VA_OPT__ (, __VA_ARGS__)), true)	\
   ? (void)0 : AssertFailed ())
#else
#define Assert(EXPR, ...)					\
  ((void)sizeof (bool (EXPR __VA_OPT__ (, __VA_ARGS__))), (void)0)
inline void Unreachable ()
{
  __builtin_unreachable ();
}
#endif

// FIXME: This should be user visible in some way
void BuildNote (FILE *stream) noexcept;

}
