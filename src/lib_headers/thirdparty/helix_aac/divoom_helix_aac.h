/* AstroToo tone_lite Helix AAC API
 * Public AAC* symbols are renamed to avoid clash with MPP eyesee-mpp libaac.a
 * (same Helix lineage, already linked into the firmware).
 */
#ifndef DIVOOM_HELIX_AAC_H
#define DIVOOM_HELIX_AAC_H

#define AACInitDecoder       divoom_hx_AACInitDecoder
#define AACInitDecoderPre    divoom_hx_AACInitDecoderPre
#define AACFreeDecoder       divoom_hx_AACFreeDecoder
#define AACDecode            divoom_hx_AACDecode
#define AACFindSyncWord      divoom_hx_AACFindSyncWord
#define AACGetLastFrameInfo  divoom_hx_AACGetLastFrameInfo
#define AACSetRawBlockParams divoom_hx_AACSetRawBlockParams
#define AACFlushCodec        divoom_hx_AACFlushCodec
#define AllocateBuffersPre   divoom_hx_AllocateBuffersPre

#include "aacdec.h"

#endif /* DIVOOM_HELIX_AAC_H */
