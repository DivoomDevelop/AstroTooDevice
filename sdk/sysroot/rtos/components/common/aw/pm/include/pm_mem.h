#ifndef __PM_MEM_H__
#define __PM_MEM_H__

#ifdef CONFIG_PM_STANDBY_MEMORY
#define __standby_saved_data     __attribute__((__section__(".standby_saved_data")))
#define __standby_saved_bss      __attribute__((__section__(".standby_saved_bss")))
#define __standby_unsaved_data   __attribute__((__section__(".standby_unsaved_data")))
#define __standby_unsaved_bss    __attribute__((__section__(".standby_unsaved_bss")))
#else
#define __standby_saved_data
#define __standby_saved_bss
#define __standby_unsaved_data
#define __standby_unsaved_bss
#endif

#endif
