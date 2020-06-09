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

namespace Detail  {

/// Internal buffering class.  Used to concatenate outgoing messages
/// and Lex incoming ones.
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
  ///
  /// Finalize a buffer to be written.  No more lines can be added to
  /// the buffer
  void PrepareToWrite ()
  {
    buffer.push_back ('\n');
    lastBol = 0;
  }
  ///
  /// Prepare a buffer for reading.
  void PrepareToRead ()
  {
    buffer.clear ();
    lastBol = 0;
  }

public:
  /// Begin a message line
  void BeginLine ();
  /// End a message line
  void EndLine () {}

public:
  /// Append a string to the current line.  No whitespace is prepended
  /// or appended.
  ///
  /// @param str the string to be written
  /// @param maybe_quote indicate if there's a possibility the string
  /// contains characters that need quoting.  Defaults to false.
  /// It is always safe to set
  /// this true, but that causes an additional scan of the string.
  /// @param len The length of the string.  If not specified, strlen
  /// is used to find the length.
  void Append (char const *str, bool maybe_quote = false,
	       size_t len = ~size_t (0));

  /// Add whitespace word separator
  void Space ()
  {
    Append (' ');
  }
  /// Add a word as with Append, but prefixing whitespace to make a
  /// separate word
  void AppendWord (char const *str, bool maybe_quote = false,
		   size_t len = ~size_t (0))
  {
    if (buffer.size () != lastBol)
      Space ();
    Append (str, maybe_quote, len);
  }
  /// Add a word as with AppendWord
  /// @param str the string to append
  void AppendWord (std::string const &str, bool maybe_quote = false)
  {
    AppendWord (str.data (), maybe_quote, str.size ());
  }
  ///
  /// Add an integral value, prepending a space.
  void AppendInteger (unsigned u);

private:
  void Append (char c);

public:
  // Reading from a bufer
  // ERRNO on error (including at end), 0 on ok

  /// Lex the next input line.
  /// @param words filled with a vector of lexed strings
  /// @result 0 if no errors, an errno value on lexxing error such as
  /// there being no next line (ENOMSG), or malformed quoting (EINVAL)
  int Lex (std::vector<std::string> &words);

  // string_view is a C++17 thing, so this is awkward
  /// Provide the most-recently lexxed line.
  void LexedLine (std::string &);
  ///
  /// Detect if we have reached the end of the input buffer.
  /// I.e. there are no more lines to Lex
  /// @return True if at end
  bool IsAtEnd () const
  {
    return lastBol == buffer.size ();
  }

public:
  // Read from fd.  Return ERR on error, EAGAIN on incompete, 0 on
  // completion

  /// Read from end point into a read buffer, as with read(2).  This will
  /// not block , unless FD is blocking, and there is nothing
  /// immediately available.
  /// @param fd file descriptor to read from.  This may be a regular
  /// file, pipe or socket.
  /// @result on error returns errno.  If end of file occurs, returns
  /// -1.  At end of message returns 0.  If there is more needed
  /// returns EAGAIN (or possibly EINTR).  If the message is
  /// malformed, returns EINVAL.
  int Read (int fd) noexcept;

public:
  /// Write to an end point from a write buffer, as with write(2).  As
  /// with Read, this will not usually block.
  /// @param fd file descriptor to write to.  This may be a regular
  /// file, pipe or socket.
  /// @result on error returns errno.
  /// At end of message returns 0.  If there is more to write
  /// returns EAGAIN (or possibly EINTR).
  int Write (int fd) noexcept;
};

///
/// Internal Request codes
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

}

///
/// Response data for a request.  Returned by Client's request calls. 
///
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
  ///
  /// Return the packet type
  unsigned GetCode () const
  {
    return code;
  }
  ///
  /// Return the category of the packet's payload
  Category GetCategory () const
  {
    return cat;
  }

public:
  ///
  /// Return an integral payload.  Undefined if the category is not INTEGER
  size_t GetInteger () const
  {
    return integer;
  }
  ///
  /// Return (a reference to) a string payload.  Undefined if the
  /// category is not STRING
  std::string const &GetString () const
  {
    return string;
  }
  std::string &GetString ()
  {
    return string;
  }
  ///
  /// Return (a reference to) a vector of strings payload.  Undefined
  /// if the category is not VECTOR
  std::vector<std::string> const &GetVector () const
  {
    return vector;
  }
  std::vector<std::string> &GetVector ()
  {
    return vector;
  }
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
  Detail::MessageBuffer write;
  Detail::MessageBuffer read;
  std::string corked; // Queued request tags
  union
  {
    Detail::FD fd;
    Server *server;
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
    server = s;
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
  Server *GetServer () const
  {
    return is_direct ? server : nullptr;
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
  Detail::MessageBuffer write;
  Detail::MessageBuffer read;
  Resolver *resolver;
  Detail::FD fd;
  bool is_connected = false;
  Direction direction : 2;

public:
  Server (Resolver *r);
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
  void DirectProcess (Detail::MessageBuffer &from, Detail::MessageBuffer &to);
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
