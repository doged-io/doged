# Bitcoin ABC 0.30.3 Release Notes

Bitcoin ABC version 0.30.3 is now available from:

  <https://download.bitcoinabc.org/0.30.3/>

This release includes the following features and fixes:
 - Several minor Chronik bugs have been fixed that could return wrong data for
   some SLP type 2 mint transactions, or store extraneous content to the
   database. It is advised to upgrade if you are affected by these issues on
   your current chronik instance. No reindex is required but the first start
   will upgrade the database automatically which will take about 15 minutes.

Iguana Debugger
---------------

This release adds the Iguana debugger, a simple tool that runs the scripts of
transactions and outputs a trace of each opcode with the current stack at each
step. It also has a CSV option to view the trace in a spreadsheet software.

You can run it like this:
`iguana -tx=<txhex> -value=<value> -scriptpubkey=<scriptpubkey> -inputindex=0`

Or run `iguana -h` to see all the options.

Deprecating ARM (32 bits)
-------------------------

Starting with this release, the Bitcoin ABC ARM (32 bits) version is deprecated.
It will no longer be delivered with the release binaries. Users that still want
to run Bitcoin ABC on this platform should build the binary from the sources.
