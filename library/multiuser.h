#ifndef LIBRARIES_MULTIUSER_H
#define LIBRARIES_MULTIUSER_H
/*
**      $VER: multiuser.h 39.14 (29.6.94)
**      MultiUser Release 1.8
**
**      multiuser.library definitions
**
**      (C) Copyright 1993-1994 Geert Uytterhoeven
**          All Rights Reserved
*/

/* Geert gave me permission to distribute this header with the ixemul
   distribution. Hans Verkuil, 22 Apr 1996. */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif  /* EXEC_TYPES_H */

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif  /* EXEC_LISTS_H */

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif  /* EXEC_LIBRARIES_H */

#ifndef EXEC_EXECBASE_H
#include <exec/execbase.h>
#endif  /* EXEC_EXECBASE_H */

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif  /* EXEC_PORTS_H */

#ifndef LIBRARIES_DOS_H
#include <libraries/dos.h>
#endif  /* LIBRARIES_DOS_H */

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif  /* UTILITY_TAGITEM_H */

#ifndef LIBRARIES_LOCALE_H
#include <libraries/locale.h>
#endif /* LIBRARIES_LOCALE_H */

#define MULTIUSERNAME         "multiuser.library"
#define MULTIUSERVERSION      (39)

#define MULTIUSERCATALOGNAME    "multiuser.catalog"
#define MULTIUSERCATALOGVERSION (1)

   /*
    *    Reserved users/groups
    *
    *    WARNING: a uid may NOT be zero, a gid may be zero
    */

#define muOWNER_NOBODY  (0x00000000)   /* no rights */
#define muOWNER_SYSTEM  (0xffffffff)   /* always owner */

#define muMASK_UID      (0xffff0000)   /* Mask for uid bits */
#define muMASK_GID      (0x0000ffff)   /* Mask for gid bits */

#define muROOT_UID      (0xffff)       /* super user uid */
#define muROOT_GID      (0xffff)       /* super user gid */

#define muNOBODY_UID    (0x0000)       /* nobody uid */

#define muUSERIDSIZE    (32)           /* Maximum size for a User ID */
#define muGROUPIDSIZE   (32)           /* Maximum size for a Group ID */
#define muPASSWORDSIZE  (32)           /* Maximum size for a Password */
#define muUSERNAMESIZE  (220)          /* Maximum size for a User Name */
#define muGROUPNAMESIZE (220)          /* Maximum size for a Group Name */
#define muHOMEDIRSIZE   (256)          /* Maximum size for a Home Directory */
#define muSHELLSIZE     (256)          /* Maximum size for a Shell */


   /*
    *    Password File
    *
    *
    *    For each user, the Password File must contain a line like this:
    *
    *    <UserID>|<Password>|<uid>|<gid>|<UserName>|<HomeDir>|<Shell>
    *
    *    with:
    *
    *       <UserID>    User Login ID (max. muUSERIDSIZE-1 characters)
    *       <PassKey>   Encrypted Password
    *       <uid>       User Number (1 - 65535)
    *       <gid>       Primary Group Number (0 - 65535)
    *       <UserName>  Full User Name (max. muUSERNAMESIZE-1 characters)
    *       <HomeDir>   Home directory (max. muHOMEDIRSIZE-1 characters)
    *       <Shell>     Default Shell (max. muSHELLSIZE-1 characters)
    *                   (not used yet, AS225 compatibility)
    */

