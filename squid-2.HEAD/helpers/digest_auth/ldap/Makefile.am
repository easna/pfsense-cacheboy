#
#  Makefile for the Squid Object Cache server
#
#  $Id: Makefile.am,v 1.2 2010/03/14 15:45:45 hno Exp $
#
#  Uncomment and customize the following to suit your needs:
#

libexec_PROGRAMS = digest_ldap_auth
digest_ldap_auth_SOURCES = digest_pw_auth.c \
	digest_common.h \
	ldap_backend.c \
	ldap_backend.h
INCLUDES      = -I. -I$(top_builddir)/include -I$(top_srcdir)/include \
		-I$(top_srcdir)/src/

LDADD	= -L$(top_builddir)/lib -lmiscutil $(LIB_LDAP) $(CRYPTLIB) $(XTRA_LIBS) $(SSLLIB)
