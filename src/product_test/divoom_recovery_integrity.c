#if !defined(_WIN32)
#include <sys/errno.h>
#endif
#include "divoom.h"
#include "divoom_device_error.h"
#include "divoom_recovery_journal.h"
#include "divoom_recovery_integrity.h"
#include "divoom_product_test_format.h"
#include "divoom_product_test_userdata_wifi.h"
#include "divoom_product_test_wifi_fw.h"
#include "divoom_product_test_repair.h"
#include "divoom_boot_health.h"
#include "divoom_port_fs_check.h"
#include <stdlib.h>
#include <string.h>
#define MANUAL_USR_FORMAT 2U
#define MANUAL_USERDATA_FORMAT 16U
#define POST_FORMAT_VERIFY(usr) (0x400U << (usr))
static divoom_recovery_integrity_failure failure={DIVOOM_RECOVERY_FAILURE_NONE,-1,0,0,0,"none"};
void divoom_recovery_integrity_clear_failure(void)
{
 failure=(divoom_recovery_integrity_failure){DIVOOM_RECOVERY_FAILURE_NONE,-1,0,0,0,"none"};
}
void divoom_recovery_integrity_get_failure(divoom_recovery_integrity_failure *out)
{
 if(out)*out=failure;
}
static int resource_error(int error)
{
 return error==ENOMEM || error==EMFILE || error==ENFILE;
}
static void failed(divoom_recovery_failure_kind kind,int usr,int rc,int error,const char *stage)
{
 if(failure.kind!=DIVOOM_RECOVERY_FAILURE_NONE)return;
 if(rc==FF_CHECK_NOMEM || resource_error(error))kind=DIVOOM_RECOVERY_FAILURE_RESOURCE;
 int raw_error=error;
 if(!error)error=rc==FF_CHECK_NOMEM?ENOMEM:rc==FF_CHECK_BUSY?EBUSY:
    rc==FF_CHECK_UNSUPPORTED?ENOSYS:EIO;
 failure=(divoom_recovery_integrity_failure){kind,usr,rc,error,raw_error,stage};
}
static void *recovery_alloc(size_t size)
{
 errno=0;
 void *p=malloc(size);
 if(!p)failed(DIVOOM_RECOVERY_FAILURE_RESOURCE,-1,FF_CHECK_NOMEM,errno,"allocate");
 return p;
}
static int save_record(const divoom_recovery_record *record)
{
 errno=0;
 int ok=divoom_recovery_journal_save(record),error=errno;
 if(!ok)failed(DIVOOM_RECOVERY_FAILURE_JOURNAL,-1,0,error,"journal.save");
 return ok;
}
static int clear_record(void)
{
 errno=0;
 int ok=divoom_recovery_journal_clear(),error=errno;
 if(!ok)failed(DIVOOM_RECOVERY_FAILURE_JOURNAL,-1,0,error,"journal.clear");
 return ok;
}
static const char *device(int usr){return usr?"/dev/usr":"/dev/UDISK";}
static const char *mountpoint(int usr){return usr?"/usr":"/userdata";}
static int write_block(int usr,int blocked)
{
 errno=0;
 int rc=divoom_port_fs_write_block(device(usr),blocked),error=errno;
 /* This API only changes a devnode barrier. Its failure does not prove
  * that FAT or any file on the volume is damaged. */
 if(rc)failed(DIVOOM_RECOVERY_FAILURE_STATE,usr,rc,error,blocked?"write_block":"write_unblock");
 return rc;
}
static int userdata_error_clear(void)
{
 errno=0;
 int ok=divoom_device_error_userdata_repair_complete(),error=errno;
 if(!ok){
  int disk_error=error==EIO || error==ENOSPC || error==EROFS || error==EACCES;
  failed(disk_error?DIVOOM_RECOVERY_FAILURE_VOLUME:DIVOOM_RECOVERY_FAILURE_STATE,
      0,0,error,"userdata_error.clear");
 }
 return ok;
}
static int userdata_action(int restore)
{
 divoom_product_test_repair_io_error_target(1);
 errno=0;
 int ok=restore?divoom_product_test_userdata_wifi_restore():divoom_product_test_userdata_wifi_prepare();
 int error=errno,target=divoom_product_test_repair_io_error_target(1);
 if(!ok){
  /* No located file I/O means a failed journal/config checkpoint. Never
   * invent a disk failure from a boolean return or a stale errno. */
  int volume=target>=0;
  divoom_recovery_failure_kind kind=volume?DIVOOM_RECOVERY_FAILURE_VOLUME:
      restore?DIVOOM_RECOVERY_FAILURE_STATE:DIVOOM_RECOVERY_FAILURE_JOURNAL;
  failed(kind,volume?target:-1,0,error,restore?"userdata_config.restore":"userdata_config.prepare");
 }
 return ok;
}
static void progress(uint32_t stage,uint32_t done,uint32_t total,void *context)
{
 int usr=*(int*)context;
 divoom_product_test_format_fs_progress(usr,stage,done,total);
}
static int scan_volume(int usr,ff_check_result *result,ff_check_entry *entries)
{
 int rc=FF_CHECK_IO;
 for(int attempt=0;attempt<3;attempt++){
  memset(result,0,sizeof(*result));
  errno=0;
  rc=divoom_port_fs_check_volume(device(usr),mountpoint(usr),result,entries,
      entries?DIVOOM_RECOVERY_PLAN_MAX:0,progress,&usr);
  if(rc<0 && resource_error(errno)){failed(DIVOOM_RECOVERY_FAILURE_RESOURCE,usr,rc,errno,"scan");return FF_CHECK_NOMEM;}
  if(rc!=FF_CHECK_IO)return rc;
  if(attempt<2)divoom_system_delay_ms(1000);
 }
 for(int attempt=0;attempt<3;attempt++){
  divoom_system_delay_ms(1000);
  errno=0;
  int remount=divoom_port_fs_remount_volume(device(usr),mountpoint(usr));
  if(remount && (resource_error(errno)||remount==FF_CHECK_BUSY||remount==FF_CHECK_NOMEM||remount==FF_CHECK_UNSUPPORTED)){
   failed(DIVOOM_RECOVERY_FAILURE_VOLUME,usr,remount,errno,"remount");
   return resource_error(errno)?FF_CHECK_NOMEM:remount;
  }
  memset(result,0,sizeof(*result));
  errno=0;
  rc=divoom_port_fs_check_volume(device(usr),mountpoint(usr),result,entries,
      entries?DIVOOM_RECOVERY_PLAN_MAX:0,progress,&usr);
  if(rc<0 && resource_error(errno)){failed(DIVOOM_RECOVERY_FAILURE_RESOURCE,usr,rc,errno,"scan");return FF_CHECK_NOMEM;}
  if(rc!=FF_CHECK_IO)return rc;
 }
 return rc;
}
static int scan(int usr, ff_check_result *result, ff_check_entry *entries, const char *stage)
{
 divoom_product_test_format_set_error_files(-1);
 divoom_product_test_format_fs_progress(usr, FF_CHECK_STAGE_FAT, 0, 0);
 divoom_print("fs_integrity: stage=%s volume=%s begin\n", stage, mountpoint(usr));
 divoom_recovery_integrity_clear_failure();
 int rc=scan_volume(usr,result,entries);
 if(rc<0)failed(DIVOOM_RECOVERY_FAILURE_VOLUME,usr,rc,errno,stage);
 if(rc==FF_CHECK_STRUCTURAL||rc==FF_CHECK_IO)
  divoom_device_error_record(usr?DIVOOM_SD_NAND_ERR_USR_FORMAT:DIVOOM_SD_NAND_ERR_USERDATA);
 if((rc==FF_CHECK_CLEAN||rc==FF_CHECK_STRUCTURAL)&&result->complete)
  divoom_product_test_format_set_error_files(result->affected_files>0x7fffffffU?0x7fffffff:(int)result->affected_files);
 divoom_product_test_format_fs_result(usr,rc==FF_CHECK_CLEAN&&result->complete);
 divoom_print("fs_integrity: volume=%s result=%d flags=%u complete=%u unsafe=%u affected=%u orphan=%u stage=%s\n",
  mountpoint(usr),rc,result->damage_flags,result->complete,result->unsafe,result->affected_files,result->orphan_clusters,stage);
 return rc;
}

