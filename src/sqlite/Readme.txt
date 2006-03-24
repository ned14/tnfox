In here resides a modified copy of SQLite, a public-domain embedded
SQL database engine. You can find out more about SQLite at:

http://www.sqlite.org/

The enclosed copy is simply a copy of SQLite after it has been
configured and built on Linux, then all the unnecessary stuff removed.
This way, SQLite is standardised and doesn't require TCL or any of
the many other tools its build system requires to generate some of
the sources.

os_common.h has been modified to use TnFOX's memory allocator instead
of the standard one.
