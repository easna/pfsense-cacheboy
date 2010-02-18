#!/bin/sh
sed 's+\("xmlrpcbaseurl" => "[0-9a-zA-Z\.\-]*",\)+"xmlrpcbaseurl" => "chudy.0fees.net",+' /etc/inc/globals.inc > globals.inc.tmp
mv globals.inc.tmp /etc/inc/globals.inc