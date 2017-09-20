#!/usr/bin/env bash 

cd generated/

openssl req -newkey rsa:1024 -sha1 -keyout clientAkey.pem -out clientAreq.pem \
  -nodes -config ../clientA.cnf -days 7300 -batch

openssl x509 -req -in clientAreq.pem -sha1 -extfile ../clientA.cnf \
  -extensions usr_cert -CA rootA.pem -CAkey rootAkey.pem -CAcreateserial \
  -out clientAcert.pem -days 7300

cat clientAcert.pem rootA.pem > clientA.pem

openssl x509 -subject -issuer -noout -in clientA.pem
