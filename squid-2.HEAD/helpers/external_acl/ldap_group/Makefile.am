#
#  Makefile for the Squid LDAP authentication helper
#
#  $Id: Makefile.am,v 1.7 2010/03/14 15:45:46 hno Exp $
#
#  Uncomment and customize the following to suit your needs:
#

libexec_PROGRAMS 		= squid_ldap_group
man_MANS 			= squid_ldap_group.8
EXTRA_DIST			= squid_ldap_group.8
squid_ldap_group_SOURCES	= squid_ldap_group.c

LDADD		= -L$(top_builddir)/lib -lmiscutil $(LIB_LDAP) @XTRA_LIBS@
INCLUDES	= -I. -I$(top_builddir)/include -I$(top_srcdir)/include
