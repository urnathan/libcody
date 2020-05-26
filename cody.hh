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

class Action
{
public:
  enum Code
  {
    MODULE_EXPORT,
    MODULE_IMPORT,
    MODULE_DONE,
    INCLUDE_TRANSLATE,
  };

private:
  Code code;

public:
  Action (Code c)
    : code (c)
  {
  }
  virtual ~Action ();
};

class ActionWord : public Action
{
public:
  using Parent = Action;

protected:
  std::string value;

public:
  ActionWord (Code c, std::string &&v)
    : Parent (c), value (v)
  {
  }
  virtual ~ActionWord ();
};

class Response
{
public:
  enum Code
  {
    ERROR,
    MODULE_CMI,
    UNSIGNED,
  };

protected:
  Code code;

public:
  Response (Code c)
    : code (c)
  {
  }
  virtual ~Response ();
};

class ResponseWord : public Response
{
public:
  using Parent = Response;

protected:
  std::string value;

public:
  ResponseWord (Code c, std::string &&v)
    : Parent (c), value (v)
  {
  }
  virtual ~ResponseWord ();
};

class ResponseUnsigned : public Response
{
public:
  using Parent = Response;

protected:
  unsigned value;

public:
  ResponseUnsigned (Code c, unsigned v)
    : Parent (c), value (v)
  {
  }
  virtual ~ResponseUnsigned ();
};

class MessageBuffer
{
  std::vector<char> buffer;
  size_t lastBol = 0;

public:
  MessageBuffer () = default;
  ~MessageBuffer () = default;
  MessageBuffer (MessageBuffer &&) = default;
  MessageBuffer &operator=  (MessageBuffer &&) = default;

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

private:
  void Append (char c);

public:
  // -1 on end of messages, ERRNO on error, 0 on ok
  int Tokenize (std::vector<std::string> &);

public:
  // Read.  Return ERR on error, -1 on completion, 0 otherwise
  int Read (int fd) noexcept;
  // Write to FD.  Return ERR on error, -1 on completion, 0 otherwise
  int Write (int fd) noexcept;
};

class Server
{
public:
  Server ();
  ~Server ();
};

class Client 
{
private:
  MessageBuffer write;
  MessageBuffer read;

public:
  // FIXME: Allow direct connection to server
  Client ();

public:
  std::unique_ptr<Response> ModuleExport (char const *str, size_t len);
  std::unique_ptr<Response> ModuleImport (char const *str, size_t len);
  std::unique_ptr<Response> ModuleDone (char const *str, size_t len);
  std::unique_ptr<Response> IncludeTranslate (char const *str, size_t len);

public:
  void Cork ();
  std::vector<std::unique_ptr<Response>> Uncork ();
};

}

#endif // CODY_HH
