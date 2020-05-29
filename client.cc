// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// Cody
#include "internal.hh"
// C
#include <cstring>

namespace Cody {

// Queued requests
enum RequestCode : char
{
  RC_CORK,
  RC_CONNECT,
  RC_MODULE_REPO,
  RC_MODULE_EXPORT,
  RC_MODULE_IMPORT,
  RC_MODULE_COMPILED,
  RC_INCLUDE_TRANSLATE,
  RC_HWM
};

// These do not need to be members
static Token ConnectResponse (std::vector<std::string> &words);
static Token ModuleRepoResponse (std::vector<std::string> &words);
static Token ModuleCMIResponse (std::vector<std::string> &words);
static Token ModuleCompiledResponse (std::vector<std::string> &words);
static Token IncludeTranslateResponse (std::vector<std::string> &words);

// Must be consistently ordered with the RequestCode enum
Token (*requestTable[RC_HWM]) (std::vector<std::string> &) = {
  nullptr,
  &ConnectResponse,
  &ModuleRepoResponse,
  &ModuleCMIResponse,
  &ModuleCMIResponse,
  &ModuleCompiledResponse,
  &IncludeTranslateResponse,
};

Client::Client ()
{
  fd_from = fd_to = -1;
}

Client::~Client ()
{
  // FIXME: Disconnect?
}

int Client::OpenFDs (int from, int to)
{
  Assert (!direct && fd_from < 0 && fd_to < 0);
  fd_from = from;
  fd_to = to < 0 ? from : to;

  return 0;
}

int Client::CommunicateWithServer ()
{
  write.PrepareToWrite ();
  read.PrepareToRead ();
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

static Token UnrecognizedResponse (std::vector<std::string> const &words)
{
  std::string msg {"unrecognized response"};

  for (auto iter = words.begin (); iter != words.end (); ++iter)
    {
      msg.append (" '");
      msg.append (*iter);
      msg.append ("'");
    }
  
  return Token (Client::TC_ERROR, std::move (msg));
}

static Token CommunicationError (int err)
{
  std::string e {"communication error:"};
  e.append (strerror (err));

  return Token (Client::TC_ERROR, std::move (e));
}

Token Client::ProcessResponse (std::vector<std::string> &words,
			       unsigned code, bool isLast)
{
  if (read.Lex (words))
    return UnrecognizedResponse (words);

  Assert (!words.empty ());
  if (words[0] == "ERROR")
    return Token (Client::TC_ERROR,
		  std::move (words.size () == 1 ? words[1]
			     : "malformed error response"));

  if (isLast && !read.IsAtEnd ())
    return Token (Client::TC_ERROR, std::string ("unexpected extra response"));

  Assert (code < RC_HWM);
  return requestTable[code] (words);
}

Token Client::MaybeRequest (unsigned code)
{
  if (IsCorked ())
    {
      corked.push_back (code);
      return Token (TC_CORKED);
    }

  if (int err = CommunicateWithServer ())
    return CommunicationError (err);

  std::vector<std::string> words;
  return ProcessResponse(words, code, true);
}

void Client::Cork ()
{
  if (corked.empty ())
    corked.push_back (RC_CORK);
}

std::vector<Token> Client::Uncork ()
{
  std::vector<Token> result;

  if (corked.size () > 1)
    {
      if (int err = CommunicateWithServer ())
	result.emplace_back (CommunicationError (err));
      else
	{
	  std::vector<std::string> words;
	  for (auto iter = corked.begin () + 1; iter != corked.end ();)
	    {
	      char code = *iter;
	      ++iter;
	      result.emplace_back (ProcessResponse (words, code,
						    iter == corked.end ()));
	    }
	}
    }

  corked.clear ();

  return result;
}

// Now the individual message handlers

// HELLO $vernum $agent $ident
Token Client::Connect (char const *agent, char const *ident,
			  size_t alen, size_t ilen)
{
  write.BeginLine ();
  write.AppendWord ("HELLO");
  char v[5];
  write.AppendWord (v, std::snprintf (v, sizeof (v), "%u", Version));
  write.AppendWord (agent, true, alen);
  write.AppendWord (ident, true, ilen);
  write.EndLine ();

  return MaybeRequest (RC_CONNECT);
}

// HELLO VERSION AGENT
Token ConnectResponse (std::vector<std::string> &words)
{
  if (words[0] == "HELLO" && words.size () == 3)
    {
      // I suppose at some point I may need to pay attention to the
      // version information
      std::vector<std::string> response;
      response.emplace_back (std::move (words[1]));
      response.emplace_back (std::move (words[2]));
      return Token (Client::TC_CONNECT, std::move (response));
    }

  return UnrecognizedResponse (words);
}

// MODULE-REPO
Token Client::ModuleRepo ()
{
  write.BeginLine ();
  write.AppendWord ("MODULE-REPO");
  write.EndLine ();

  return MaybeRequest (RC_MODULE_REPO);
}

// MODULE-REPO $dir
Token ModuleRepoResponse (std::vector<std::string> &words)
{
  if (words[0] == "MODULE-REPO" && words.size () == 2)
    {
      return Token (Client::TC_MODULE_REPO, std::move (words[1]));
    }

  return UnrecognizedResponse (words);
}

// MODULE-EXPORT $modulename
Token Client::ModuleExport (char const *module, size_t mlen)
{
  write.BeginLine ();
  write.AppendWord ("MODULE-EXPORT");
  write.AppendWord (module, true, mlen);
  write.EndLine ();

  return MaybeRequest (RC_MODULE_EXPORT);
}

// MODULE-IMPORT $modulename
Token Client::ModuleImport (char const *module, size_t mlen)
{
  write.BeginLine ();
  write.AppendWord ("MODULE-IMPORT");
  write.AppendWord (module, true, mlen);
  write.EndLine ();

  return MaybeRequest (RC_MODULE_IMPORT);
}

// MODULE-CMI $cmifile
Token ModuleCMIResponse (std::vector<std::string> &words)
{
  if (words[0] == "MODULE-CMI" && words.size () == 2)
    return Token (Client::TC_MODULE_CMI, std::move (words[1]));
  else
    return UnrecognizedResponse (words);
}

// MODULE-COMPILED $modulename
Token Client::ModuleCompiled (char const *module, size_t mlen)
{
  write.BeginLine ();
  write.AppendWord ("MODULE-COMPILED");
  write.AppendWord (module, true, mlen);
  write.EndLine ();

  return MaybeRequest (RC_MODULE_COMPILED);
}

// OK
Token ModuleCompiledResponse (std::vector<std::string> &words)
{
  if (words[0] == "OK")
    return Token (Client::TC_MODULE_COMPILED, 0);
  else
    return UnrecognizedResponse (words);
}

Token Client::IncludeTranslate (char const *include, size_t ilen)
{
  write.BeginLine ();
  write.AppendWord ("INCLUDE-TRANSLATE");
  write.AppendWord (include, true, ilen);
  write.EndLine ();

  return MaybeRequest (RC_INCLUDE_TRANSLATE);
}

// INCLUDE-TEXT
// INCLUDE-IMPORT $cmifile?
Token IncludeTranslateResponse (std::vector<std::string> &words)
{
  if (words[0] == "INCLUDE-TEXT" && words.size () == 1)
    return Token (Client::TC_INCLUDE_TRANSLATE, 0);
  else if (words[0] == "INCLUDE-IMPORT" && words.size () <= 2)
    return Token (Client::TC_INCLUDE_TRANSLATE,
		  std::move (words.size () > 1 ? words[1] : std::string ("")));
  else
    return UnrecognizedResponse (words);
}


}

