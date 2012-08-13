

#ifndef _UTIL_MD5_H
#define _UTIL_MD5_H

// int = docsis_add_cm_mic()
// Adds the CM Message Integrity Check
// ****************************************************
// tlvbuf -- pointer to buffer binary tlv data
// tlvlen -- current length of tlv data
// maxtlv -- maximum size of tlv buffer
// returns the size of the data in the tlv buffer.
int  docsis_add_cm_mic( u_int8_t *tlvbuf, u_int32_t tlvlen, u_int32_t maxtlv );

// int = docsis_add_cmts_mic()
// Adds the CMTS Message Integrity Check
// tlvbuf -- pointer to buffer binary tlv data
// tlvlen -- current length of tlv data
// maxtlv -- maximum size of tlv buffer
// key    -- pointer to key string
// keylen -- length of key
// returns the size of the data in the tlv buffer.
int  docsis_add_cmts_mic( u_int8_t *tlvbuf, u_int32_t tlvlen, u_int32_t maxtlv,
			  u_int8_t *key,    u_int32_t keylen );

// int = docsis_add_eod_and_pad()
// Finish up the file.
// tlvbuf -- pointer to buffer binary tlv data
// tlvlen -- current length of tlv data
// maxtlv -- maximum size of tlv buffer
// returns the size of the data in the tlv buffer.
int  docsis_add_eod_and_pad( u_int8_t *tlvbuf, u_int32_t tlvlen, u_int32_t maxtlv );

#endif /* _UTIL_MD5_H */