#define muPasswd_FileName     "passwd"  /* for AS225 compatibility */


   /*
    *    Group File
    *
    *
    *    This file gives more information about the groups and defines
    *    the secondary groups (other than the primary group) a user
    *    belongs to. It exists out of two parts, separated by a blank line.
    *
    *    The first part contains lines with the format:
    *
    *    <GroupID>|<gid>|<MgrUid>|<GroupName>
    *
    *    with:
    *
    *       <GroupID>   Group short ID (max. muGROUPIDSIZE-1 characters)
    *       <gid>       Group Number (0 - 65535)
    *       <MgrUid>    User Number of this group's manager, 0 for no
    *                   manager. A group's manager must not belong to the
    *                   group.
    *       <GroupName> Full Group Name (max. muGROUPNAMESIZE-1 characters)
    *
    *    NOTE: Not every group must have a line in this file, but at least
    *          one group must have one.
    *
    *
    *    The seconds part contains lines with the format:
    *
    *    <uid>:<gid>[,<gid>...]
    *
    *    with:
    *
    *       <uid>       User Number (1-65535)
    *       <gid>       Group Number (0 - 65535)
    *
    *    If you think you'll exceed the maximum line length (circa 1K),
    *    you may use more than one line per user.
    */

#define muGroup_FileName      "MultiUser.group"


   /*
    *    Configuration File
    *
    *
    *    This file contains lines with options in the form <OPT>=<val>.
    *    0 is used for OFF, 1 for ON.
    *    Defaults to the values between square brackets.
    *
    *    LIMITDOSSETPROTECTION   dos.library/SetProtection() cannot change
    *                            protection bits for GROUP and OTHER [1]
    *    PROFILE                 execute the Profile if it exists [1]
    *    LASTLOGINREQ            display the Lastlogin requester [1]
    *    LOGSTARTUP              log startup [0]
    *    LOGLOGIN                log successful logins [0]
    *    LOGLOGINFAIL            log failed logins [0]
    *    LOGPASSWD               log successful password changes [0]
    *    LOGPASSWDFAIL           log failed password changes [0]
    *    LOGCHECKPASSWD          log successful password checks [0]
    *    LOGCHECKPASSWDFAIL      log failed password checks [0]
    *    PASSWDUIDLEVEL          users with a uid greather than or equal to
    *                            <val> can change their passwords [0]
    *    PASSWDGIDLEVEL          users with a gid greather than or equal to
    *                            <val> can change their passwords [0]
    *
    *    NOTE: if a user has a uid less than the PASSWDUIDLEVEL AND a gid
    *          less than PASSWDGIDLEVEL he/she is NOT allowed to change
    *          his/her password!
    */

#define muConfig_FileName     "MultiUser.config"


   /*
    *    Log File
    */

#define muLog_FileName        "MultiUser.log"


   /*
    *    Lastlogin File
    */

#define muLastLogin_FileName  ".lastlogin"


   /*
    *    Profile
    */

#define muProfile_FileName    ".profile"


   /*
    *    Plan file
    */

#define muPlan_FileName       ".plan"


   /*
    *    Key File
    *
    *
    *    This file must be present in the root directory of every volume
    *    that uses the MultiUserFileSystem. It must contain 3 lines:
    *
    *       - a pseudo random ASCII key (max. 1023 characters).
    *       - the directory of the password file, if located on this volume,
    *         otherwise an empty line (no spaces!).
    *           e.g. ":MultiUser"
    *           e.g. ":inet/db" for AS225 compatibility
    *       - the directory of the configuration file, if located on this
    *         volume, otherwise an empty line (no spaces!).
    *           e.g. ":MultiUser"
    *
    *    If there is ANY inconsistency the system will refuse to work!!
    */

#define muKey_FileName        ":.MultiUser.keyfile"


   /*
    *    Tags for muLogoutA()
    *             muLoginA()
    *             muSetDefProtectionA()
    */

#define muT_Input          (TAG_USER+1)   /* filehandle - default is Input() */
#define muT_Output         (TAG_USER+2)   /* filehandle - default is Output() */
#define muT_Graphical      (TAG_USER+3)   /* boolean - default is FALSE */
#define muT_PubScrName     (TAG_USER+4)   /* name of public screen */
#define muT_Task           (TAG_USER+5)   /* task (NOT the name!!) */
#define muT_Own            (TAG_USER+6)   /* make a task owned by this user */
#define muT_Global         (TAG_USER+7)   /* change it for all tasks on the */
					  /* same level as this one */
