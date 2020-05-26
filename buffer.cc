// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// Cody
#include "internal.hh"
// C++
#include <algorithm>
// C
#include <cstring>
// OS
#include <unistd.h>
#include <cerrno>

namespace Cody {

// QUOTE means 'maybe quote', we search it for quote-needing chars

void MessageBuffer::Append (char const *str, bool quote, size_t len)
{
  if (len == ~size_t (0))
    len = strlen (str);

  if (!len && !quote)
    return;

  if (!buffer.size () || buffer.back () == '\n')
    {
      // First character on a new line.
      if (buffer.size ())
	{
	  // Previous line will continue
	  Assert (buffer[lastBol] == (lastBol ? '-' : ' '));
	  buffer[lastBol] = '+';
	}

      lastBol = buffer.size ();
      // Space for current line to continue
      buffer.push_back (lastBol ? '-' : ' ');
    }

  // We want to quote characters outside of [-+_A-Za-z0-9/%.], anything
  // that could remotely be shell-active.  UTF8 encoding for non-ascii.
  if (quote && len)
    {
      quote = false;
      for (size_t ix = len; ix--;)
	{
	  unsigned char c = (unsigned char)str[ix];
	  if (!((c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| (c >= '0' && c <= '9')
		|| c == '-' || c == '+' || c == '_'
		|| c == '/' || c == '%' || c == '.'))
	    {
	      quote = true;
	      break;
	    }
	}
    }

  if (quote)
    buffer.push_back ('\'');

  for (auto *end = str + len; str != end;)
    {
      auto *e = end;

      if (quote)
	// Look for next escape-needing char.  More restricted than
	// the earlier needs-quoting check.
	for (e = str; e != end; ++e)
	  {
	    unsigned char c = (unsigned char)*e;
	    if (c < ' ' || c >= 0x7f
		|| c == '\\' || c == '\'')
	      break;
	  }
      buffer.insert (buffer.end (), str, e);
      str = e;

      if (str == end)
	break;

      buffer.push_back ('\\');
      switch (unsigned char c = (unsigned char)*str++)
	{
	case '\t':
	  c = 't';
	  goto append;

	case '\n':
	  c = 'n';
	  goto append;

	case '\'':
	case '\\':
	append:
	  buffer.push_back (c);
	  break;

	default:
	  // Full-on escape.  Use 2 lower-case hex chars
	  for (unsigned shift = 8; shift;)
	    {
	      shift -= 4;

	      char nibble = (c >> shift) & 0xf;
	      nibble += '0';
	      if (nibble > '9')
		nibble += 'a' - ('9' + 1);
	      buffer.push_back (nibble);
	    }
	}
    }

  if (quote)
    buffer.push_back ('\'');
  
}

void MessageBuffer::Append (char c)
{
  if (!buffer.size () || buffer.back () == '\n')
    // Writing at beginning of line (why?)
    Append (&c, false, 1);
  else
    buffer.push_back (c);
}

void MessageBuffer::Eom ()
{
  if (!buffer.size () || buffer.back () != '\n')
    Append ("\n", false, 1);

  lastBol = buffer.front () == ' ' ? 1 : 0;
}

int MessageBuffer::Write (int fd)
{
  int err = 0;
  size_t limit = buffer.size () - lastBol;
  ssize_t count = write (fd, &buffer.data ()[lastBol], limit);
  
  if (count < 0)
    {
      err = errno;
      if (err == EAGAIN || err == EINTR)
	err = 0;
    }
  else
    {
      if (size_t (count) == limit)
	// We're done
	err = -1;
      lastBol += count;
    }

  if (err)
    {
      // We're done, reset for next block
      buffer.clear ();
      lastBol = 0;
    }

  return err;
}

int MessageBuffer::Read (int fd)
{
  constexpr size_t blockSize = 200;

  size_t lwm = buffer.size ();
  size_t hwm = buffer.capacity ();
  if (hwm - lwm < blockSize / 2)
    hwm += blockSize;
  buffer.resize (hwm);

  auto iter = buffer.begin () + lwm;
  ssize_t count = read (fd, &*iter, hwm - lwm);
  buffer.resize (lwm + (count >= 0 ? count : 0));

  int err = 0;
  
  if (count < 0)
    {
      err = errno;
      if (!(err == EAGAIN || err == EINTR))
	{
	malformed:
	  buffer.clear ();
	  goto done;
	}
    }

  {
    bool more = lastBol == buffer.size () || buffer[lastBol] == '+';
    for (;;)
      {
	auto newline = std::find (iter, buffer.end (), '\n');
	if (newline == buffer.end ())
	  break;
	iter = newline + 1;
	lastBol = iter - buffer.begin ();

	if (iter == buffer.end ())
	  break;

	if (!more)
	  {
	    // There is no continuation, but there are chars after the
	    // newline.
	    err = EINVAL;
	    goto malformed;
	  }
	more = *iter == '+';
      }

    if (more)
      return 0;
  }

  err = -1;

 done:
  lastBol = 0;
  return err;
}

int MessageBuffer::Tokenize (std::vector<std::string> &result)
{
  result.clear ();

 again:
  int err = -1;
  if (lastBol == buffer.size ())
    {
    out:
      buffer.clear ();
      lastBol = 0;
      return err;
    }

  Assert (buffer.back () == '\n');

  auto iter = buffer.begin () + lastBol;

  if (*iter == '+' || *iter == '-')
    // Strip continuation character
    ++iter;

  for (std::string *word = nullptr;;)
    {
      char c = *iter;

      ++iter;
      if (c == ' ')
	{
	  word = nullptr;
	  continue;
	}

      if (c == '\n')
	break;

      if (!word)
	{
	  result.emplace_back ();
	  word = &result.back ();
	}

      if (c != '\'')
	// Unquoted character
	word->push_back (c);
      else
	{
	  // Quoted word
	  for (;;)
	    {
	      c = *iter;

	      if (c == '\n')
		{
		malformed:;
		  result.clear ();
		  iter = std::find (iter, buffer.end (), '\n');
		  result.emplace_back (&buffer[lastBol],
				       iter - buffer.begin () + lastBol);
		  err = EINVAL;
		  goto out;
		}

	      ++iter;
	      if (c == '\'')
		break;

	      if (c == '\\')
		// escape
		switch (c = *iter)
		  {
		    case '\\':
		    case '\'':
		      ++iter;
		      break;

		    case 'n':
		      c = '\n';
		      ++iter;
		      break;

		    case 't':
		      c = '\t';
		      ++iter;
		      break;

		    default:
		      {
			unsigned v = 0;
			for (unsigned nibble = 0; nibble != 2; nibble++)
			  {
			    c = *iter;
			    if (c < '0')
			      {
				if (!nibble)
				  goto malformed;
				break;
			      }
			    else if (c <= '9')
			      c -= '0';
			    else if (c < 'a')
			      {
				if (!nibble)
				  goto malformed;
				break;
			      }
			    else if (c <= 'f')
			      c -= 'a' - 10;
			    else
			      {
				if (!nibble)
				  goto malformed;
				break;
			      }
			    ++iter;
			    v = (v << 4) | c;
			  }
			c = v;
		      }
		  }
	      word->push_back (c);
	    }
	}
    }
  lastBol = iter - buffer.begin ();
  if (result.empty ())
    goto again;

  return 0;
}

}
