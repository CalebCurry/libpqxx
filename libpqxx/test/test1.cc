#include <cassert>
#include <iostream>

#include "pg_connection.h"
#include "pg_transaction.h"
#include "pg_result.h"

using namespace PGSTD;
using namespace Pg;


// Define, locally but with C linkage, a function to process warnings
// generated by the database connection.  This is optional.
namespace
{
extern "C"
{
void ReportWarning(void *, const char msg[])
{
  cerr << msg;
}
}
}


// Simple test program for libpqxx.  Open connection to database, start
// a transaction, and perform a query inside it.
//
// Usage: test1 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int argc, char *argv[])
{
  try
  {
    // Set up a connection to the backend
    Connection C(argv[1] ? argv[1] : "");

    // If connection failed, C's constructor would have thrown an exception
    assert(C.IsOpen());

    // Tell C to report its warnings to stderr (which it'd have done anyway,
    // but let's show the folks how this can be changed if desired)
    C.SetNoticeProcessor(ReportWarning, 0);

    // Normally the database uses our notice processor to report warnings and
    // other messages, but so can we.  The trailing newline is mandatory.
    C.ProcessNotice("Connection created\n");

    // ProcessNotice() can take either a C++ string or a C-style char *.
    const string HostName = (C.HostName() ? C.HostName() : "<local>");
    C.ProcessNotice(string() +
		    "database=" + C.DbName() + ", "
		    "username=" + C.UserName() + ", "
		    "hostname=" + HostName + ", "
		    "port=" + ToString(C.Port()) + ", "
		    "options='" + C.Options() + "', "
		    "backendpid=" + ToString(C.BackendPID()) + "\n");

    // Begin a transaction acting on our current connection
    Transaction T(C, "test1");

    // Transaction has a ProcessNotice() as well, which calls Connection's
    // version and does the exact same thing
    T.ProcessNotice("Transaction started.\n");

    // Perform a query on the database, storing result tuples in R
    Result R( T.Exec("SELECT * FROM pg_tables") );

    // Give some feedback to the test program's user prior to the real work
    T.ProcessNotice(ToString(R.size()) + " "
		    "result tuples in transaction " +
		    T.Name() +
		    "\n");

    // Process each successive result tuple
    for (Result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
      // Read value of column 0 into a string N
      string N;
      c[0].to(N);

      // Dump tuple number and column 0 value to cout
      cout << '\t' << ToString(c.num()) << '\t' << N << endl;
    }

    // Tell the transaction that it has been successful
    T.Commit();
  }
  catch (const exception &e)
  {
    // All exceptions thrown by libpqxx are derived from std::exception
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    // This is really unexpected (see above)
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}