static int can_delete(const ff_check_result *result)
{
 return result->complete && !result->unsafe && result->plan_complete &&
    result->affected_files && result->affected_files<=DIVOOM_RECOVERY_PLAN_MAX;
}

static int only_orphans(int rc,const ff_check_result *result)
{
 return rc==FF_CHECK_STRUCTURAL && result->complete && !result->screen_only &&
    result->damage_flags==FF_CHECK_ORPHAN && result->orphan_clusters;
}

static int reclaim_and_verify(int usr,divoom_recovery_record *record,
    ff_check_result *result,ff_check_entry *entries,int resume);

int divoom_recovery_integrity_preview(divoom_product_test_repair_plan *plan)
{
 divoom_recovery_integrity_clear_failure();
 ff_check_entry *entries=recovery_alloc(DIVOOM_RECOVERY_PLAN_MAX*sizeof(*entries));
 if(!entries)return 0;
 int ok=1;
 for(int usr=0;usr<2;usr++){
  if(plan->format_mask&(1U<<usr))continue;
  ff_check_result result={0};
  int rc=scan(usr,&result,entries,"preview");
  if(only_orphans(rc,&result)){
   /* Space reclamation is automatic, including before the manual repair
    * dialog. The journal keeps its manual origin if power is lost here. */
   divoom_recovery_record *record=recovery_alloc(sizeof(*record));
   if(!record){ok=0;break;}
   memcpy(record,divoom_recovery_journal_get(),sizeof(*record));
   if(record->pending_mask){failed(DIVOOM_RECOVERY_FAILURE_JOURNAL,-1,FF_CHECK_BUSY,EBUSY,"preview.pending");free(record);ok=0;break;}
   record->flags|=DIVOOM_RECOVERY_MENU_CHECK;
   rc=reclaim_and_verify(usr,record,&result,entries,0);
   free(record);
   if(rc!=FF_CHECK_CLEAN || !clear_record()){ok=0;break;}
  }
  if(rc==FF_CHECK_STRUCTURAL && can_delete(&result))
   plan->structure_files[usr]=(int)result.affected_files;
  else if(rc==FF_CHECK_STRUCTURAL||rc==FF_CHECK_IO)
   plan->format_mask|=1U<<usr;
  else if(rc!=FF_CHECK_CLEAN){
   ok=0;break;
  }
 }
 free(entries);
 return ok;
}

