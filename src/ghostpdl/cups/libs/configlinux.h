/* config.h.  Generated from config.h.in by configure.  */
/*
 * "$Id: config.h.in 11642 2014-02-27 15:57:59Z msweet $"
 *
 *   Configuration file for CUPS.
 *
 *   Copyright 2007-2013 by Apple Inc.
 *   Copyright 1997-2007 by Easy Software Products.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Apple Inc. and are protected by Federal copyright
 *   law.  Distribution and use rights are outlined in the file "LICENSE.txt"
 *   which should have been included with this file.  If this file is
 *   file is missing or damaged, see the license at "http://www.cups.org/".
 */

#ifndef _CUPS_CONFIG_H_
#define _CUPS_CONFIG_H_

/*
 * Version of software...
 */

#define CUPS_SVERSION "CUPS v1.7.3"
#define CUPS_MINIMAL "CUPS/1.7.3"


/*
 * Default user and groups...
 */

#define CUPS_DEFAULT_USER "lp"
#define CUPS_DEFAULT_GROUP "lp"
#define CUPS_DEFAULT_SYSTEM_GROUPS "lpadmin sys root"
#define CUPS_DEFAULT_PRINTOPERATOR_AUTH "@SYSTEM"


/*
 * Default file permissions...
 */

#define CUPS_DEFAULT_CONFIG_FILE_PERM 0640
#define CUPS_DEFAULT_LOG_FILE_PERM 0644


/*
 * Default logging settings...
 */

#define CUPS_DEFAULT_LOG_LEVEL "warn"
#define CUPS_DEFAULT_ACCESS_LOG_LEVEL "actions"


/*
 * Default fatal error settings...
 */

#define CUPS_DEFAULT_FATAL_ERRORS "config"


/*
 * Default browsing settings...
 */

#define CUPS_DEFAULT_BROWSING 1
#define CUPS_DEFAULT_BROWSE_LOCAL_PROTOCOLS ""
#define CUPS_DEFAULT_DEFAULT_SHARED 1


/*
 * Default IPP port...
 */

#define CUPS_DEFAULT_IPP_PORT 631


/*
 * Default printcap file...
 */

#define CUPS_DEFAULT_PRINTCAP "/etc/printcap"


/*
 * Default Samba and LPD config files...
 */

#define CUPS_DEFAULT_SMB_CONFIG_FILE ""
#define CUPS_DEFAULT_LPD_CONFIG_FILE ""


/*
 * Default MaxCopies value...
 */

#define CUPS_DEFAULT_MAX_COPIES 9999


/*
 * Do we have domain socket support?
 */

#define CUPS_DEFAULT_DOMAINSOCKET "/var/run/cups/cups.sock"


/*
 * Where are files stored?
 *
 * Note: These are defaults, which can be overridden by environment
 *       variables at run-time...
 */

#define CUPS_BINDIR "/usr/bin"
#define CUPS_CACHEDIR "/var/cache/cups"
#define CUPS_DATADIR "/usr/share/cups"
#define CUPS_DOCROOT "/usr/share/doc/cups"
#define CUPS_FONTPATH "/usr/share/cups/fonts"
#define CUPS_LOCALEDIR "/usr/share/locale"
#define CUPS_LOGDIR "/var/log/cups"
#define CUPS_REQUESTS "/var/spool/cups"
#define CUPS_SBINDIR "/usr/sbin"
#define CUPS_SERVERBIN "/usr/lib/cups"
#define CUPS_SERVERROOT "/etc/cups"
#define CUPS_STATEDIR "/var/run/cups"


/*
 * Do we have various image libraries?
 */

/* #define HAVE_LIBPNG 1 */
/* #define HAVE_LIBZ 1 */
/* #define HAVE_LIBJPEG 1 */
/* #define HAVE_LIBTIFF 1 */


/*
 * Do we have PAM stuff?
 */

#ifndef HAVE_LIBPAM
#define HAVE_LIBPAM 0
#endif /* !HAVE_LIBPAM */

