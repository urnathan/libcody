// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// Cody
#include "internal.hh"


namespace Cody {

ClientEnd::ClientEnd ()
{
  fd_from = fd_to = -1;
}

ClientEnd::~ClientEnd ()
{
  // FIXME: Disconnect?
}

int ClientEnd::OpenFDs (int from, int to)
{
  Assert (!direct && fd_from < 0 && fd_to < 0);
  fd_from = from;
  fd_to = to < 0 ? from : to;

  return 0;
}

void ClientEnd::Cork ()
{
  if (corked.empty ())
    corked.push_back (R_CORK);
}

int ClientEnd::DoTransaction ()
{
  int e = -1;
  if (direct)
    {
      std::swap (write, server->read);
      // FIXME: Invoke end
      std::swap (server->write, read);
    }
  else
    {
      // Write the write buffer
      while (!(e = write.Write (fd_to)))
	continue;
      if (e < 0)
	// Read the read buffer
	while (!(e = read.Read (fd_from)))
	  continue;
    }

  return e;
}

Token ClientEnd::Connect (char const *agent, char const *ident,
			  size_t alen, size_t ilen)
{
  write.AppendWord ("HELLO");
  char v[5];
  write.AppendWord (v, std::snprintf (v, sizeof (v), "%u", Version));
  write.AppendWord (agent, true, alen);
  write.AppendWord (ident, true, ilen);
  write.Eom ();

  int err = DoTransaction ();
  if (err > 0)
    {
      // Diagnose error
    }
  else
    {
      // HELLO VERSION AGENT <REPO>
      // ERROR 'text'
      // FIXME: 'REPO' does not belong here, it is module-specific.
      // Server should return some kind of flag or tuple set?
      std::vector<std::string> tokens;

      // FIXME: Probably create a helper fn, once I figure what it
      // should do
      err = read.Tokenize (tokens);
      if (err != 0)
	{
	  // Create error result
	}
      else
	{
	  auto &first = tokens[0];
	  if (first == "HELLO")
	    {
	      if (tokens.size () >= 4)
		return Token (R_HELLO, tokens[3]);
	      else
		return Token (R_HELLO, std::string (""));
	    }
	  else if (first == "ERROR")
	    {
	      return Token (R_ERROR, tokens[1]);
	    }
	  else
	    {
	      // Create error result
	    }
	}
      
    }

  return Token (R_ERROR, std::string ("Wat?"));
}

}