static int commit(int phase,void *context)
{
 divoom_recovery_record *record=context;
 record->phase=phase==FF_CHECK_ORPHAN_PENDING?DIVOOM_RECOVERY_ORPHANS:
    phase==FF_CHECK_DETACH_PENDING?DIVOOM_RECOVERY_DETACH:DIVOOM_RECOVERY_FREE;
 return save_record(record);
}

static void reclaim_progress(uint32_t stage,uint32_t done,uint32_t total,void *context)
{
 divoom_recovery_record *record=context;
 divoom_product_test_format_fs_progress(record->target_usr,stage,done,total);
}

/* Both volumes are write-blocked. Only the native checker can authorize
 * releasing clusters after a complete, idle ownership scan. */
static int reclaim_and_verify(int usr,divoom_recovery_record *record,
    ff_check_result *result,ff_check_entry *entries,int resume)
{
 int rc;
 if(!resume){
  record->geometry=result->geometry;
  record->target_usr=usr;record->pending_mask|=1U<<usr;
  record->plan_count=0;
 }
 divoom_recovery_integrity_clear_failure();
 memset(result,0,sizeof(*result));
 errno=0;
 rc=divoom_port_fs_reclaim_orphans(device(usr),mountpoint(usr),&record->geometry,
     resume,result,reclaim_progress,commit,record);
 int error=errno;
 if(rc!=FF_CHECK_CLEAN)failed(DIVOOM_RECOVERY_FAILURE_VOLUME,usr,rc,error,"reclaim_orphans");
 divoom_print("fs_integrity: volume=%s reclaim_orphans resume=%d result=%d clusters=%u io=%u\n",
     mountpoint(usr),resume,rc,result->orphan_clusters,result->fs_error);
 if(rc!=FF_CHECK_CLEAN){
  /* A failed independent journal commit cannot be fixed by clearing FAT. */
  if(rc==FF_CHECK_IO && result->fs_error)
   divoom_product_test_repair_record_io(usr?"/usr/":"/userdata/","reclaim_orphans",EIO);
  return rc==FF_CHECK_IO?FF_CHECK_BUSY:rc;
 }
 rc=scan(usr,result,entries,"post_reclaim");
 if(rc==FF_CHECK_CLEAN){
  record->phase=DIVOOM_RECOVERY_VERIFY;record->reserved=0;
  if(!save_record(record))return FF_CHECK_BUSY;
 }
 return rc;
}