#define muT_Quiet          (TAG_USER+8)   /* for muLogoutA(), don't give a */
					  /* login prompt, simply logout */
#define muT_UserID         (TAG_USER+9)   /* UserID for muLoginA() */
#define muT_Password       (TAG_USER+10)  /* Password for muLoginA(), must */
					  /* be combined with muT_UserID!! */
#define muT_DefProtection  (TAG_USER+11)  /* Default protection bits */
					  /* default is RWED GROUP R OTHER R */
#define muT_All            (TAG_USER+12)  /* for muLogoutA(), logout until */
					  /* user stack is empty */
#define muT_NoLog          (TAG_USER+13)  /* for muLoginA(), only root */


   /*
    *    Public User Information Structure
    *
    *
    *    For future compatibility, you should ALWAYS use muAllocUserInfo()
    *    to allocate this structure. NEVER do it by yourself!!
    */

struct muUserInfo {
   char UserID[muUSERIDSIZE];
   UWORD uid;
   UWORD gid;
   char UserName[muUSERNAMESIZE];
   char HomeDir[muHOMEDIRSIZE];
   UWORD NumSecGroups;              /* Number of Secondary Groups this */
				    /* user belongs to */
   UWORD *SecGroups;                /* Points to an array of NumSecGroups */
				    /* Secondary Group Numbers */
   char Shell[muSHELLSIZE];
};


   /*
    *    Public Group Information Structure
    *
    *
    *    For future compatibility, you should ALWAYS use muAllocGroupInfo()
    *    to allocate this structure. NEVER do it by yourself!!
    */

struct muGroupInfo {
   char GroupID[muGROUPIDSIZE];
   UWORD gid;
   UWORD MgrUid;                    /* Manager of this group */
   char GroupName[muGROUPNAMESIZE];
};


   /*
    *    KeyTypes for muGetUserInfo()
    *                 muGetGroupInfo()
    */

#define muKeyType_First          (0)
#define muKeyType_Next           (1)
#define muKeyType_gid            (4)

   /*
    *    KeyTypes for muGetUserInfo() only
    */

#define muKeyType_UserID         (2)   /* Case-sensitive */
#define muKeyType_uid            (3)
#define muKeyType_gidNext        (5)
#define muKeyType_UserName       (6)   /* Case-insensitive */
#define muKeyType_WUserID        (7)   /* Case-insensitive, wild cards allowed */
#define muKeyType_WUserName      (8)   /* Case-insensitive, wild cards allowed */
#define muKeyType_WUserIDNext    (9)
#define muKeyType_WUserNameNext  (10)

   /*
    *    KeyTypes for muGetGroupInfo() only
    */

#define muKeyType_GroupID        (11)  /* Case-sensitive */
#define muKeyType_WGroupID       (12)  /* Case-insensitive, wild cards allowed */
#define muKeyType_WGroupIDNext   (13)
#define muKeyType_GroupName      (14)  /* Case-insensitive */
#define muKeyType_WGroupName     (15)  /* Case-insensitive, wild cards allowed */
#define muKeyType_WGroupNameNext (16)
#define muKeyType_MgrUid         (17)
#define muKeyType_MgrUidNext     (18)


   /*
    *    Extended Owner Information Structure
    *
    *
    *    A pointer to this structure is returned by muGetTaskExtOwner().
    *    You MUST use muFreeExtOwner() to deallocate it!!
    */

struct muExtOwner {
   UWORD uid;
   UWORD gid;
   UWORD NumSecGroups;              /* Number of Secondary Groups this */
				    /* user belongs too. */
};

   /* NOTE: This structure is followed by a UWORD array containing
    *       the Secondary Group Numbers
    *       Use the following macro to access these group numbers,
    *       e.g. sgid = muSecGroups(extowner)[i];
    *
    *       Do not use this macro on a NULL pointer!!
    */

