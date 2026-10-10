/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicitly initialized stopped/main-loop policy, never a migration authority. */
#include "native_autosave_session.h"
static struct {
    uint32_t saved,seen,idle,poll,last,verify,writes,sequence;
    uint8_t ready,enabled,attempted,err,pending,record;
} native_as __attribute__((section(".pool")));

void d8p1_autosave_session_end(void) { native_as.ready=0; }
void d8p1_autosave_session_hold(void) { native_as.idle=fm1_ms; }
/* A record read verifies exactly what persisted, never adopts music. Sequence
 * equality distinguishes an unchanged prior record from a committed replacement,
 * including UINT32 rollover. Failed reads keep uncertainty and prohibit retries. */
static int nas_reconcile(void)
{
    native_as.verify=fm1_ms;
    if(cv_cpu_active||!df_automatic_quiet()||transport_req)return D8POOL_BUSY;
    uint32_t sig=0;d8pool_record record;
    int rc=d8p1_autosave_snapshot_flash(&sig,&record,1);
    if(rc!=D8POOL_OK&&rc!=D8POOL_EMPTY)return rc;
    int committed=!rc&&(!native_as.record||record.sequence!=native_as.sequence);
    if(!rc) {
        native_as.saved=sig;native_as.sequence=record.sequence;native_as.record=1;
    } else native_as.record=0;
    native_as.err=!committed;
    if(committed){native_as.writes++;native_as.last=fm1_ms;}
    native_as.pending=0;
    return D8POOL_OK;
}
int d8p1_autosave_session_begin(int authorized,int clean_boot)
{
    if(!authorized||project_native_status()!=1)return D8POOL_UNSUPPORTED;
    if(native_as.ready)return D8POOL_BUSY;
    if(!flash_ok)return D8POOL_IO;
    int rc=D8POOL_OK;
    if(clean_boot&&!(ui_prefs&PREF_RESTORE_OFF)) {
        rc=d8p1_restore_flash_autosave(1,1);
        if(rc!=D8POOL_OK&&rc!=D8POOL_EMPTY)return rc;
    }
    uint32_t current=0,stored=0;d8pool_record record;
    rc=d8p1_signature_runtime(&current);
    if(rc)return d8pr_runtime_status(rc);
    rc=d8p1_autosave_snapshot_flash(&stored,&record,1);
    if(rc!=D8POOL_OK&&rc!=D8POOL_EMPTY)return rc;
    memset(&native_as,0,sizeof native_as);
    native_as.saved=native_as.seen=current;
    native_as.idle=native_as.poll=fm1_ms;
    native_as.enabled=!(ui_prefs&PREF_RESTORE_OFF);
    if(!rc){native_as.record=1;native_as.sequence=record.sequence;}
    native_as.ready=1;return D8POOL_OK;
}
int d8p1_autosave_session_poll(void)
{
    uint32_t now=fm1_ms;
    if(!native_as.ready||project_native_status()!=1)return D8POOL_UNSUPPORTED;
    if(now-native_as.poll<AS_POLL_MS)return D8POOL_OK;
    native_as.poll=now;
    unsigned enabled=!(ui_prefs&PREF_RESTORE_OFF);
    if(enabled!=native_as.enabled){native_as.enabled=(uint8_t)enabled;native_as.idle=now;}
    if(!enabled){native_as.idle=now;return D8POOL_OK;}
    if(!flash_ok){native_as.idle=now;return D8POOL_IO;}
    if(cv_cpu_active||!df_automatic_quiet()||transport_req){native_as.idle=now;return D8POOL_BUSY;}
    /* Resolve uncertain physical results before even considering another write.
     * Bounded polling avoids repeated full scans after a persistent I/O error. */
    if(native_as.pending) {
        if(now-native_as.verify<AS_RETRY_MS)return D8POOL_OK;
        int rc=nas_reconcile();if(rc)return rc;
    }
    uint32_t sig=0;int rc=d8p1_signature_runtime(&sig);
    if(rc){native_as.idle=now;return d8pr_runtime_status(rc);}
    if(sig!=native_as.seen){native_as.seen=sig;native_as.idle=now;return D8POOL_OK;}
    if(sig==native_as.saved||now-native_as.idle<AS_IDLE_MS||
       (native_as.attempted&&now-native_as.last<(native_as.err?AS_RETRY_MS:AS_GAP_MS)))return D8POOL_OK;
    /* Refresh the current record immediately before the attempt. A different
     * approved writer may already have saved this music; never overwrite blind. */
    uint32_t stored=0;d8pool_record record;
    rc=d8p1_autosave_snapshot_flash(&stored,&record,1);
    if(rc!=D8POOL_OK&&rc!=D8POOL_EMPTY){native_as.idle=now;return rc;}
    native_as.record=!rc;
    if(!rc){native_as.sequence=record.sequence;if(stored==sig){native_as.saved=stored;return D8POOL_OK;}}
    native_as.last=now;native_as.attempted=1;native_as.pending=1;native_as.err=1;
    rc=d8p1_autosave_flash(1);
    /* Even OK is reconciled from the committed record, not the pre-save hash.
     * Late changes may produce a different capture or a different current RAM. */
    int checked=nas_reconcile();
    if(checked)return checked;
    return rc;
}