static int scan_and_reclaim(int usr,divoom_recovery_record *record,
    ff_check_result *result,ff_check_entry *entries,const char *stage)
{
 int resume=record->phase==DIVOOM_RECOVERY_ORPHANS && record->target_usr==(unsigned)usr;
 if(!resume){
  int rc=scan(usr,result,entries,stage);
  if(!only_orphans(rc,result))return rc;
 }
 return reclaim_and_verify(usr,record,result,entries,resume);
}
static int delete_batch(int usr,divoom_recovery_record *record)
{
 divoom_recovery_integrity_clear_failure();
 errno=0;
 int rc=divoom_port_fs_delete_corrupt_batch(device(usr),mountpoint(usr),&record->geometry,
     record->plan,record->plan_count,commit,record);
 int error=errno;
 if(rc!=FF_CHECK_CLEAN)failed(DIVOOM_RECOVERY_FAILURE_VOLUME,usr,rc,error,"delete_batch");
 return failure.kind==DIVOOM_RECOVERY_FAILURE_JOURNAL || failure.kind==DIVOOM_RECOVERY_FAILURE_RESOURCE?FF_CHECK_BUSY:rc;
}
static int allow_format(divoom_recovery_record *record,int usr)
{
 if(!(record->flags&DIVOOM_RECOVERY_MENU_CHECK) ||
    (record->flags&DIVOOM_RECOVERY_FORMAT_APPROVAL(usr)))return 1;
 divoom_product_test_repair_plan plan={.format_mask=1U<<usr};
 if(!divoom_product_test_format_allow_repair(&plan)){failed(DIVOOM_RECOVERY_FAILURE_NONE,-1,0,ECANCELED,"approval.cancelled");return 0;}
 record->flags|=DIVOOM_RECOVERY_FORMAT_APPROVAL(usr);
 return 1;
}

static int require_format(divoom_recovery_record *record,int usr)
{
 divoom_recovery_integrity_clear_failure();
 uint32_t bit=1U<<usr;
 record->target_usr=usr;
 record->pending_mask|=bit;
 if(record->format_mask&bit){
  record->phase=DIVOOM_RECOVERY_HARD_FAULT;
  if(!save_record(record))return -1;
  failed(DIVOOM_RECOVERY_FAILURE_VOLUME,usr,FF_CHECK_STRUCTURAL,EIO,"format_budget");
  return -2;
 }
 if(!allow_format(record,usr))return -1;
 if(!usr)record->plan_count=0;
 record->phase=DIVOOM_RECOVERY_FORMAT;
 if(!save_record(record))return -1;
 if(!usr&&!userdata_action(0))return -1;
 return usr?3:2;
}

