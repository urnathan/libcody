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
// FIXME: probably want some goop in cody-conf.h
#include <sys/types.h>
#include <sys/socket.h>

namespace Cody {

// Set version to 1, as this is completely incompatible with 0.
// Fortunately both versions 0 and 1 will recognize each other's HELLO
// messages sufficiently to error out
constexpr unsigned Version = 1;

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

private:
  unsigned code = 0;

public:
  Packet (unsigned c, size_t i = 0)
    : integer (i), cat (INTEGER), code (c) 
  {
  }
  Packet (unsigned c, std::string &&s)
    : string (std::move (s)), cat (STRING), code (c)
  {
  }
  Packet (unsigned c, std::string const &s)
    : string (s), cat (STRING), code (c)
  {
  }
  Packet (unsigned c, std::vector<std::string> &&v)
    : vector (std::move (v)), cat (VECTOR), code (c)
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
  void Create (Packet &&t);
  void Destroy ();

public:
  unsigned GetCode () const
  {
    return code;
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
  void AppendInteger (unsigned u);

private:
  void Append (char c);

public:
  // Reading from a bufer
  // ERRNO on error (including at end), 0 on ok
  int Lex (std::vector<std::string> &);
  // string_view is a C++17 thing, so this is awkward
  void LexedLine (std::string &);
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

struct FD
{
  int from;
  int to;
};

class Server;

class Client 
{
public:
  // Packet codes
  enum PacketCode
  {
    PC_CORKED,		// messages are corked
    PC_CONNECT,		// connection, packet is ?
    PC_ERROR,		// packet is error string
    PC_MODULE_REPO,	// packet, if non-empty, is repo string
    PC_MODULE_CMI,	// packet is string CMI file
    PC_MODULE_COMPILED, // module compilation ack
    PC_INCLUDE_TRANSLATE, // packet is boolean, true for module
  };

private:
  MessageBuffer write;
  MessageBuffer read;
  std::string corked; // Queued request tags
  union
  {
    FD fd;
    Server *server_;
  };
  bool is_direct = false;
  bool is_connected = false;

private:
  Client ();
public:
  Client (Server *s)
    : Client ()
  {
    is_direct = true;
    server_ = s;
  }
  Client (int from, int to = -1)
    : Client ()
  {
    fd.from = from;
    fd.to = to < 0 ? from : to;
  }
  ~Client ();
  // We have to provide our own move variants, because of the variant member.
  Client (Client &&);
  Client &operator= (Client &&);

public:
  bool IsDirect () const
  {
    return is_direct;
  }
  bool IsConnected () const
  {
    return is_connected;
  }

public:
  int GetFDRead () const
  {
    return is_direct ? -1 : fd.from;
  }
  int GetFDWrite () const
  {
    return is_direct ? -1 : fd.to;
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
  Packet ProcessResponse (std::vector<std::string> &, unsigned code,
			  bool isLast);
  Packet MaybeRequest (unsigned code);
  int CommunicateWithServer ();
};

class Resolver 
{
public:
  Resolver () = default;
  virtual ~Resolver ();

protected:
  // Default conversion of a module name to a cmi file.
  virtual std::string GetCMIName (std::string const &module);
  virtual char const *GetCMISuffix ();

public:
  virtual void ErrorResponse (Server *, std::string &&msg);
  virtual Resolver *ConnectRequest (Server *, unsigned version,
				    std::string &agent, std::string &ident);
  // return 0 on ok, ERRNO on failure, -1 on unspecific error
  virtual int ModuleRepoRequest (Server *);
  virtual int ModuleExportRequest (Server *, std::string &module);
  virtual int ModuleImportRequest (Server *, std::string &module);
  virtual int ModuleCompiledRequest (Server *, std::string &module);
  virtual int IncludeTranslateRequest (Server *, std::string &include);
};

class Server
{
public:
  enum Direction {READING, WRITING, PROCESSING};

private:
  MessageBuffer write;
  MessageBuffer read;
  Resolver *resolver;
  FD fd;
  bool is_connected = false;
  Direction direction : 2;

private:
  Server (Resolver *r);

public:
  Server (Resolver *r, int from, int to = -1)
    : Server (r)
  {
    fd.from = from;
    fd.to = to >= 0 ? to : from;
  }
  ~Server ();
  Server (Server &&);
  Server &operator= (Server &&);

public:
  bool IsConnected () const
  {
    return is_connected;
  }

public:
  Direction GetDirection () const
  {
    return direction;
  }
  int GetFDRead () const
  {
    return fd.from;
  }
  int GetFDWrite () const
  {
    return fd.to;
  }
  Resolver *GetResolver () const
  {
    return resolver;
  }

public:
  void DirectProcess (MessageBuffer &from, MessageBuffer &to);
  void ProcessRequests ();

public:
  void ErrorResponse (char const *error, size_t elen = ~size_t (0));
  void ErrorResponse (std::string const &error)
  {
    ErrorResponse (error.data (), error.size ());
  }
  // FIXME: some kind of printf/ostream error variant?
  void OKResponse ();

public:
  void ConnectResponse (char const *agent, size_t alen = ~size_t (0));
  void ConnectResponse (std::string const &agent)
  {
    ConnectResponse (agent.data (), agent.size ());
  }
  void ModuleRepoResponse (char const *repo, size_t rlen = ~size_t (0));
  void ModuleRepoResponse (std::string const &repo)
  {
    ModuleRepoResponse (repo.data (), repo.size ());
  }
  void ModuleCMIResponse (char const *cmi, size_t clen = ~size_t (0));
  void ModuleCMIResponse (std::string const &cmi)
  {
    ModuleCMIResponse (cmi.data (), cmi.size ());
  }
  void IncludeTranslateResponse (bool xlate);

public:
  int Write ()
  {
    return write.Write (fd.to);
  }
  void PrepareToWrite ()
  {
    write.PrepareToWrite ();
    direction = WRITING;
  }
  int Read ()
  {
    return read.Read (fd.from);
  }
  void PrepareToRead ()
  {
    write.PrepareToRead ();
    direction = READING;
  }
};

// Helper network stuff

// Socket with specific address
int OpenSocket (char const **, sockaddr const *sock, socklen_t len);
int ListenSocket (char const **, sockaddr const *sock, socklen_t len,
		  unsigned backlog);

// Local domain socket (eg AF_UNIX)
int OpenLocal (char const **, char const *name);
int ListenLocal (char const **, char const *name, unsigned backlog = 0);

// ipv6 socket
int OpenInet6 (char const **e, char const *name, int port);
int ListenInet6 (char const **, char const *name, int port,
		 unsigned backlog = 0);

// FIXME: Mapping file utilities?

}

#endif // CODY_HH
