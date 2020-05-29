
// Test message round tripping
/*
  RUN: <<HELLO 0 TESTING REPO \
  RUN: <<MODULE-CMI biz/bar \
  RUN: <<MODULE-CMI blob \
  RUN: <<INCLUDE-TEXT \
  RUN: << INCLUDE-IMPORT foo \
  RUN: <<OK
*/
// RUN: $subdir$stem | ezio -p OUT $src |& ezio -p ERR $src
// RUN-END:

/*
  OUT-NEXT:^HELLO {:[0-9]+} TEST IDENT \$
  OUT-NEXT:^MODULE-EXPORT bar \
  OUT-NEXT:^MODULE-IMPORT foo \
  OUT-NEXT:^INCLUDE-TRANSLATE baz.frob \
  OUT-NEXT:^INCLUDE-TRANSLATE ./quux \
  OUT-NEXT:^MODULE-COMPILED bar
*/
// OUT-NEXT:$EOF

// ERR-NEXT:Code:1$
// ERR-NEXT:String:REPO$
// ERR-NEXT:Code:4$
// ERR-NEXT:String:biz/bar$
// ERR-NEXT:Code:4$
// ERR-NEXT:String:blob$
// ERR-NEXT:Code:6$
// ERR-NEXT:Integer:0$
// ERR-NEXT:Code:6$
// ERR-NEXT:String:foo
// ERR-NEXT:Code:5$
// ERR-NEXT:Integer:
// ERR-NEXT:$EOF


// Cody
#include "cody.hh"
// C++
#include <iostream>

using namespace Cody;

int main (int, char *[])
{
  Client client;

  client.OpenFDs (0, 1);

  client.Cork ();
  if (client.Connect ("TEST", "IDENT").GetCode () != Client::TC_CORKED)
    std::cerr << "Not corked!\n";
  if (client.ModuleExport ("bar").GetCode () != Client::TC_CORKED)
    std::cerr << "Not corked!\n";
  if (client.ModuleImport ("foo").GetCode () != Client::TC_CORKED)
    std::cerr << "Not corked!\n";
  if (client.IncludeTranslate ("baz.frob").GetCode () != Client::TC_CORKED)
    std::cerr << "Not corked!\n";
  if (client.IncludeTranslate ("./quux").GetCode () != Client::TC_CORKED)
    std::cerr << "Not corked!\n";
  if (client.ModuleCompiled ("bar").GetCode () != Client::TC_CORKED)
    std::cerr << "Not corked!\n";

  auto result = client.Uncork ();
  for (auto iter = result.begin (); iter != result.end (); ++iter)
    {
      std::cerr << "Code:" << iter->GetCode () << '\n';
      switch (iter->GetCategory ())
	{
	case Token::INTEGER:
	  std::cerr << "Integer:" << iter->GetInteger () << '\n';
	  break;
	case Token::STRING:
	  std::cerr << "String:" << iter->GetString () << '\n';
	  break;
	case Token::VECTOR:
	  std::cerr << "Vector:?" << '\n';
	  break;
	}
    }
}
