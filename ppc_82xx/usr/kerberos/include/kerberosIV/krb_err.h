/*
 * krb_err.h:
 * This file is automatically generated; please do not edit it.
 */

#if defined(macintosh) || (defined(__MACH__) && defined(__APPLE__))
#include <KerberosComErr/KerberosComErr.h>
#else
#include <com_err.h>
#endif

#define KRBET_KSUCCESS                           (39525376L)
#define KRBET_KDC_NAME_EXP                       (39525377L)
#define KRBET_KDC_SERVICE_EXP                    (39525378L)
#define KRBET_KDC_AUTH_EXP                       (39525379L)
#define KRBET_KDC_PKT_VER                        (39525380L)
#define KRBET_KDC_P_MKEY_VER                     (39525381L)
#define KRBET_KDC_S_MKEY_VER                     (39525382L)
#define KRBET_KDC_BYTE_ORDER                     (39525383L)
#define KRBET_KDC_PR_UNKNOWN                     (39525384L)
#define KRBET_KDC_PR_N_UNIQUE                    (39525385L)
#define KRBET_KDC_NULL_KEY                       (39525386L)
#define KRBET_KRB_RES11                          (39525387L)
#define KRBET_KRB_RES12                          (39525388L)
#define KRBET_KRB_RES13                          (39525389L)
#define KRBET_KRB_RES14                          (39525390L)
#define KRBET_KRB_RES15                          (39525391L)
#define KRBET_KRB_RES16                          (39525392L)
#define KRBET_KRB_RES17                          (39525393L)
#define KRBET_KRB_RES18                          (39525394L)
#define KRBET_KRB_RES19                          (39525395L)
#define KRBET_KDC_GEN_ERR                        (39525396L)
#define KRBET_GC_TKFIL                           (39525397L)
#define KRBET_GC_NOTKT                           (39525398L)
#define KRBET_KRB_RES23                          (39525399L)
#define KRBET_KRB_RES24                          (39525400L)
#define KRBET_KRB_RES25                          (39525401L)
#define KRBET_MK_AP_TGTEXP                       (39525402L)
#define KRBET_KRB_RES27                          (39525403L)
#define KRBET_KRB_RES28                          (39525404L)
#define KRBET_KRB_RES29                          (39525405L)
#define KRBET_KRB_RES30                          (39525406L)
#define KRBET_RD_AP_UNDEC                        (39525407L)
#define KRBET_RD_AP_EXP                          (39525408L)
#define KRBET_RD_AP_NYV                          (39525409L)
#define KRBET_RD_AP_REPEAT                       (39525410L)
#define KRBET_RD_AP_NOT_US                       (39525411L)
#define KRBET_RD_AP_INCON                        (39525412L)
#define KRBET_RD_AP_TIME                         (39525413L)
#define KRBET_RD_AP_BADD                         (39525414L)
#define KRBET_RD_AP_VERSION                      (39525415L)
#define KRBET_RD_AP_MSG_TYPE                     (39525416L)
#define KRBET_RD_AP_MODIFIED                     (39525417L)
#define KRBET_RD_AP_ORDER                        (39525418L)
#define KRBET_RD_AP_UNAUTHOR                     (39525419L)
#define KRBET_KRB_RES44                          (39525420L)
#define KRBET_KRB_RES45                          (39525421L)
#define KRBET_KRB_RES46                          (39525422L)
#define KRBET_KRB_RES47                          (39525423L)
#define KRBET_KRB_RES48                          (39525424L)
#define KRBET_KRB_RES49                          (39525425L)
#define KRBET_KRB_RES50                          (39525426L)
#define KRBET_GT_PW_NULL                         (39525427L)
#define KRBET_GT_PW_BADPW                        (39525428L)
#define KRBET_GT_PW_PROT                         (39525429L)
#define KRBET_GT_PW_KDCERR                       (39525430L)
#define KRBET_GT_PW_NULLTKT                      (39525431L)
#define KRBET_SKDC_RETRY                         (39525432L)
#define KRBET_SKDC_CANT                          (39525433L)
#define KRBET_KRB_RES58                          (39525434L)
#define KRBET_KRB_RES59                          (39525435L)
#define KRBET_KRB_RES60                          (39525436L)
#define KRBET_INTK_W_NOTALL                      (39525437L)
#define KRBET_INTK_BADPW                         (39525438L)
#define KRBET_INTK_PROT                          (39525439L)
#define KRBET_KRB_RES64                          (39525440L)
#define KRBET_KRB_RES65                          (39525441L)
#define KRBET_KRB_RES66                          (39525442L)
#define KRBET_KRB_RES67                          (39525443L)
#define KRBET_KRB_RES68                          (39525444L)
#define KRBET_KRB_RES69                          (39525445L)
#define KRBET_INTK_ERR                           (39525446L)
#define KRBET_AD_NOTGT                           (39525447L)
#define KRBET_KRB_RES72                          (39525448L)
#define KRBET_KRB_RES73                          (39525449L)
#define KRBET_KRB_RES74                          (39525450L)
#define KRBET_KRB_RES75                          (39525451L)
#define KRBET_NO_TKT_FIL                         (39525452L)
#define KRBET_TKT_FIL_ACC                        (39525453L)
#define KRBET_TKT_FIL_LCK                        (39525454L)
#define KRBET_TKT_FIL_FMT                        (39525455L)
#define KRBET_TKT_FIL_INI                        (39525456L)
#define KRBET_KNAME_FMT                          (39525457L)
#define ERROR_TABLE_BASE_krb (39525376L)

extern struct error_table et_krb_error_table;

#if !defined(_MSDOS) && !defined(_WIN32) && !defined(macintosh) && !(defined(__MACH__) && defined(__APPLE__))
/* for compatibility with older versions... */
extern void initialize_krb_error_table ();
#define init_krb_err_tbl initialize_krb_error_table
#define krb_err_base ERROR_TABLE_BASE_krb
#else
#define initialize_krb_error_table()
#endif