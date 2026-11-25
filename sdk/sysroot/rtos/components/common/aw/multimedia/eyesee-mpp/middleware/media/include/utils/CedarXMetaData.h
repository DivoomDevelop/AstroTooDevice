/*
********************************************************************************
*                           Android multimedia module
*
*          (c) Copyright 2010-2015, Allwinner Microelectronic Co., Ltd.
*                              All Rights Reserved
*
* File   : CedarXMetaData.h
* Version: V1.0
* By     : eric_wang
* Date   : 2015-9-14
* Description:
    extend meta data key.
********************************************************************************
*/
#ifndef _CEDARXMETADATA_H_
#define _CEDARXMETADATA_H_

#include <stdint.h>

#ifdef __cplusplus
#include <MetaData.h>

namespace EyeseeLinux {
#endif

/* CedarX MetaData keys - use uint32_t for C compatibility */
enum CedarXMetaDataKey {
    kCedarXKeyFrameCount  = 0x66636e74,  /* 'fcnt' - int32_t, gpu frame count */
    kCedarXKeyColorSpace  = 0x436f6c53,  /* 'ColS' - color space */
    kCedarXKeyVideoLayer  = 0x564c6179,  /* 'VLay' - video layer id. */
    kCedarXKeyDisplayTopX = 0x746f7058,  /* 'topX' - int32_t, displayTopX */
    kCedarXKeyDisplayTopY = 0x746f7059,  /* 'topY' - int32_t, displayTopY */
};

#ifdef __cplusplus
}  // namespace EyeseeLinux
#endif

