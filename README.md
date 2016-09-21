# slookup - Parallelized fast DNS lookup tool

`slookup` is a simple program to do parallelized DNS lookups in a convenient way (useful for log parsing scripts and one-liners). It reads names (A/MX/NS lookups) or addresses (in dotted-quad format for PTR) on stdin and writes the results on stdout. One record per line. It can run up to 128 parallel DNS lookup processes (easily overloading a slow DNS server) which makes for Really Fast lookups for a large number of records. Beware, output is written in the order the DNS replies are received, which is usually different from the input order if parallel lookups are done.

--------

**NOTE** This is a fork of the original [`slookup`](http://he.fi/slookup/) utility. 

Project goals:

- [x] macOS support
- [ ] Enable JSON output
- [ ] Provide file-based input vs just `stdin`
- [ ] Specify a pool of nameservers to distribute lookups across
- [ ] Supoport all DNS record types
- [ ] Add a progress indicator
- [x] Tests (basic)
- [ ] ?? Distributed lookup (i.e. a cluster of `slookup` agents)
- [ ] ?? R package


### Installation

    $ make && make install

The `slookup` binary will be in `/usr/local/bin`.


### Usage

    slookup [-f <children>] [-p] [-t A|PTR|MX|NS]
    -f  Number of children to Fork for Fast parallel lookups
    -p  Use persistent TCP connection(s) to DNS server
    -t  Specify query type

i.e.

    $ echo rud.is | ./slookup -t a
    rud.is + A 104.236.112.222

    $ echo 104.236.112.222 | ./slookup -t ptr
    104.236.112.222 + PTR rud.is

    $ echo rud.is | ./slookup -t mx
    rud.is + MX 10 aspmx.l.google.com MX 40 aspmx2.googlemail.com MX 20 alt1.aspmx.l.google.com MX 30 alt2.aspmx.l.google.com

    $ echo "rud.is" | bin/slookup -t NS 
    rud.is + NS dns.mwebdns.de NS dns.mwebdns.eu NS dns.mwebdns.net


### Output

Until JSON output is supported, `slookup` responses come in the form of single line records in the form of:

    <question> <+|-> <response|error message

_Question_ is the string that was asked on `stdin`. _+_ means that the query succeeded, _-_ means that it failed. For failed requests, an error message stating the reason is printed. For successful requests, a list of responses is printed.


### Contributor Code of Conduct

As contributors and maintainers of this project, we pledge to respect all people who contribute through reporting issues, posting feature requests, updating documentation, submitting pull requests or patches, and other activities.

We are committed to making participation in this project a harassment-free experience for everyone, regardless of level of experience, gender, gender identity and expression, sexual orientation, disability, personal appearance, body size, race, ethnicity, age, or religion.

Examples of unacceptable behavior by participants include the use of sexual language or imagery, derogatory comments or personal attacks, trolling, public or private harassment, insults, or other unprofessional conduct.

Project maintainers have the right and responsibility to remove, edit, or reject comments, commits, code, wiki edits, issues, and other contributions that are not aligned to this Code of Conduct. Project maintainers who do not follow the Code of Conduct may be removed from the project team.

Instances of abusive, harassing, or otherwise unacceptable behavior may be reported by opening an issue or contacting one or more of the project maintainers.

This Code of Conduct is adapted from the Contributor Covenant (http:contributor-covenant.org), version 1.0.0, available at http://contributor-covenant.org/version/1/0/0/