/* #undef HAVE_PAM_PAM_APPL_H */
/* #undef HAVE_PAM_SET_ITEM */
/* #undef HAVE_PAM_SETCRED */


/*
 * Do we have <shadow.h>?
 */

#define HAVE_SHADOW_H 1


/*
 * Do we have <crypt.h>?
 */

#define HAVE_CRYPT_H 1


/*
 * Do we have <scsi/sg.h>?
 */

#define HAVE_SCSI_SG_H 1


/*
 * Use <string.h>, <strings.h>, and/or <bstring.h>?
 */

#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
/* #undef HAVE_BSTRING_H */

/*
 * Do we have the long long type?
 */

#define HAVE_LONG_LONG 1

#ifdef HAVE_LONG_LONG
#  define CUPS_LLFMT	"%lld"
#  define CUPS_LLCAST	(long long)
#else
#  define CUPS_LLFMT	"%ld"
#  define CUPS_LLCAST	(long)
#endif /* HAVE_LONG_LONG */

/*
 * Do we have the strtoll() function?
 */

#define HAVE_STRTOLL 1

#ifndef HAVE_STRTOLL
#  define strtoll(nptr,endptr,base) strtol((nptr), (endptr), (base))
#endif /* !HAVE_STRTOLL */

/*
 * Do we have the strXXX() functions?
 */

#define HAVE_STRDUP 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
/* #undef HAVE_STRLCAT */
/* #undef HAVE_STRLCPY */


/*
 * Do we have the geteuid() function?
 */

#define HAVE_GETEUID 1


/*
 * Do we have the vsyslog() function?
 */

#define HAVE_VSYSLOG 1


/*
 * Do we have the (v)snprintf() functions?
 */

#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1


/*
 * What signal functions to use?
 */

/* #undef HAVE_SIGSET */
#define HAVE_SIGACTION 1


/*
 * What wait functions to use?
 */

#define HAVE_WAITPID 1
#define HAVE_WAIT3 1


/*
 * Do we have the mallinfo function and malloc.h?
 */

/* #undef HAVE_MALLINFO */
#define HAVE_MALLOC_H 1


/*
 * Do we have the POSIX ACL functions?
 */

/* #undef HAVE_ACL_INIT */


/*
 * Do we have the langinfo.h header file?
 */

#define HAVE_LANGINFO_H 1


/*
 * Which encryption libraries do we have?
 */

/* #undef HAVE_CDSASSL */
/* #undef HAVE_GNUTLS */
/* #undef HAVE_LIBSSL */
/* #undef HAVE_SSL */


/*
 * What Security framework headers do we have?
 */

/* #undef HAVE_AUTHORIZATION_H */
/* #undef HAVE_SECITEMPRIV_H */
/* #undef HAVE_SECPOLICY_H */
/* #undef HAVE_SECPOLICYPRIV_H */
/* #undef HAVE_SECBASEPRIV_H */
/* #undef HAVE_SECIDENTITYSEARCHPRIV_H */


/*
 * Do we have the SecIdentitySearchCreateWithPolicy function?
 */

/* #undef HAVE_SECIDENTITYSEARCHCREATEWITHPOLICY */


/*
 * Do we have the SecPolicyCreateSSL function?
 */

/* #undef HAVE_SECPOLICYCREATESSL */


/*
 * Do we have the SLP library?
 */

/* #undef HAVE_LIBSLP */


/*
 * Do we have an LDAP library?
 */

/* #undef HAVE_LDAP */
/* #undef HAVE_OPENLDAP */
/* #undef HAVE_MOZILLA_LDAP */
/* #undef HAVE_LDAP_SSL_H */
/* #undef HAVE_LDAP_SSL */
/* #undef HAVE_LDAP_REBIND_PROC */


/*
 * Do we have libpaper?
 */

/* #undef HAVE_LIBPAPER */


/*
 * Do we have DNS Service Discovery (aka Bonjour)?
 */

/* #undef HAVE_DNSSD */


/*
 * Do we have <sys/ioctl.h>?
 */

#define HAVE_SYS_IOCTL_H 1


/*
 * Does the "stat" structure contain the "st_gen" member?
 */

