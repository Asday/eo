#!/bin/sh

echo -n "message 1" | nc -cu localhost 6562 && echo "success"
echo -n "message 2" | nc -cu localhost 6562 && echo "success"
sleep 1
echo -n "message 3" | nc -cu localhost 6562 && echo "success"
echo -n "should fail" | nc -cu localhost 6562 || \
  echo "success (expected connection refused)"