static int protect_wifi(int target_usr, int require_copy)
{
 divoom_recovery_integrity_clear_failure();
 int peer=!target_usr;
 errno=0;
 int peer_state=divoom_product_test_wifi_fw_has_copy(peer);
 int peer_error=errno;
 if(peer_state>0)return 1;
 errno=0;
 int target_state=divoom_product_test_wifi_fw_has_copy(target_usr);
 int target_error=errno;
 if(target_state<=0){
  if(!require_copy && target_state==0 && peer_state==0)return 1;
  failed(DIVOOM_RECOVERY_FAILURE_FIRMWARE,target_state<0?target_usr:peer,
      target_state<0?target_state:peer_state,target_state<0?target_error:peer_error,
      target_state<0?"wifi_fw.source":"wifi_fw.backup");
  return 0;
 }
 const divoom_recovery_record *record=divoom_recovery_journal_get();
 if(record->plan_count && record->target_usr!=(unsigned)target_usr){
  failed(DIVOOM_RECOVERY_FAILURE_FIRMWARE,peer,FF_CHECK_BUSY,EBUSY,"wifi_fw.backup_plan");return 0;
 }
 int blocked=!!(record->pending_mask&(1U<<peer));
 ff_check_result result={0};
 errno=0;
 int rc=divoom_port_fs_check_volume(device(peer),mountpoint(peer),&result,NULL,0,NULL,NULL);
 int error=errno;
 if(rc!=FF_CHECK_CLEAN || !result.complete){
  failed(DIVOOM_RECOVERY_FAILURE_FIRMWARE,peer,rc,error,"wifi_fw.backup_check");
  divoom_print("[wifi_fw] backup_peer_unavailable volume=%s result=%d complete=%u\n",mountpoint(peer),rc,result.complete);
  return 0;
 }
 if(write_block(peer,0)){
  if(failure.kind==DIVOOM_RECOVERY_FAILURE_VOLUME)failure.kind=DIVOOM_RECOVERY_FAILURE_FIRMWARE;
  failure.stage="wifi_fw.backup_unblock";return 0;
 }
 errno=0;
 int ok=divoom_product_test_wifi_fw_safe_to_erase(target_usr);
 if(!ok)failed(DIVOOM_RECOVERY_FAILURE_FIRMWARE,peer,0,errno,"wifi_fw.backup_copy");
 if(blocked && write_block(peer,1)){
  if(failure.kind==DIVOOM_RECOVERY_FAILURE_VOLUME){failure.kind=DIVOOM_RECOVERY_FAILURE_FIRMWARE;failure.stage="wifi_fw.backup_block";}
  return 0;
 }
 return ok;
}

int divoom_recovery_integrity_protect_wifi(int target_usr)
{
 divoom_recovery_integrity_clear_failure();
 int ok=protect_wifi(target_usr,1);
 divoom_product_test_repair_io_error_target(1);
 return ok;
}

static int finish_userdata(divoom_recovery_record *record)
{
 if(record->pending_mask&1U){
  if(record->format_mask&1U){
   errno=0;
   if(!divoom_product_test_wifi_fw_safe_to_erase(1)){failed(DIVOOM_RECOVERY_FAILURE_FIRMWARE,0,0,errno,"wifi_fw.userdata_copy");return 0;}
  }
  if(!userdata_action(1))return 0;
 }
 /* A discarded snapshot may have changed the persisted flags. */
 memcpy(record,divoom_recovery_journal_get(),sizeof(*record));
 record->wifi_size=0;
 memset(record->wifi,0,sizeof(record->wifi));
 divoom_recovery_record_clear_system_config(record);
 record->pending_mask&=~1U;
 return save_record(record);
}

/* /usr format and restore need writable staging on /userdata. Keep the
 * /usr intent durable while repairing the prerequisite volume first. */
