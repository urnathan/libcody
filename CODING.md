# Coding standard

I guess I should document this, it might not be obvious

## GNU

The underlying formatting is GNU style.  What normally catches programmers unfamiliar with it is:

* Spaces between binary operators.  Particularly in a function call,
  between the name and the open paren:

  `Fn (a + b, ary[4], *ptr);`

* Scope braces are always on a line of their own, indented by 2
  spaces, if they're a sub-statement of an `if`, `for` or whatever:

  ```
  if (bob)
    {
      frob ();
      quux ();
    }
  ```

  Conditions and loops containing a single statement should not use `{}`.
  FWIW this was my personal indentation scheme, before I even met GNU code!

* The same is true for a function definition, except the indentation is zero.

* Break lines at 80 chars, this should be /before/ the operator, not after:

  ```
  a = (b
       + c);
  ```

  Use parens to control indentation.

* Template instantiations and c++ casts should have no space before the `<`:

  `std::vector<int> k;`

## Names

Unlike GNU code, use `PascalCase` for function, type and global
variable names.  Use `camelCase` for member variables.  Block-scope
vars can be `camelCase` or `snake_case`, your choice.