#define muSecGroups(x) ((UWORD *)((UBYTE *)x+sizeof(struct muExtOwner)))


   /*
    *    Macro to convert an Extended Owner Information Structure to a ULONG
    *    (cfr. muGetTaskOwner())
    */

#define muExtOwner2ULONG(x) ((ULONG)(x ? (x)->uid<<16|(x)->gid : muOWNER_NOBODY))


   /*
    *    Packet types (see also <dos/dosextens.h> :-)
    */

/* #define ACTION_SET_OWNER        1036 */


   /*
    *    Protection bits (see also <dos/dos.h> :-)
    */

#define muFIBB_SET_UID        (31)  /* Change owner during execution */
#define muFIBB_SET_GID        (30)  /* Change group during execution - not yet implemented */

/* FIBB are bit definitions, FIBF are field definitions */
/* Regular RWED bits are 0 == allowed. */
/* NOTE: GRP and OTR RWED permissions are 0 == not allowed! */

/* #define FIBB_OTR_READ      15   Other: file is readable */
/* #define FIBB_OTR_WRITE     14   Other: file is writable */
/* #define FIBB_OTR_EXECUTE   13   Other: file is executable */
/* #define FIBB_OTR_DELETE    12   Other: prevent file from being deleted */
/* #define FIBB_GRP_READ      11   Group: file is readable */
/* #define FIBB_GRP_WRITE     10   Group: file is writable */
/* #define FIBB_GRP_EXECUTE   9    Group: file is executable */
/* #define FIBB_GRP_DELETE    8    Group: prevent file from being deleted */

#define FIBB_HOLD             7

/* #define FIBB_SCRIPT        6    program is a script (execute) file */
/* #define FIBB_PURE          5    program is reentrant and rexecutable */
/* #define FIBB_ARCHIVE       4    cleared whenever file is changed */
/* #define FIBB_READ          3    ignored by old filesystem */
/* #define FIBB_WRITE         2    ignored by old filesystem */
/* #define FIBB_EXECUTE       1    ignored by system, used by Shell */
/* #define FIBB_DELETE        0    prevent file from being deleted */

#define muFIBF_SET_UID        (1<<muFIBB_SET_UID)
#define muFIBF_SET_GID        (1<<muFIBB_SET_GID)

/* #define FIBF_OTR_READ      (1<<FIBB_OTR_READ) */
/* #define FIBF_OTR_WRITE     (1<<FIBB_OTR_WRITE) */
/* #define FIBF_OTR_EXECUTE   (1<<FIBB_OTR_EXECUTE) */
/* #define FIBF_OTR_DELETE    (1<<FIBB_OTR_DELETE) */
/* #define FIBF_GRP_READ      (1<<FIBB_GRP_READ) */
/* #define FIBF_GRP_WRITE     (1<<FIBB_GRP_WRITE) */
/* #define FIBF_GRP_EXECUTE   (1<<FIBB_GRP_EXECUTE) */
/* #define FIBF_GRP_DELETE    (1<<FIBB_GRP_DELETE) */

#define FIBF_HOLD             (1<<FIBB_HOLD)

/* #define FIBF_SCRIPT        (1<<FIBB_SCRIPT) */
/* #define FIBF_PURE          (1<<FIBB_PURE) */
/* #define FIBF_ARCHIVE       (1<<FIBB_ARCHIVE) */
/* #define FIBF_READ          (1<<FIBB_READ) */
/* #define FIBF_WRITE         (1<<FIBB_WRITE) */
/* #define FIBF_EXECUTE       (1<<FIBB_EXECUTE) */
/* #define FIBF_DELETE        (1<<FIBB_DELETE) */


   /*
    *    Default Protection Bits
    */

#define DEFPROTECTION (FIBF_OTR_READ | FIBF_GRP_READ)


   /*
    *    Relations returned by muGetRelationshipA()
    */

