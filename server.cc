// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// Cody
#include "internal.hh"

namespace Cody {

Server::Server (int from, int to)
  : fd_from (from), fd_to (to)
{
}

Server::~Server ()
{
  // FIXME: Disconnect?
}

void Server::DirectProcess (MessageBuffer &from, MessageBuffer &to)
{
  std::swap (read, from);
  ParseRequests ();
  if (pendingRequests)
    ProcessRequests ();
  WriteResponses ();
  std::swap (to, write);
}

void Server::ProcessRequests ()
{
  for (auto iter = requests.begin (); iter != requests.end (); iter++)
    if (!iter->IsDone ())
      {
	*iter = Packet (RC_HWM, "unknown request");
	iter->SetDone ();
	pendingRequests--;
      }
}

bool Server::ParseRequests ()
{
  struct Request
  {
    char const *word;
    unsigned min_args;
    unsigned max_args;
  };
  Request const requestForms[]=
    {
      // Same order as enum RequestCode
      {"HELLO", 2, 3},
      {"MODULE-REPO", 0, 0},
      {"MODULE-EXPORT", 1, 1},
      {"MODULE-IMPORT", 1, 1},
      {"MODULE-COMPILED", 1, 1},
      {"INCLUDE-TRANSLATE", 1, 1},
      {nullptr, 0, 0}
    };

  std::vector<std::string> words;

  while (!read.IsAtEnd ())
    {
      if (!read.Lex (words))
	{
	  Assert (!words.empty ());
	  for (auto *ptr = requestForms; ptr->word; ++ptr)
	    if (words.size () > ptr->min_args
		&& words.size () <= ptr->max_args + 1
		&& words[0] == ptr->word)
	      {
		unsigned code = ptr - requestForms;
		switch (ptr->max_args)
		  {
		  default:
		    words.erase (words.begin ());
		    requests.emplace_back (Packet (code, std::move (words)));
		    break;

		  case 1:
		    requests.emplace_back (Packet (code, words[0]));
		    break;

		  case 0:
		    requests.emplace_back (Packet (code, 0));
		    break;
		  }
		pendingRequests++;
		goto found;
	      }
	}
      {
	std::string msg {"unrecognized request"};
	for (auto iter = words.begin (); iter != words.end (); ++iter)
	  {
	    msg.append (" '");
	    msg.append (*iter);
	    msg.append ("'");
	  }
	requests.emplace_back (Packet (RC_HWM, std::move (msg)));
	requests.back ().SetDone ();
      }
    found:;
    }

  return pendingRequests != 0;
}

void Server::WriteResponses ()
{
  for (auto iter = requests.begin (); iter != requests.end (); ++iter)
    {
      write.BeginLine ();
      switch (iter->GetCode ())
	{
	case RC_CONNECT:
	  {
	    write.AppendWord ("HELLO");
	    auto &vec = iter->GetVector ();
	    for (auto word = vec.begin (); word != vec.end (); ++word)
	      write.AppendWord (*word, true);
	  }
	  break;

	case RC_MODULE_REPO:
	case RC_MODULE_EXPORT:
	case RC_MODULE_IMPORT:
	  write.AppendWord ("MODULE-CMI");
	  write.AppendWord (iter->GetString (), true);
	  break;

	case RC_MODULE_COMPILED:
	  write.AppendWord ("OK");
	  break;

	case RC_INCLUDE_TRANSLATE:
	  if (iter->GetCode () == Packet::INTEGER && !iter->GetInteger ())
	    write.AppendWord ("INCLUDE-TEXT");
	  else
	    {
	      write.AppendWord ("INCLUDE-IMPORT");
	      if (iter->GetCode () == Packet::STRING)
		write.AppendWord (iter->GetString (), true);
	    }
	  break;

	case RC_HWM:
	  write.AppendWord ("ERROR");
	  write.AppendWord (iter->GetString (), true);
	  break;
	}
      write.EndLine ();
    }

  write.PrepareToWrite ();
}

}