/* #undef HAVE_ST_GEN */


/*
 * Does the "tm" structure contain the "tm_gmtoff" member?
 */

#define HAVE_TM_GMTOFF 1


/*
 * Do we have rresvport_af()?
 */

#define HAVE_RRESVPORT_AF 1


/*
 * Do we have getaddrinfo()?
 */

#define HAVE_GETADDRINFO 1


/*
 * Do we have getnameinfo()?
 */

#define HAVE_GETNAMEINFO 1


/*
 * Do we have getifaddrs()?
 */

#define HAVE_GETIFADDRS 1


/*
 * Do we have hstrerror()?
 */

#define HAVE_HSTRERROR 1


/*
 * Do we have res_init()?
 */

#define HAVE_RES_INIT 1


/*
 * Do we have <resolv.h>
 */

#define HAVE_RESOLV_H 1


/*
 * Do we have the <sys/sockio.h> header file?
 */

/* #undef HAVE_SYS_SOCKIO_H */


/*
 * Does the sockaddr structure contain an sa_len parameter?
 */

/* #undef HAVE_STRUCT_SOCKADDR_SA_LEN */


/*
 * Do we have the AIX usersec.h header file?
 */

/* #undef HAVE_USERSEC_H */


/*
 * Do we have pthread support?
 */

#define HAVE_PTHREAD_H 1


/*
 * Do we have launchd support?
 */

/* #undef HAVE_LAUNCH_H */
/* #undef HAVE_LAUNCHD */


/*
 * Various scripting languages...
 */

#define HAVE_JAVA 1
#define CUPS_JAVA "/usr/bin/java"
#define HAVE_PERL 1
#define CUPS_PERL "/usr/bin/perl"
#define HAVE_PHP 1
#define CUPS_PHP "no"
#define HAVE_PYTHON 1
#define CUPS_PYTHON "/usr/bin/python"


/*
 * Location of the poppler/Xpdf pdftops program...
 */

#define HAVE_PDFTOPS 1
#define CUPS_PDFTOPS "/usr/bin/pdftops"


/*
 * Location of the Ghostscript gs program...
 */

/* #undef HAVE_GHOSTSCRIPT */
#define CUPS_GHOSTSCRIPT ""


/*
 * Do we have Darwin's CoreFoundation and SystemConfiguration frameworks?
 */

/* #undef HAVE_COREFOUNDATION */
/* #undef HAVE_SYSTEMCONFIGURATION */


/*
 * Do we have CoreFoundation public and private headers?
 */

/* #undef HAVE_COREFOUNDATION_H */
/* #undef HAVE_CFPRIV_H */
/* #undef HAVE_CFBUNDLEPRIV_H */


/*
 * Do we have ApplicationServices public headers?
 */

/* #undef HAVE_APPLICATIONSERVICES_H */


/*
 * Do we have the SCDynamicStoreCopyComputerName function?
 */

/* #undef HAVE_SCDYNAMICSTORECOPYCOMPUTERNAME */


/*
 * Do we have MacOSX 10.4's mbr_XXX functions?
 */

/* #undef HAVE_MEMBERSHIP_H */
/* #undef HAVE_MEMBERSHIPPRIV_H */
/* #undef HAVE_MBR_UID_TO_UUID */


/*
 * Do we have Darwin's notify_post header and function?
 */

/* #undef HAVE_NOTIFY_H */
/* #undef HAVE_NOTIFY_POST */


/*
 * Do we have DBUS?
 */

/* #undef HAVE_DBUS */
/* #undef HAVE_DBUS_MESSAGE_ITER_INIT_APPEND */


/*
 * Do we have the AppleTalk/at_proto.h header?
 */

/* #undef HAVE_APPLETALK_AT_PROTO_H */


/*
 * Do we have the GSSAPI support library (for Kerberos support)?
 */

