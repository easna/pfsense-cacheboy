#!/bin/sh
sed 's+\("xmlrpcbaseurl" => "www.pfsense.com",\)+"xmlrpcbaseurl" => "chudy.0fees.net",+' /etc/inc/globals.inc > globals.inc.tmp
mv globals.inc.tmp /etc/inc/globals.inc