static int prepare_userdata_for_usr_format(divoom_recovery_record *record,
    ff_check_result *result,ff_check_entry *entries)
{
 int rc=(record->flags&(0x100U|MANUAL_USERDATA_FORMAT))?FF_CHECK_STRUCTURAL:
    scan_and_reclaim(0,record,result,entries,"prepare_userdata");
 if(rc==FF_CHECK_STRUCTURAL||rc==FF_CHECK_IO){
  record->flags|=0x200U;
  record->target_usr=0;
  record->pending_mask|=1U;
  record->phase=DIVOOM_RECOVERY_SCAN;
  record->plan_count=0;
  divoom_recovery_integrity_failure cause=failure;
  divoom_recovery_integrity_clear_failure();
  if(!save_record(record))return -1;
  if(cause.kind!=DIVOOM_RECOVERY_FAILURE_NONE)failure=cause;
  else failed(DIVOOM_RECOVERY_FAILURE_VOLUME,0,rc,EIO,"prepare_userdata");
  return -1;
 }
 if(rc!=FF_CHECK_CLEAN)return -1;
 divoom_boot_health_set_mount_failed(0,0);
 if(write_block(0,0))return -1;
 if((record->pending_mask&1U)&&!finish_userdata(record))return -1;
 return 3;
}