/* MetaData base keys - for C only, C++ uses MetaData.h */
#ifndef __cplusplus
enum MetaDataKey {
    kKeyMIMEType          = 0x6d696d65,  /* 'mime' - cstring */
    kKeyWidth             = 0x77696474,  /* 'widt' - int32_t, image pixel */
    kKeyHeight            = 0x68656967,  /* 'heig' - int32_t, image pixel */
    kKeyDisplayWidth      = 0x64576964,  /* 'dWid' - int32_t, display/presentation */
    kKeyDisplayHeight     = 0x64486774,  /* 'dHgt' - int32_t, display/presentation */
    kKeyCropRect          = 0x63726f70,  /* 'crop' */
    kKeyRotation          = 0x726f7441,  /* 'rotA' - int32_t (angle in degrees) */
    kKeyIFramesInterval   = 0x69666976,  /* 'ifiv' - int32_t */
    kKeyStride            = 0x73747264,  /* 'strd' - int32_t */
    kKeySliceHeight       = 0x736c6874,  /* 'slht' - int32_t */
    kKeyChannelCount      = 0x2363686e,  /* '#chn' - int32_t */
    kKeyChannelMask       = 0x63686e6d,  /* 'chnm' - int32_t */
    kKeySampleRate        = 0x73727465,  /* 'srte' - int32_t (audio sampling rate Hz) */
    kKeyFrameRate         = 0x66726d52,  /* 'frmR' - int32_t (video frame rate fps) */
    kKeyBitRate           = 0x62727465,  /* 'brte' - int32_t (bps) */
    kKeyESDS              = 0x65736473,  /* 'esds' - raw data */
    kKeyAACProfile        = 0x61616370,  /* 'aacp' - int32_t */
    kKeyAVCC              = 0x61766363,  /* 'avcc' - raw data */
    kKeyD263              = 0x64323633,  /* 'd263' - raw data */
    kKeyVorbisInfo        = 0x76696e66,  /* 'vinf' - raw data */
    kKeyVorbisBooks       = 0x76626f6f,  /* 'vboo' - raw data */
    kKeyWantsNALFragments = 0x4e414c66,  /* 'NALf' */
    kKeyIsSyncFrame       = 0x73796e63,  /* 'sync' - int32_t (bool) */
    kKeyIsCodecConfig     = 0x636f6e66,  /* 'conf' - int32_t (bool) */
    kKeyTime              = 0x74696d65,  /* 'time' - int64_t (usecs) */
    kKeyDecodingTime      = 0x64656354,  /* 'decT' - int64_t */
    kKeyNTPTime           = 0x6e747054,  /* 'ntpT' - uint64_t */
    kKeyTargetTime        = 0x74617254,  /* 'tarT' - int64_t */
    kKeyDriftTime         = 0x64667454,  /* 'dftT' - int64_t */
    kKeyAnchorTime        = 0x616e6354,  /* 'ancT' - int64_t */
    kKeyDuration          = 0x64757261,  /* 'dura' - int64_t */
    kKeyColorFormat       = 0x636f6c66,  /* 'colf' */
    kKeyPlatformPrivate   = 0x70726976,  /* 'priv' - pointer */
    kKeyDecoderComponent  = 0x64656343,  /* 'decC' - cstring */
    kKeyBufferID          = 0x62664944,  /* 'bfID' */
    kKeyMaxInputSize      = 0x696e7053,  /* 'inpS' */
    kKeyThumbnailTime     = 0x74686254,  /* 'thbT' - int64_t */
    kKeyTrackID           = 0x74724944,  /* 'trID' */
    kKeyIsDRM             = 0x6964726d,  /* 'idrm' - int32_t (bool) */
    kKeyEncoderDelay      = 0x656e6364,  /* 'encd' - int32_t */
    kKeyEncoderPadding    = 0x656e6370,  /* 'encp' - int32_t */
    kKeyAlbum             = 0x616c6275,  /* 'albu' - cstring */
    kKeyArtist            = 0x61727469,  /* 'arti' - cstring */
    kKeyAlbumArtist       = 0x61617274,  /* 'aart' - cstring */
    kKeyComposer          = 0x636f6d70,  /* 'comp' - cstring */
    kKeyGenre             = 0x67656e72,  /* 'genr' - cstring */
    kKeyTitle             = 0x7469746c,  /* 'titl' - cstring */
    kKeyYear              = 0x79656172,  /* 'year' - cstring */
    kKeyAlbumArt          = 0x616c6241,  /* 'albA' - compressed image data */
    kKeyAlbumArtMIME      = 0x616c414d,  /* 'alAM' - cstring */
    kKeyAuthor            = 0x61757468,  /* 'auth' - cstring */
    kKeyCDTrackNumber     = 0x63647472,  /* 'cdtr' - cstring */
    kKeyDiscNumber        = 0x646e756d,  /* 'dnum' - cstring */
    kKeyDate              = 0x64617465,  /* 'date' - cstring */
    kKeyWriter            = 0x77726974,  /* 'writ' - cstring */
    kKeyCompilation       = 0x6370696c,  /* 'cpil' - cstring */
    kKeyLocation          = 0x6c6f6320,  /* 'loc ' - cstring */
    kKeyTimeScale         = 0x746d736c,  /* 'tmsl' - int32_t */
    kKeyVideoProfile      = 0x76707266,  /* 'vprf' - int32_t */
    kKeyVideoLevel        = 0x766c6576,  /* 'vlev' - int32_t */
    kKey64BitFileOffset   = 0x666f6274,  /* 'fobt' - int32_t (bool) */
    kKey2ByteNalLength    = 0x324e414c,  /* '2NAL' - int32_t (bool) */
    kKeyFileType          = 0x66747970,  /* 'ftyp' - int32_t */
    kKeyTrackTimeStatus   = 0x746b746d,  /* 'tktm' - int64_t */
    kKeyNotRealTime       = 0x6e747274,  /* 'ntrt' - bool (int32_t) */
    kKeyNumBuffers        = 0x6e626266,  /* 'nbbf' - int32_t */
    kKeyAutoLoop          = 0x6175744c,  /* 'autL' - bool (int32_t) */
    kKeyValidSamples      = 0x76616c44,  /* 'valD' - int32_t */
    kKeyIsUnreadable      = 0x756e7265,  /* 'unre' - bool (int32_t) */
    kKeyRendered          = 0x72656e64,  /* 'rend' - bool (int32_t) */
    kKeyMediaLanguage     = 0x6c616e67,  /* 'lang' - cstring */
    kKeyTextFormatData    = 0x74657874,  /* 'text' - raw data */
    kKeyRequiresSecureBuffers = 0x73656375, /* 'secu' - bool (int32_t) */
    kKeyIsADTS            = 0x61647473,  /* 'adts' - bool (int32_t) */
    kKeyEncryptedSizes    = 0x656e6372,  /* 'encr' - size_t[] */
    kKeyPlainSizes        = 0x706c6169,  /* 'plai' - size_t[] */
    kKeyCryptoKey         = 0x6372794b,  /* 'cryK' - uint8_t[16] */
    kKeyCryptoIV          = 0x63727949,  /* 'cryI' - uint8_t[16] */
    kKeyCryptoMode        = 0x6372794d,  /* 'cryM' - int32_t */
    kKeyOffset            = 0x6f666673,  /* 'offs' */
    kKey3dDoubleStream    = 0x646f7562,  /* 'doub' - int32_t */
};
#endif /* __cplusplus */

#endif  /* _CEDARXMETADATA_H_ */

