// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: Apache v2.0

// Cody
#include "internal.hh"
// C++
#include <tuple>
// C
#include <cstring>
#include <cerrno>

// Server code

namespace Cody {

// These do not need to be members
static Resolver *ConnectRequest (Server *, Resolver *,
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
  const requestTable[Detail::RC_HWM] =
  {
    // Same order as enum RequestCode
    {"HELLO", nullptr},
    {"MODULE-REPO", ModuleRepoRequest},
    {"MODULE-EXPORT", ModuleExportRequest},
    {"MODULE-IMPORT", ModuleImportRequest},
    {"MODULE-COMPILED", ModuleCompiledRequest},
    {"INCLUDE-TRANSLATE", IncludeTranslateRequest},
  };

Server::Server (Resolver *r)
  : resolver (r)
{
}

Server::Server (Server &&src)
  : write (std::move (src.write)),
    read (std::move (src.read)),
    resolver (src.resolver),
    is_connected (src.is_connected),
    direction (src.direction)
{
  fd.from = src.fd.from;
  fd.to = src.fd.to;
}

Server::~Server ()
{
}

Server &Server::operator= (Server &&src)
{
  write = std::move (src.write);
  read = std::move (src.read);
  resolver = src.resolver;
  is_connected = src.is_connected;
  direction = src.direction;
  fd.from = src.fd.from;
  fd.to = src.fd.to;

  return *this;
}

void Server::DirectProcess (Detail::MessageBuffer &from,
			    Detail::MessageBuffer &to)
{
  read.PrepareToRead ();
  std::swap (read, from);
  ProcessRequests ();
  resolver->WaitUntilReady (this);
  write.PrepareToWrite ();
  std::swap (to, write);
}

void Server::ProcessRequests (void)
{
  std::vector<std::string> words;

  direction = PROCESSING;
  while (!read.IsAtEnd ())
    {
      int err = 0;
      unsigned ix = Detail::RC_HWM;
      if (!read.Lex (words))
	{
	  Assert (!words.empty ());
	  while (ix--)
	    {
	      if (words[0] != std::get<0> (requestTable[ix]))
		continue; // not this one

	      if (ix == Detail::RC_CONNECT)
		{
		  // CONNECT
		  if (IsConnected ())
		    err = -1;
		  else if (auto *r = ConnectRequest (this, resolver, words))
		    resolver = r;
		  else
		    err = -1;
		}
	      else
		{
		  if (!IsConnected ())
		    err = -1;
		  else if (int res = (std::get<1> (requestTable[ix])
				      (this, resolver, words)))
		    err = res;
		}
	      break;
	    }
	}

      if (err || ix >= Detail::RC_HWM)
	{
	  // Some kind of error
	  std::string msg;

	  if (err > 0)
	    msg = "error processing '";
	  else if (ix >= Detail::RC_HWM)
	    msg = "unrecognized '";
	  else if (IsConnected () && ix == Detail::RC_CONNECT)
	    msg = "already connected '";
	  else if (!IsConnected () && ix != Detail::RC_CONNECT)
	    msg = "not connected '";
	  else
	    msg = "malformed '";

	  read.LexedLine (msg);
	  msg.push_back ('\'');
	  if (err > 0)
	    {
	      msg.push_back (' ');
	      msg.append (strerror (err));
	    }
	  resolver->ErrorResponse (this, std::move (msg));
	}
    }
}

Resolver *ConnectRequest (Server *s, Resolver *r,
			  std::vector<std::string> &words)
{
  if (words.size () < 3 || words.size () > 4)
    return nullptr;

  if (words.size () == 3)
    words.emplace_back ("");
  char *eptr;
  unsigned long version = strtoul (words[1].c_str (), &eptr, 10);
  if (*eptr)
    return nullptr;

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
    return EINVAL;

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
