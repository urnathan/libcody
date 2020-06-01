// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// Cody
#include "internal.hh"

// Server code

namespace Cody {

// These do not need to be members
static int ConnectRequest (Server *, Resolver *,
			   std::vector<std::string> &words);
static int ModuleRepoRequest (Server *, Resolver *,
			      std::vector<std::string> &words);
static int ModuleExportRequest (Server *, Resolver *,
				std::vector<std::string> &words);
static int ModuleImportRequest (Server *, Resolver *,
				std::vector<std::string> &words);
static int ModuleCompiledRequest (Server *, Resolver *,
				  std::vector<std::string> &words);
static int IncludeTranslateRequest (Server *, Resolver *,
				    std::vector<std::string> &words);


static std::tuple<char const *,
		  int (*) (Server *, Resolver *, std::vector<std::string> &)>
  const requestTable[RC_HWM] =
  {
    // Same order as enum RequestCode
    {"HELLO", ConnectRequest},
    {"MODULE-REPO", ModuleRepoRequest},
    {"MODULE-EXPORT", ModuleExportRequest},
    {"MODULE-IMPORT", ModuleImportRequest},
    {"MODULE-COMPILED", ModuleCompiledRequest},
    {"INCLUDE-TRANSLATE", IncludeTranslateRequest},
  };

Server::Server ()
{
}

Server::~Server ()
{
  // FIXME: Disconnect?
}

void Server::DirectProcess (MessageBuffer &from, MessageBuffer &to)
{
  read.PrepareToRead ();
  std::swap (read, from);
  ParseRequests (direct);
  write.PrepareToWrite ();
  std::swap (to, write);
}

bool Server::ParseRequests (Resolver *resolver)
{
  std::vector<std::string> words;
  bool deferred = false;

  direction = PROCESSING;
  while (!read.IsAtEnd ())
    {
      bool err = 0;
      if (!read.Lex (words))
	{
	  Assert (!words.empty ());
	  for (unsigned ix = RC_HWM; ix--;)
	    if (words[0] == std::get<0> (requestTable[ix]))
	      {
		if (IsConnected () == (ix == RC_CONNECT))
		  {
		    err = -1;
		    break;
		  }
		int res = std::get<1> (requestTable[ix]) (this, resolver, words);
		if (res < 0)
		  {
		    err = +1;
		    break;
		  }

		if (res > 0)
		  deferred = true;
		goto found;
	      }
	}

      {
	std::string msg {err > 0 ? "malformed"
			 : !err ? "unrecognized"
			 : IsConnected () ? "connected"
			 : "unconnected"};

	msg.append (" request '");
	read.LexedLine (msg);
	msg.push_back ('\'');
	resolver->ErrorResponse (this, std::move (msg));
      }
    found:;
    }

  return deferred;
}

int ConnectRequest (Server *s, Resolver *r, std::vector<std::string> &words)
{
  if (words.size () < 3 || words.size () > 4)
    return -1;
  if (words.size () == 3)
    words.emplace_back ("");
  char *eptr;
  unsigned long version = strtoul (words[1].c_str (), &eptr, 10);
  if (*eptr)
    return -1;

  return r->ConnectRequest (s, unsigned (version), words[2], words[3]);
}

int ModuleRepoRequest (Server *s, Resolver *r,std::vector<std::string> &words)
{
  if (words.size () != 1)
    return -1;

  return r->ModuleRepoRequest (s);
}

int ModuleExportRequest (Server *s, Resolver *r, std::vector<std::string> &words)
{
  if (words.size () != 2 || words[1].empty ())
    return -1;

  return r->ModuleExportRequest (s, words[1]);
}

int ModuleImportRequest (Server *s, Resolver *r, std::vector<std::string> &words)
{
  if (words.size () != 2 || words[1].empty ())
    return -1;

  return r->ModuleExportRequest (s, words[1]);
}

int ModuleCompiledRequest (Server *s, Resolver *r,
			   std::vector<std::string> &words)
{
  if (words.size () != 2 || words[1].empty ())
    return -1;

  return r->ModuleCompiledRequest (s, words[1]);
}

int IncludeTranslateRequest (Server *s, Resolver *r,
			     std::vector<std::string> &words)
{
  if (words.size () != 2 || words[1].empty ())
    return -1;

  return r->IncludeTranslateRequest (s, words[1]);
}

void Server::ErrorResponse (char const *error, size_t elen)
{
  write.BeginLine ();
  write.AppendWord ("ERROR");
  write.AppendWord (error, true, elen);
  write.EndLine ();
}

void Server::OKResponse ()
{
  write.BeginLine ();
  write.AppendWord ("OK");
  write.EndLine ();
}

void Server::ConnectResponse (char const *agent, size_t alen)
{
  is_connected = true;

  write.BeginLine ();
  write.AppendWord ("HELLO");
  write.AppendInteger (Version);
  write.AppendWord (agent, true, alen);
  write.EndLine ();
}

void Server::ModuleRepoResponse (char const *repo, size_t rlen)
{
  write.BeginLine ();
  write.AppendWord ("MODULE-REPO");
  write.AppendWord (repo, true, rlen);
  write.EndLine ();
}

void Server::ModuleCMIResponse (char const *cmi, size_t clen)
{
  write.BeginLine ();
  write.AppendWord ("MODULE-CMI");
  write.AppendWord (cmi, true, clen);
  write.EndLine ();
}

void Server::IncludeTranslateResponse (bool translate)
{
  write.BeginLine ();
  write.AppendWord (translate ? "INCLUDE-IMPORT" : "INCLUDE-TEXT");
  write.EndLine ();
}

}