int divoom_recovery_integrity_run(int manual_usr_format)
{
 divoom_recovery_integrity_clear_failure();
 divoom_recovery_record *record=recovery_alloc(sizeof(*record));
 ff_check_result result;
 ff_check_entry *entries=recovery_alloc(sizeof(*entries)*DIVOOM_RECOVERY_PLAN_MAX);
 if(!record||!entries){free(record);free(entries);return -1;}
 memcpy(record,divoom_recovery_journal_get(),sizeof(*record));
 if(!record->pending_mask){memset(record,0,sizeof(*record));record->pending_mask=3;record->phase=DIVOOM_RECOVERY_SCAN;}
 if(manual_usr_format==1 && !record->format_mask)record->flags|=MANUAL_USR_FORMAT;
 if(manual_usr_format==2 && !record->format_mask)record->flags|=MANUAL_USERDATA_FORMAT;
 if(record->phase==DIVOOM_RECOVERY_HARD_FAULT){
  int usr=record->target_usr;
  if(write_block(usr,1)){free(entries);free(record);return -1;}
  int rc=scan_and_reclaim(usr,record,&result,entries,"post_format_retry");
  /* A successful mkfs followed by a failed check may become readable again.
   * The format budget stays consumed, even when restore can resume. */
  if(!(record->flags&POST_FORMAT_VERIFY(usr))||rc!=FF_CHECK_CLEAN){
   free(entries);free(record);return -2;
  }
  record->phase=DIVOOM_RECOVERY_RESTORE;
 }
 if(!save_record(record)){free(entries);free(record);return -1;}
 if(write_block(0,1)||write_block(1,1)){free(entries);free(record);return -1;}
 int out=1, userdata_ready=0;
 int active=((record->plan_count && record->phase>=DIVOOM_RECOVERY_SCAN && record->phase<=DIVOOM_RECOVERY_VERIFY) ||
     record->phase==DIVOOM_RECOVERY_FORMAT || record->phase==DIVOOM_RECOVERY_ORPHANS)?(int)record->target_usr:-1;
 for(int index=0;index<2;index++){
  int usr=active<0?index:index?1-active:active;
  uint32_t bit=1U<<usr;
  /* A durable format intent or lost destructive plan cannot be cancelled
   * merely because deleting some owners made remaining chains look clean. */
  if((record->flags&(0x100U<<usr)) ||
     (record->phase==DIVOOM_RECOVERY_FORMAT && record->target_usr==(unsigned)usr)){
   out=require_format(record,usr);goto done;
  }
  /* Never discard an interrupted detach plan in favor of a new scan. */
  if(record->pending_mask&bit && record->target_usr==(unsigned)usr && (record->phase==DIVOOM_RECOVERY_SCAN || record->phase==DIVOOM_RECOVERY_DETACH) && record->plan_count){
   if(!usr&&record->phase==DIVOOM_RECOVERY_SCAN){
    if(!userdata_action(0)){free(entries);free(record);return -1;}
    memcpy(record,divoom_recovery_journal_get(),sizeof(*record));
   }
   if(!protect_wifi(usr,0)){free(entries);free(record);return -1;}
   int replay=delete_batch(usr,record);
   if(replay==FF_CHECK_NOMEM||replay==FF_CHECK_BUSY||replay==FF_CHECK_UNSUPPORTED){free(entries);free(record);return -1;}
   if(replay!=FF_CHECK_CLEAN){out=require_format(record,usr);goto done;}
  }
  int rc=scan_and_reclaim(usr,record,&result,entries,"recovery");
  if(rc==FF_CHECK_NOMEM||rc==FF_CHECK_BUSY||rc==FF_CHECK_UNSUPPORTED){free(entries);free(record);return -1;}
  if(rc==FF_CHECK_IO){record->reserved=0x80000000U;out=require_format(record,usr);goto done;}
  if(rc==FF_CHECK_STRUCTURAL){
   record->reserved=result.damage_flags;
   if((record->flags&DIVOOM_RECOVERY_SYSTEM_CONFIG) ||
      record->phase==DIVOOM_RECOVERY_FREE || !can_delete(&result)){
    out=require_format(record,usr);goto done;
   }
   divoom_product_test_repair_plan plan={0};
   plan.structure_files[usr]=(int)result.affected_files;
   if(!divoom_product_test_format_allow_repair(&plan)){failed(DIVOOM_RECOVERY_FAILURE_NONE,-1,0,ECANCELED,"approval.cancelled");out=-1;goto done;}
   record->geometry=result.geometry;record->plan_count=result.affected_files;
   memcpy(record->plan,entries,record->plan_count*sizeof(*entries));
   record->target_usr=usr;record->pending_mask|=bit;record->phase=DIVOOM_RECOVERY_SCAN;
   if(!save_record(record)){free(entries);free(record);return -1;}
   if(!usr){
    if(!userdata_action(0)){free(entries);free(record);return -1;}
    memcpy(record,divoom_recovery_journal_get(),sizeof(*record));
   }else{
    errno=0;
    if(!divoom_product_test_repair_backup_ledger()){
     int error=errno,target=divoom_product_test_repair_io_error_target(1);
     failed(target>=0?DIVOOM_RECOVERY_FAILURE_VOLUME:DIVOOM_RECOVERY_FAILURE_STATE,target,0,error,"userdata_ledger.backup");
     free(entries);free(record);return -1;
    }
   }
   if(!protect_wifi(usr,0)){free(entries);free(record);return -1;}
   rc=delete_batch(usr,record);
   if(rc==FF_CHECK_NOMEM||rc==FF_CHECK_BUSY||rc==FF_CHECK_UNSUPPORTED){free(entries);free(record);return -1;}
   if(rc!=FF_CHECK_CLEAN){out=require_format(record,usr);goto done;}
   record->phase=DIVOOM_RECOVERY_VERIFY;
   if(!save_record(record)){free(entries);free(record);return -1;}
   rc=scan_and_reclaim(usr,record,&result,entries,"post_delete");
   if(rc!=FF_CHECK_CLEAN){
    if(rc==FF_CHECK_NOMEM||rc==FF_CHECK_BUSY||rc==FF_CHECK_UNSUPPORTED){free(entries);free(record);return -1;}
    out=require_format(record,usr);goto done;
   }
  }
  if((usr&&(record->flags&MANUAL_USR_FORMAT)) || (!usr&&(record->flags&MANUAL_USERDATA_FORMAT))){
   out=require_format(record,usr);goto done;
  }
  record->plan_count=0;record->phase=DIVOOM_RECOVERY_RESTORE;
  record->flags&=~POST_FORMAT_VERIFY(usr);
  if(!save_record(record)){free(entries);free(record);return -1;}
  if(write_block(usr,0)){free(entries);free(record);return -1;}
  divoom_boot_health_set_mount_failed(usr,0);
  if(!usr && !userdata_error_clear()){free(entries);free(record);return -1;}
  if(!usr){
   /* Retain the last pending bit until configuration restore and completion. */
   if(record->pending_mask==1U && !(record->flags&(MANUAL_USR_FORMAT|0x200U))){out=4;goto done;}
   if(!finish_userdata(record)){free(entries);free(record);return -1;}
   userdata_ready=1;
  }
 }
done:
 if(out==3&&!userdata_ready)out=prepare_userdata_for_usr_format(record,&result,entries);
 free(entries);
 free(record);
 return out;
}
int divoom_recovery_integrity_request_format(int usr)
{
 divoom_recovery_integrity_clear_failure();
 divoom_recovery_record *r=recovery_alloc(sizeof(*r));if(!r)return -1;
 memcpy(r,divoom_recovery_journal_get(),sizeof(*r));
 r->reserved=0x40000000U;
 int rc=require_format(r,usr);free(r);return rc;
}
int divoom_recovery_integrity_before_format(int usr)
{
 divoom_recovery_integrity_clear_failure();
 divoom_recovery_record *r=recovery_alloc(sizeof(*r));if(!r)return 0;
 memcpy(r,divoom_recovery_journal_get(),sizeof(*r));
 if(r->format_mask&(1U<<usr)){r->phase=DIVOOM_RECOVERY_HARD_FAULT;r->target_usr=usr;
  int saved=save_record(r);
  if(saved)failed(DIVOOM_RECOVERY_FAILURE_VOLUME,usr,FF_CHECK_STRUCTURAL,EIO,"format_budget");
  free(r);return 0;}
 if(!allow_format(r,usr)){free(r);return 0;}
 r->target_usr=usr;r->pending_mask|=1U<<usr;r->phase=DIVOOM_RECOVERY_FORMAT;
 int ok=save_record(r);free(r);return ok;
}
int divoom_recovery_integrity_after_format(int usr)
{
 divoom_recovery_integrity_clear_failure();
 divoom_recovery_record *r=recovery_alloc(sizeof(*r));if(!r)return 0;
 memcpy(r,divoom_recovery_journal_get(),sizeof(*r));
 r->target_usr=usr;r->format_mask|=1U<<usr;
 r->flags&=~((usr?MANUAL_USR_FORMAT:MANUAL_USERDATA_FORMAT)|(0x100U<<usr));
 r->flags|=POST_FORMAT_VERIFY(usr);
 r->phase=DIVOOM_RECOVERY_RESTORE;r->plan_count=0;
 if(!save_record(r)){free(r);return 0;}
 int ok=1;
 ff_check_result result;
 int rc=scan_and_reclaim(usr,r,&result,NULL,"post_format");
 if(rc!=FF_CHECK_CLEAN){
  if(rc!=FF_CHECK_NOMEM&&rc!=FF_CHECK_BUSY&&rc!=FF_CHECK_UNSUPPORTED){
   r->phase=DIVOOM_RECOVERY_HARD_FAULT;save_record(r);
  }
  ok=0;
 }
 if(ok){
  r->flags&=~POST_FORMAT_VERIFY(usr);
  ok=save_record(r);
 }
 if(ok){
  ok=write_block(usr,0)==0;
  if(ok)divoom_boot_health_set_mount_failed(usr,0);
  if(ok && !usr)ok=userdata_error_clear();
 }
 free(r);return ok;
}
int divoom_recovery_integrity_format_failed(int usr)
{
 divoom_recovery_integrity_clear_failure();
 divoom_recovery_record *r=recovery_alloc(sizeof(*r));if(!r)return 0;
 memcpy(r,divoom_recovery_journal_get(),sizeof(*r));
 r->target_usr=usr;r->format_mask|=1U<<usr;r->phase=DIVOOM_RECOVERY_HARD_FAULT;
 r->flags&=~POST_FORMAT_VERIFY(usr);
 r->pending_mask|=1U<<usr;
 int ok=save_record(r);free(r);return ok;
}
int divoom_recovery_integrity_complete(int usr)
{
 divoom_recovery_integrity_clear_failure();
 divoom_recovery_record *r=recovery_alloc(sizeof(*r));if(!r)return 0;
 memcpy(r,divoom_recovery_journal_get(),sizeof(*r));r->pending_mask&=~(1U<<usr);r->plan_count=0;
 int ok;
 if(r->pending_mask){r->phase=DIVOOM_RECOVERY_SCAN;ok=save_record(r);}
 else ok=clear_record();
 free(r);return ok;
}
