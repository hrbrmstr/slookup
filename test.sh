#!/bin/bash

# A 

TEST_1=`echo rud.is | bin/slookup -t a`
RESP_1="rud.is + A 104.236.112.222"

if [ "$TEST_1" != "$RESP_1" ] ; then
  echo "Test 1: (A lookup) failed..."
  exit 1
else 
  echo "Test 1: (A lookup) passed"
fi

# PTR

TEST_2=`echo 104.236.112.222 | bin/slookup -t ptr`
RESP_2="104.236.112.222 + PTR rud.is"

if [ "$TEST_2" != "$RESP_2" ] ; then
  echo "Test 2: (PTR lookup) failed"
  exit 1
else 
  echo "Test 2: (PTR lookup) passed"
fi

# MX

TEST_3=`echo rud.is | bin/slookup -t mx`
RESP_3="rud.is + MX 10 aspmx.l.google.com MX 40 aspmx2.googlemail.com MX 20 alt1.aspmx.l.google.com MX 30 alt2.aspmx.l.google.com"

if [ "${TEST_3:7:5}" != "${RESP_3:7:5}" ] ; then
  echo "Test 3: (MX lookup) failed"
  exit 1
else 
  echo "Test 3: (MX lookup) passed"
fi

# NS

TEST_4=`echo rud.is | bin/slookup -t ns`
RESP_4="rud.is + NS dns.mwebdns.de NS dns.mwebdns.eu NS dns.mwebdns.net"

if [ "${TEST_4:7:5}" != "${RESP_4:7:5}" ] ; then
  echo "Test 4: (NS lookup) failed"
  exit 1
else 
  echo "Test 4: (NS lookup) passed"
fi

# TXT

TEST_5=`echo rud.is | bin/slookup -t txt`
RESP_5="rud.is + TXT google-site-verification=bkvck5fphxeqxf_6l0rdtdkk_utgd0bx7lqujg62zoo"

if [ "$TEST_5" != "$RESP_5" ] ; then
  echo "Test 5: (TXT lookup) failed"
  exit 1
else 
  echo "Test 5: (TXT lookup) passed"
fi

TEST_6=`echo www.github.com | bin/slookup -t cname`
RESP_6="www.github.com + CNAME github.com"

if [ "$TEST_6" != "$RESP_6" ] ; then
  echo "Test 6: (CNAME lookup) failed"
  exit 1
else 
  echo "Test 6: (CNAME lookup) passed"
fi


TEST_7=`echo rud.is | bin/slookup -t soa`
RESP_7="rud.is + SOA dns.mwebdns.de hostmaster.mandoraweb.de 2010012630 10800 3600 604800 86400"

if [ "$TEST_7" != "$RESP_7" ] ; then
  echo "Test 7: (SOA lookup) failed"
  exit 1
else 
  echo "Test 7: (SOA lookup) passed"
fi


tempfoo=`basename $0`
TMPFILE=`mktemp /tmp/${tempfoo}.XXXXXX` || exit 1

echo "rud.is" > $TMPFILE

TEST_8=`bin/slookup -t txt -i $TMPFILE`
RESP_8="rud.is + TXT google-site-verification=bkvck5fphxeqxf_6l0rdtdkk_utgd0bx7lqujg62zoo"

if [ "$TEST_8" != "$RESP_8" ] ; then
  echo "Test 8: (file input) failed"
  unlink $TMPFILE
  exit 1
else 
  echo "Test 8: (file input) passed"
  unlink $TMPFILE
fi

echo
echo "All tests succeeded"
exit 0
