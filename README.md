# slookup - Parallelized fast DNS lookup tool

`slookup` is a simple program to do parallelized DNS lookups in a convenient way (useful for log parsing scripts and one-liners). It reads names (A/MX/NS lookups) or addresses (in dotted-quad format for PTR) on stdin and writes the results on stdout. One record per line. It can run up to 128 parallel DNS lookup processes (easily overloading a slow DNS server) which makes for Really Fast lookups for a large number of records. Beware, output is written in the order the DNS replies are received, which is usually different from the input order if parallel lookups are done.

--------

NOTE This is a fork of the original [`slookup`](http://he.fi/slookup/) utility. 

Project goals:

- [x] macOS support
- [ ] Enable JSON output
- [ ] Providie file-based input vs just `stdin`
- [ ] Specify a pool of nameservers to distribute lookups across
- [ ] Supoport all DNS record types
- [ ] Add a progress indicator
- [ ] Tests
- [ ] ?? Distributed lookup (i.e. a cluster of `slookup` agents)
- [ ] ?? R package

### Installation

    $ make && make install

The `slookup` binary will be in `/usr/local/bin`.

### Usage

    slookup [-v] [-f <children>] [-p] [-t A|PTR|MX|NS]
    -f  Number of children to Fork for Fast parallel lookups
    -p  Use persistent TCP connection(s) to DNS server
    -t  Specify query type

### Output

Until JSON output is supported, `slookup` responses come in the form of single line records in the form of:

    <question> <+|-> <response|error message

_Question_ is the string that was asked on `stdin`. _+_ means that the query succeeded, _-_ means that it failed. For failed requests, an error message stating the reason is printed. For successful requests, a list of responses is printed.

