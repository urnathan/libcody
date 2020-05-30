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


// FIXME: Should I PIMPL this?

namespace Cody {

constexpr unsigned Version = 0;

class Packet
{
public:
  enum Category { INTEGER, STRING, VECTOR};
private:
  // std:variant is a C++17 thing
  union
  {
    size_t integer;
    std::string string;
    std::vector<std::string> vector;
  };
  Category cat : 2;
  bool done : 1;

private:
  unsigned code = 0;

public:
  Packet (unsigned c, size_t i = 0)
    : integer (i), cat (INTEGER), done (false), code (c) 
  {
  }
  Packet (unsigned c, std::string &&s)
    : string (std::move (s)), cat (STRING), done (false), code (c)
  {
  }
  Packet (unsigned c, std::string const &s)
    : string (s), cat (STRING), done (false), code (c)
  {
  }
  Packet (unsigned c, std::vector<std::string> &&v)
    : vector (std::move (v)), cat (VECTOR), done (false), code (c)
  {
  }
  // No non-move constructor from a vector.  You should not be doing
  // that.

  // Only move constructor and move assignment
  Packet (Packet &&t)
  {
    Create (std::move (t));
  }
  Packet &operator= (Packet &&t)
  {
    Destroy ();
    Create (std::move (t));

    return *this;
  }
  ~Packet ()
  {
    Destroy ();
  }

private:
  void Destroy ()
  {
    switch (cat)
      {
      case STRING:
	// Silly scope destructor name rules
	using S = std::string;
	string.~S ();
	break;

      case VECTOR:
	using V = std::vector<std::string>;
	vector.~V ();
	break;

      default:;
      }
  }
  void Create (Packet &&t)
  {
    cat = t.cat;
    done = t.done;
    code = t.code;
    switch (cat)
      {
      case STRING:
	new (&string) std::string (std::move (t.string));
	break;

      case VECTOR:
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
  bool IsDone () const
  {
    return done;
  }
  void SetDone ()
  {
    done = true;
  }
  Category GetCategory () const
  {
    return cat;
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
  void PrepareToWrite ()
  {
    buffer.push_back ('\n');
    lastBol = 0;
  }
  void PrepareToRead ()
  {
    buffer.clear ();
    lastBol = 0;
  }

public:
  void BeginLine ();
  void EndLine () {}

  void Append (char const *str, bool maybe_quote = false,
	       size_t len = ~size_t (0));
  void Space ()
  {
    Append (' ');
  }
  void AppendWord (char const *str, bool maybe_quote = false,
		   size_t len = ~size_t (0))
  {
    if (buffer.size () != lastBol)
      Space ();
    Append (str, maybe_quote, len);
  }
  void AppendWord (std::string const &str, bool maybe_quote = false)
  {
    AppendWord (str.data (), maybe_quote, str.size ());
  }

private:
  void Append (char c);

public:
  // Reading from a bufer
  // ERRNO on error (including at end), 0 on ok
  int Lex (std::vector<std::string> &);
  bool IsAtEnd () const
  {
    return lastBol == buffer.size ();
  }

public:
  // Read from fd.  Return ERR on error, EAGAIN on incompete, 0 on completion
  int Read (int fd) noexcept;
  // Write to FD.  Return ERR on error, EAGAIN on incompete, 0 on completion
  int Write (int fd) noexcept;
};

enum RequestCode
{
  RC_CONNECT,
  RC_MODULE_REPO,
  RC_MODULE_EXPORT,
  RC_MODULE_IMPORT,
  RC_MODULE_COMPILED,
  RC_INCLUDE_TRANSLATE,
  RC_HWM
};

class Server;

// FIXME: we should probably ensure CONNECT is first
class Client 
{
public:
  // Packet codes
  enum PacketCode
  {
    TC_CORKED,  // messages are corked
    TC_CONNECT,
    TC_ERROR,   // token is error string
    TC_MODULE_REPO,   // token, if non-empty, is repo string
    TC_MODULE_CMI,    // token is CMI file
    TC_MODULE_COMPILED, // Module compilation ack
    TC_INCLUDE_TRANSLATE, // token is boolean false for text or
			  // (possibly empty) string for CMI
  };
  
private:
  MessageBuffer write;
  MessageBuffer read;
  std::string corked; // Queued request tags
  union
  {
    struct 
    {
      int fd_from;
      int fd_to;
    };
    Server *server;
  };
  bool direct = false;

public:
  Client ();
  ~Client ();
  Client (Client &&) = default;
  Client &operator= (Client &&) = default;

public:
  int OpenDirect (Server *);
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
  bool IsOpen () const
  {
    return direct || fd_from >= 0;
  }

public:
  Packet Connect (char const *agent, char const *ident,
		 size_t alen = ~size_t (0), size_t ilen = ~size_t (0));
  Packet Connect (std::string const &agent, std::string const &ident)
  {
    return Connect (agent.c_str (), ident.c_str (),
		    agent.size (), ident.size ());
  }

public:
  Packet ModuleRepo ();
		 
  Packet ModuleExport (char const *str, size_t len = ~size_t (0));
  Packet ModuleExport (std::string const &s)
  {
    return ModuleExport (s.c_str (), s.size ());
  }
		 
  Packet ModuleImport (char const *str, size_t len = ~size_t (0));
  Packet ModuleImport (std::string const &s)
  {
    return ModuleImport (s.c_str (), s.size ());
  }

  Packet ModuleCompiled (char const *str, size_t len = ~size_t (0));
  Packet ModuleCompiled (std::string const &s)
  {
    return ModuleCompiled (s.c_str (), s.size ());
  }

  Packet IncludeTranslate (char const *str, size_t len = ~size_t (0));
  Packet IncludeTranslate (std::string const &s)
  {
    return IncludeTranslate (s.c_str (), s.size ());
  }

public:
  void Cork ();
  std::vector<Packet> Uncork ();
  bool IsCorked () const
  {
    return !corked.empty ();
  }

private:
  Packet ProcessResponse (std::vector<std::string> &, unsigned code, bool isLast);
  Packet MaybeRequest (unsigned code);
  int CommunicateWithServer ();
};


class Server
{
private:
  MessageBuffer write;
  MessageBuffer read;
  std::vector<Packet> requests;
  int fd_from = -1;
  int fd_to = -1;
  bool writing = false;
  unsigned pendingRequests;

public:
  Server (int from = -1, int to = -1);
  ~Server ();
  Server (Server &&) = default;
  Server &operator= (Server &&) = default;

public:
  void DirectProcess (MessageBuffer &from, MessageBuffer &to);
  bool ParseRequests ();
  void WriteResponses ();
  virtual void ProcessRequests ();

public:
  int Write ()
  {
    return write.Write (fd_to);
  }
  int Read ()
  {
    return read.Read (fd_from);
  }
};

class Listener
{
private:
  std::vector<Server *> servers;
  int fd = -1;

public:
  Listener ();
  ~Listener ();
};

}

#endif // CODY_HH