#define muRelB_ROOT_UID    (0)   /* User == super user */
#define muRelB_ROOT_GID    (1)   /* User belongs to the super user group */
#define muRelB_NOBODY      (2)   /* User == nobody */
#define muRelB_UID_MATCH   (3)   /* User == owner */
#define muRelB_GID_MATCH   (4)   /* User belongs to owner group */
#define muRelB_PRIM_GID    (5)   /* User's primary group == owner group */
#define muRelB_NO_OWNER    (6)   /* Owner == nobody */

#define muRelF_ROOT_UID    (1<<muRelB_ROOT_UID)
#define muRelF_ROOT_GID    (1<<muRelB_ROOT_GID)
#define muRelF_NOBODY      (1<<muRelB_NOBODY)
#define muRelF_UID_MATCH   (1<<muRelB_UID_MATCH)
#define muRelF_GID_MATCH   (1<<muRelB_GID_MATCH)
#define muRelF_PRIM_GID    (1<<muRelB_PRIM_GID)
#define muRelF_NO_OWNER    (1<<muRelB_NO_OWNER)


   /*
    *    Monitor Structure
    *
    *
    *    The use of this structure is restricted to root.
    *    Do not modify or reuse this structure while it is active!
    */

struct muMonitor {
   struct MinNode Node;
   ULONG Mode;                      /* see definitions below */
   ULONG Triggers;                  /* see definitions below */
   union {
      struct {                      /* for SEND_SIGNAL */
	 struct Task *Task;
	 ULONG SignalNum;
      } Signal;

      struct {                      /* for SEND_MESSAGE */
	 struct MsgPort *Port;
      } Message;
   } s;

   /* NOTE: This structure may be extended in future! */
};

   /*
    *    Monitor Modes
    */

#define muMon_IGNORE       (0)
#define muMon_SEND_SIGNAL  (1)
#define muMon_SEND_MESSAGE (2)

   /*
    *    Monitor Message
    *
    *
    *    Sent to the application if SEND_MESSAGE is specified.
    *    Do NOT forget to reply!
    */

struct muMonMsg {
   struct Message ExecMsg;
   struct muMonitor *Monitor;       /* The monitor that sent the message */
   ULONG Trigger;                   /* The trigger that caused the message */
   UWORD From;
   UWORD To;
   char UserID[muUSERIDSIZE];
};

   /*
    *    Monitor Triggers
    */

#define muTrgB_OwnerChange       (0)   /* Task Owner Change */
				       /*    From:    uid of old user */
				       /*    To:      uid of new user */
#define muTrgB_Login             (1)   /* successful Login/Logout */
				       /*    From:    uid of old user */
				       /*    To:      uid of new user */
				       /*    UserID:  UserID of new user */
#define muTrgB_LoginFail         (2)   /* unsuccessful Login/Logout */
				       /*    From:    uid of old user */
				       /*    UserID:  UserID of new user */
#define muTrgB_Passwd            (3)   /* successful Passwd */
				       /*    From:    uid of user */
#define muTrgB_PasswdFail        (4)   /* unsuccessful Passwd */
				       /*    From:    uid of user */
#define muTrgB_CheckPasswd       (5)   /* successful CheckPasswd */
				       /*    From:    uid of user */
#define muTrgB_CheckPasswdFail   (6)   /* unsuccessful CheckPasswd */
				       /*    From:    uid of user */

#define muTrgF_OwnerChange       (1<<muTrgB_OwnerChange)
#define muTrgF_Login             (1<<muTrgB_Login)
#define muTrgF_LoginFail         (1<<muTrgB_LoginFail)
#define muTrgF_Passwd            (1<<muTrgB_Passwd)
#define muTrgF_PasswdFail        (1<<muTrgB_PasswdFail)
#define muTrgF_CheckPasswd       (1<<muTrgB_CheckPasswd)
#define muTrgF_CheckPasswdFail   (1<<muTrgB_CheckPasswdFail)


#endif  /* LIBRARIES_MULTIUSER_H */