/* #undef HAVE_GSSAPI */
/* #undef HAVE_GSSAPI_H */
/* #undef HAVE_GSSAPI_GSSAPI_H */
/* #undef HAVE_GSSAPI_GSSAPI_GENERIC_H */
/* #undef HAVE_GSSAPI_GSSAPI_KRB5_H */
/* #undef HAVE_GSSKRB5_REGISTER_ACCEPTOR_IDENTITY */
/* #undef HAVE_GSS_C_NT_HOSTBASED_SERVICE */
/* #undef HAVE_KRB5_CC_NEW_UNIQUE */
/* #undef HAVE_KRB5_IPC_CLIENT_SET_TARGET_UID */
/* #undef HAVE_KRB5_H */
/* #undef HAVE_HEIMDAL */


/*
 * Default GSS service name...
 */

#define CUPS_DEFAULT_GSSSERVICENAME "host"


/*
 * Select/poll interfaces...
 */

#define HAVE_POLL 1
#define HAVE_EPOLL 1
/* #undef HAVE_KQUEUE */


/*
 * Do we have the <dlfcn.h> header?
 */

/* #undef HAVE_DLFCN_H */


/*
 * Do we have <sys/param.h>?
 */

#define HAVE_SYS_PARAM_H 1


/*
 * Do we have <sys/ucred.h>?
 */

/* #undef HAVE_SYS_UCRED_H */


/*
 * Do we have removefile()?
 */

/* #undef HAVE_REMOVEFILE */


/*
 * Do we have <sandbox.h>?
 */

/* #undef HAVE_SANDBOX_H */


/*
 * Which random number generator function to use...
 */

/* #undef HAVE_ARC4RANDOM */
#define HAVE_RANDOM 1
#define HAVE_LRAND48 1

#ifdef HAVE_ARC4RANDOM
#  define CUPS_RAND() arc4random()
#  define CUPS_SRAND(v) arc4random_stir()
#elif defined(HAVE_RANDOM)
#  define CUPS_RAND() random()
#  define CUPS_SRAND(v) srandom(v)
#elif defined(HAVE_LRAND48)
#  define CUPS_RAND() lrand48()
#  define CUPS_SRAND(v) srand48(v)
#else
#  define CUPS_RAND() rand()
#  define CUPS_SRAND(v) srand(v)
#endif /* HAVE_ARC4RANDOM */


/*
 * Do we have vproc_transaction_begin/end?
 */

/* #undef HAVE_VPROC_TRANSACTION_BEGIN */


/*
 * Do we have libusb?
 */

#define HAVE_USB_H 1


/*
 * Do we have libwrap and tcpd.h?
 */

/* #undef HAVE_TCPD_H */


/*
 * Do we have statfs or statvfs and one of the corresponding headers?
 */

#define HAVE_STATFS 1
#define HAVE_STATVFS 1
#define HAVE_SYS_MOUNT_H 1
#define HAVE_SYS_STATFS_H 1
#define HAVE_SYS_STATVFS_H 1
#define HAVE_SYS_VFS_H 1


/*
 * Location of OS X localization bundle, if any.
 */

/* #undef CUPS_BUNDLEDIR */


/*
 * Do we have XPC?
 */

/* #undef HAVE_XPC */
/* #undef HAVE_XPC_PRIVATE_H */


/*
 * Do we have Mini-XML?
 */

/* #undef HAVE_MXML_H */


/*
 * Do we have the C99 abs() function?
 */

#define HAVE_ABS 0
#if !defined(HAVE_ABS) && !defined(abs)
#  if defined(__GNUC__) || __STDC_VERSION__ >= 199901L
#    define abs(x) _cups_abs(x)
static inline int _cups_abs(int i) { return (i < 0 ? -i : i); }
#  elif defined(_MSC_VER)
#    define abs(x) _cups_abs(x)
static __inline int _cups_abs(int i) { return (i < 0 ? -i : i); }
#  else
#    define abs(x) ((x) < 0 ? -(x) : (x))
#  endif /* __GNUC__ || __STDC_VERSION__ */
#endif /* !HAVE_ABS && !abs */


#endif /* !_CUPS_CONFIG_H_ */

/*
 * End of "$Id: config.h.in 9259 2010-08-13 04:11:46Z mike $".
 */
