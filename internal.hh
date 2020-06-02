// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

#include "cody.hh"

// C++
#if __GNUC__ >= 10
#define CODY_LOC_BUILTIN 1
#elif __has_include (<source_location>)
#include <source_location>
#define CODY_LOC_SOURCE 1
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
#if CODY_LOC_BUILTIN
		      = __builtin_FILE ()
#elif !CODY_LOC_SOURCE
		      = nullptr
#endif
		      , unsigned line_
#if CODY_LOC_BUILTIN
		      = __builtin_LINE ()
#elif !CODY_LOC_SOURCE
		      = 0
#endif
		      )
    :file (file_), line (line_)
  {
  }

#if !CODY_LOC_BUILTIN && CODY_LOC_SOURCE
  constexpr Location (source_location loc = source_location::current ())
    : Location (loc.file (), loc.line ())
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
#if !CODY_LOC_BUILTIN && !CODY_LOC_SOURCE
#define HCF(M) HCF ((M), Cody::Location (__FILE__, __LINE__))
#endif
#endif
 ) noexcept;

#if CODY_CHECKING
void AssertFailed [[noreturn]] (Location loc = Location ());
void Unreachable [[noreturn]] (Location loc = Location ());
#if !CODY_LOC_BUILTIN && !CODY_LOC_SOURCE
#define AssertFailed() AssertFailed (Cody::Location (__FILE__, __LINE__))
#define Unreachable() Unreachable (Cody::Location (__FILE__, __LINE__))
#endif

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
