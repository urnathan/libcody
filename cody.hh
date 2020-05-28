// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

#ifndef CODY_HH
#define CODY_HH 1

#include "cody-conf.h"
// C++
#include <vector>
#include <string>
#include <memory>
// C
#include <cstddef>
// OS
#include <sys/types.h>
#include <sys/socket.h>

namespace Cody {

constexpr unsigned Version = 0;

class Token
{
private:
  // std:variant is a C++17 thing
  union
  {
    size_t integer;
    std::string string;
    std::vector<std::string> vector;
  };
  enum Category { Int, String, Vector};
  Category cat : 2;

private:
  unsigned code = 0;

public:
  Token (unsigned c, size_t i = 0)
    : integer (i), cat (Int), code (c)
  {
  }
  Token (unsigned c, std::string &&s)
    : string (std::move (s)), cat (String), code (c)
  {
  }
  Token (unsigned c, std::string const &s)
    : string (s), cat (String), code (c)
  {
  }
  Token (unsigned c, std::vector<std::string> &&v)
    : vector (std::move (v)), cat (Vector), code (c)
  {
  }
  // No non-move constructor from a vector.  You should not be doing
  // that.

  // Only move constructor and move assignment
  Token (Token &&t)
  {
    Create (std::move (t));
  }
  Token &operator= (Token &&t)
  {
    Destroy ();
    Create (std::move (t));

    return *this;
  }
  ~Token ()
  {
    Destroy ();
  }

private:
  void Destroy ()
  {
    switch (cat)
      {
      case String:
	// Silly scope destructor name rules
	using S = std::string;
	string.~S ();
	break;

      case Vector:
	using V = std::vector<std::string>;
	vector.~V ();
	break;

      default:;
      }
  }
  void Create (Token &&t)
  {
    cat = t.cat;
    code = t.code;
    switch (cat)
      {
      case String:
	new (&string) std::string (std::move (t.string));
	break;

      case Vector:
	new (&vector) std::vector<std::string> (std::move (t.vector));
	break;

      default:
	integer = t.integer;
	break;
      }
  }

public:
  unsigned GetCode () const
  {
    return code;
  }

  size_t GetInteger () const
  {
    return integer;
  }
  std::string const &GetString () const
  {
    return string;
  }
  std::string &GetString ()
  {
    return string;
  }
  std::vector<std::string> const &GetVector () const
  {
    return vector;
  }
  std::vector<std::string> &GetVector ()
  {
    return vector;
  }
};

class MessageBuffer
{
  std::vector<char> buffer;
  size_t lastBol = 0;

public:
  MessageBuffer () = default;
  ~MessageBuffer () = default;
  MessageBuffer (MessageBuffer &&) = default;
  MessageBuffer &operator= (MessageBuffer &&) = default;

public:
  void Append (char const *str, bool maybe_quote = false,
	       size_t len = ~size_t (0));
  void Space ()
  {
    Append (' ');
  }
  void Eol ()
  {
    Append ('\n');
  }
  void Eom ();
  void AppendWord (char const *str, bool maybe_quote = false,
		   size_t len = ~size_t (0))
  {
    if (buffer.size () != lastBol)
      Space ();
    Append (str, maybe_quote, len);
  }

private:
  void Append (char c);

  // FIXME: 0 on complete, -1 on try again or maybe EAGAIN?
public:
  // -1 on end of messages, ERRNO on error, 0 on ok
  int Tokenize (std::vector<std::string> &);

public:
  // Read.  Return ERR on error, -1 on completion, 0 otherwise
  int Read (int fd) noexcept;
  // Write to FD.  Return ERR on error, -1 on completion, 0 otherwise
  int Write (int fd) noexcept;
};

class ServerEnd
{
private:
  MessageBuffer write;
  MessageBuffer read;
  int fd = -1;
  int fd_to = -1;

public:
  ServerEnd ();
  ~ServerEnd ();
  ServerEnd (ServerEnd &&) = default;
  ServerEnd &operator=  (ServerEnd &&) = default;

public:

  friend class ClientEnd;
};

class ClientEnd 
{
public:
  enum Responses 
  {
    R_CORKED,  // messages are corked
    R_HELLO,
    R_ERROR,   // token is error string
    R_MODULE_REPO,   // token, if non-empty, is repo string
    R_MODULE_CMI,    // token is CMI file
    R_MODULE_TRANSLATE, // token is boolean, true for translation
  };
  
private:
  enum Requests : char
  {
    R_CORK,
    R_MODULE_EXPORT,
    R_MODULE_IMPORT,
    R_MODULE_DONE,
    R_INCLUDE_TRANSLATE,
  };

private:
  MessageBuffer write;
  MessageBuffer read;
  std::string corked;
  union
  {
    struct 
    {
      int fd_from;
      int fd_to;
    };
    ServerEnd *server;
  };
  bool direct = false;

public:
  ClientEnd ();
  ~ClientEnd ();
  ClientEnd (ClientEnd &&) = default;
  ClientEnd &operator=  (ClientEnd &&) = default;

public:
  int OpenDirect (ServerEnd *);
  int OpenFDs (int from, int to = -1);
  int OpenLocal (std::string &e, char const *name, size_t len = ~size_t (0));
  int OpenLocal (std::string &e, std::string const &s)
  {
    return OpenLocal (e, s.c_str (), s.size ());
  }
  int OpenSocket (std::string &e, char const *name,
		  int port, size_t len = ~size_t (0));
  int OpenLocal (std::string &e, std::string const &s, int port)
  {
    return OpenSocket (e, s.c_str (), port, s.size ());
  }

public:
  // You will get a token back
  Token Connect (char const *agent, char const *ident,
		 size_t alen = ~size_t (0), size_t ilen = ~size_t (0));
  Token Connect (std::string const &agent, std::string const &ident)
  {
    return Connect (agent.c_str (), ident.c_str (),
		    agent.size (), ident.size ());
  }

public:
  Token ModuleExport (char const *str, size_t len = ~size_t (0));
  Token ModuleExport (std::string const &s)
  {
    return ModuleExport (s.c_str (), s.size ());
  }
		 
  Token ModuleImport (char const *str, size_t len = ~size_t (0));
  Token ModuleImport (std::string const &s)
  {
    return ModuleImport (s.c_str (), s.size ());
  }

  Token ModuleDone (char const *str, size_t len = ~size_t (0));
  Token ModuleDone (std::string const &s)
  {
    return ModuleDone (s.c_str (), s.size ());
  }

  Token IncludeTranslate (char const *str, size_t len = ~size_t (0));
  Token IncludeTranslate (std::string const &s)
  {
    return IncludeTranslate (s.c_str (), s.size ());
  }

public:
  void Cork ();
  std::vector<Token> Uncork ();

private:
  int DoTransaction ();
  Token *ProcessLine ();
};

class Server
{
private:
  std::vector<ServerEnd> clients;
  int fd = -1;

public:
  Server ();
  ~Server ();
};

}

#endif // CODY_HH
