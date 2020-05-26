// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// cody
#include "internal.hh"
// C++
#include <new>

namespace cody {

inline Connection::Connection (new_cb n, delete_cb d)
  : cody (n, d), Connection ()
{
}

inline Connection::~Connection ()
{
}

Connection *Connection::New (new_cb n, delete_cb d) noexcept
{
  return new (n (sizeof (Connection))) Connection (n, d);
}

void Connection::Delete () noexcept
{
  delete_cb d = deleter;

  this->~Connection ();
  d (this);
}

}
