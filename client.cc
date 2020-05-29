// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// Cody
#include "internal.hh"


namespace Cody {

// Queued requests
enum RequestCode : char
{
  RC_CORK,
  RC_CONNECT,
  RC_MODULE_EXPORT,
  RC_MODULE_IMPORT,
  RC_MODULE_DONE,
  RC_INCLUDE_TRANSLATE,
  RC_HWM
};

// These do not need to be members
static Token ConnectResponse (std::vector<std::string> &words);
// static Token ModuleExportResponse (std::vector<std::string> &words);
// static Token ModuleImportResponse (std::vector<std::string> &words);
// static Token ModuleDoneResponse (std::vector<std::string> &words);
// static Token IncludeTranslateResponse (std::vector<std::string> &words);

// Must be consistently ordered with the RequestCode enum
Token (*requestTable[RC_HWM]) (std::vector<std::string> &) = {
  nullptr,
  &ConnectResponse,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
};

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
    corked.push_back (RC_CORK);
}

int ClientEnd::DoTransaction ()
{
  if (direct)
    {
      std::swap (write, server->read);
      // FIXME: Invoke end
      std::swap (server->write, read);
    }
  else
    {
      // Write the write buffer
      while (int e = write.Write (fd_to))
	if (e != EAGAIN && e != EINTR)
	  return e;
      // Read the read buffer
      while (int e = read.Read (fd_from))
	if (e != EAGAIN && e != EINTR)
	  return e;
    }

  return 0;
}

Token ClientEnd::Connect (char const *agent, char const *ident,
			  size_t alen, size_t ilen)
{
  if (!IsCorked ())
    write.BeginMessage ();
  write.BeginLine ();
  write.AppendWord ("HELLO");
  char v[5];
  write.AppendWord (v, std::snprintf (v, sizeof (v), "%u", Version));
  write.AppendWord (agent, true, alen);
  write.AppendWord (ident, true, ilen);
  write.EndLine ();

  return MaybeRequest (RC_CONNECT);
}

Token ConnectResponse (std::vector<std::string> &words)
{
  // HELLO VERSION AGENT <REPO>
  // ERROR 'text'
  // FIXME: 'REPO' does not belong here, it is module-specific.
  // We should send multiple lines in this handshake
  // Server should return some kind of flag or tuple set?
  // FIXME: Probably want some more helper functions
  auto &first = words[0];
  if (first == "HELLO")
    {
      if (words.size () >= 4)
	return Token (ClientEnd::TC_CONNECT, words[3]);
      else
	return Token (ClientEnd::TC_CONNECT, std::string (""));
    }
  else if (first == "ERROR")
    {
      return Token (ClientEnd::TC_ERROR, words[1]);
    }
  else
    {
      // Create error result
    }

  return Token (ClientEnd::TC_ERROR, std::string ("Wat?"));
}

Token ClientEnd::MaybeRequest (unsigned code)
{
  if (IsCorked ())
    {
      corked.push_back (code);
      return Token (TC_CORKED);
    }

  write.EndMessage ();

  int err = DoTransaction ();
  if (err > 0)
    {
      // Diagnose error
    }
  else
    {
      std::vector<std::string> words;

      err = read.Lex (words);
      if (err != 0)
	{
	  // Create error result
	}
      else
	{
	  // FIXME: verify we're at the end of the message.  Reset to
	  // the beginning of a new message
	  return requestTable[code] (words);
	}
    }
  return Token (ClientEnd::TC_ERROR, std::string ("Wat"));
}

}

