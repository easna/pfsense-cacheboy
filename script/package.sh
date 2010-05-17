#!/bin/sh
privaterepository=$1
if [ "$privaterepository" = "off" ];then {
sed 's+\("xmlrpcbaseurl" => "[0-9a-zA-Z\.\-]*",\)+"xmlrpcbaseurl" => "www.pfsense.com",+' /etc/inc/globals.inc > globals.inc.tmp
mv globals.inc.tmp /etc/inc/globals.inc
}
else {
sed 's+\("xmlrpcbaseurl" => "[0-9a-zA-Z\.\-]*",\)+"xmlrpcbaseurl" => "chudy.0fees.net",+' /etc/inc/globals.inc > globals.inc.tmp
mv globals.inc.tmp /etc/inc/globals.inc
}
fi
