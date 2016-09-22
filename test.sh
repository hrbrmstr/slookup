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

echo
echo "All tests succeeded"
exit 0
