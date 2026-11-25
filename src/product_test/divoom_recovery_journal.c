#include "divoom_recovery_journal.h"
#include "divoom_port_fs_check.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define RECOVERY_MAGIC 0x31524a44U
#define RECOVERY_SLOT 16384U
#define RECOVERY_HEADER 512U
typedef struct {
 uint32_t magic, version, sequence, size, payload_crc, header_crc;
 uint32_t pending_mask,target_usr,phase,format_mask,flags;
 unsigned char reserved[RECOVERY_HEADER-44];
} recovery_header;
typedef char record_must_fit[(sizeof(divoom_recovery_record)<=RECOVERY_SLOT-RECOVERY_HEADER)?1:-1];
typedef char record_layout_unchanged[(offsetof(divoom_recovery_record, wifi)==11592U &&
    sizeof(divoom_recovery_record)==15688U)?1:-1];
static divoom_recovery_record current;
static uint32_t sequence;
static int initialized, active_slot=-1, unavailable;
static uint32_t checksum(const void *data, size_t size)
{
 const unsigned char *p=data; uint32_t c=~0U;
 while(size--){c^=*p++;for(unsigned i=0;i<8;i++)c=(c>>1)^((c&1)?0xedb88320U:0);}
 return ~c;
}
static int valid_header(recovery_header *h)
{
 uint32_t crc=h->header_crc;h->header_crc=0;
 int ok=h->magic==RECOVERY_MAGIC&&(h->version==1||h->version==2)&&h->size==sizeof(current)&&checksum(h,sizeof(*h))==crc;
 h->header_crc=crc;return ok;
}
static int valid_record(const divoom_recovery_record *r)
{
 return !(r->pending_mask&~3U)&&r->target_usr<=1&&r->plan_count<=DIVOOM_RECOVERY_PLAN_MAX&&
  r->wifi_size<DIVOOM_RECOVERY_WIFI_MAX&&(!r->wifi_size||r->wifi[r->wifi_size]==0)&&
  (!(r->flags&DIVOOM_RECOVERY_SYSTEM_CONFIG)||(!r->plan_count&&(r->pending_mask&1U)&&!r->target_usr))&&
  (!r->pending_mask||(r->phase>=DIVOOM_RECOVERY_SCAN&&r->phase<=DIVOOM_RECOVERY_ORPHANS));
}
int divoom_recovery_journal_boot(void)
{
 if(initialized)return unavailable?-1:current.pending_mask!=0;
 initialized=1;memset(&current,0,sizeof(current));
 unsigned char *slot=malloc(RECOVERY_SLOT);
 if(!slot){unavailable=1;divoom_port_fs_write_block("/dev/UDISK",1);divoom_port_fs_write_block("/dev/usr",1);return -1;}
 int read_error=0,corrupt=0,have_summary=0;
 recovery_header summary;memset(&summary,0,sizeof(summary));
 for(int i=0;i<2;i++){
  if(divoom_recovery_storage_io(0,i*RECOVERY_SLOT,slot,RECOVERY_SLOT)){read_error=1;continue;}
  recovery_header *h=(recovery_header*)slot;
  if(!valid_header(h)){
   if(h->magic==RECOVERY_MAGIC)corrupt=1;continue;
  }
  if(h->version==2 && !(h->pending_mask&~3U) && h->target_usr<=1 &&
    h->phase>=DIVOOM_RECOVERY_SCAN && h->phase<=DIVOOM_RECOVERY_ORPHANS &&
    (!have_summary||(int32_t)(h->sequence-summary.sequence)>0)){
   summary=*h;have_summary=1;
  }
  if(checksum(slot+RECOVERY_HEADER,h->size)!=h->payload_crc||
    !valid_record((const divoom_recovery_record*)(slot+RECOVERY_HEADER))){corrupt=1;continue;}
  if(active_slot<0||(int32_t)(h->sequence-sequence)>0){
   memcpy(&current,slot+RECOVERY_HEADER,sizeof(current));sequence=h->sequence;active_slot=i;
  }
 }
 free(slot);
 unavailable=read_error?1:0;
 /* A protected newer intent survives a damaged payload. Once detach/free may
  * have begun, a fresh clean scan cannot reconstruct every affected owner. */
 if(!read_error && have_summary && (active_slot<0||(int32_t)(summary.sequence-sequence)>0)){
  memset(&current,0,sizeof(current));current.pending_mask=summary.pending_mask;
  current.target_usr=summary.target_usr;current.phase=summary.phase;current.format_mask=summary.format_mask;
  current.flags=(summary.flags&~DIVOOM_RECOVERY_SYSTEM_CONFIG)|1U;sequence=summary.sequence;active_slot=-1;
  if(current.pending_mask && ((current.phase>=DIVOOM_RECOVERY_DETACH && current.phase<=DIVOOM_RECOVERY_FORMAT) ||
      current.phase==DIVOOM_RECOVERY_ORPHANS)){
   current.flags|=0x100U<<current.target_usr;current.phase=DIVOOM_RECOVERY_SCAN;
  }
 }
 if(!read_error && corrupt && !have_summary && active_slot<0){
  /* Recognized but untrustworthy intent is not an unused all-zero/FF slot. */
  current.pending_mask=3;current.phase=DIVOOM_RECOVERY_SCAN;current.flags|=0x301U;
  current.plan_count=current.wifi_size=0;
 }
 if(unavailable){divoom_port_fs_write_block("/dev/UDISK",1);divoom_port_fs_write_block("/dev/usr",1);}
 if(current.pending_mask&1U)divoom_port_fs_write_block("/dev/UDISK",1);
 if(current.pending_mask&2U)divoom_port_fs_write_block("/dev/usr",1);
 return unavailable?-1:current.pending_mask!=0;
}
int divoom_recovery_journal_pending(void)
{
 if(!initialized)divoom_recovery_journal_boot();
 return unavailable?-1:current.pending_mask!=0;
}
int divoom_recovery_journal_retry_read(void)
{
 if(!unavailable)return divoom_recovery_journal_pending();
 initialized=0;active_slot=-1;sequence=0;
 return divoom_recovery_journal_boot();
}
const divoom_recovery_record *divoom_recovery_journal_get(void)
{
 if(!initialized)divoom_recovery_journal_boot();return &current;
}
int divoom_recovery_journal_save(const divoom_recovery_record *record)
{
 if(!initialized)divoom_recovery_journal_boot();
 if(unavailable==1){
  /* Never overwrite an unread old plan just because the raw read failed.
   * Re-read first; the caller must rebuild its decision from recovered state. */
  divoom_recovery_journal_retry_read();return 0;
 }
 if(!valid_record(record))return 0;
 unsigned char *slot=calloc(1,RECOVERY_SLOT);
 unsigned char *verify=malloc(RECOVERY_SLOT);
 if(!slot||!verify){free(slot);free(verify);return 0;}
 recovery_header *h=(recovery_header*)slot;
 h->magic=RECOVERY_MAGIC;h->version=2;h->sequence=sequence+1;h->size=sizeof(current);
 h->pending_mask=record->pending_mask;h->target_usr=record->target_usr;h->phase=record->phase;
 h->format_mask=record->format_mask;h->flags=record->flags;
 memcpy(slot+RECOVERY_HEADER,record,sizeof(current));
 h->payload_crc=checksum(record,sizeof(current));h->header_crc=checksum(h,sizeof(*h));
 int next=active_slot==0?1:0;uint32_t base=next*RECOVERY_SLOT;
 /* Write the payload first. The old slot remains the authoritative intent
  * until the header and complete new record have both passed read-back. */
 int ok=!divoom_recovery_storage_io(1,base+RECOVERY_HEADER,slot+RECOVERY_HEADER,RECOVERY_SLOT-RECOVERY_HEADER)&&
  !divoom_recovery_storage_io(1,base,slot,RECOVERY_HEADER)&&
  !divoom_recovery_storage_io(0,base,verify,RECOVERY_SLOT)&&!memcmp(slot,verify,RECOVERY_SLOT);
 if(ok){memcpy(&current,record,sizeof(current));sequence=h->sequence;active_slot=next;unavailable=0;}
 else{unavailable=1;divoom_port_fs_write_block("/dev/UDISK",1);divoom_port_fs_write_block("/dev/usr",1);}
 free(slot);free(verify);return ok;
}
int divoom_recovery_journal_clear(void)
{
 divoom_recovery_record *done=calloc(1,sizeof(*done));if(!done)return 0;
 done->phase=DIVOOM_RECOVERY_COMPLETE;done->format_mask=current.format_mask;
 int ok=divoom_recovery_journal_save(done);free(done);return ok;
}
