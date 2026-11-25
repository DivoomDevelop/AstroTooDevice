#ifndef DIVOOM_DEVICE_ERROR_H
#define DIVOOM_DEVICE_ERROR_H
/* Coalesce pending reports by error class; preserve them across filesystem recovery. */
/* Mount transitions and mkfs own these barriers; calls drain reporter file I/O. */
void divoom_device_error_userdata_ready(int ready);
/* Call after userdata repair releases its write barrier; failure keeps recovery pending. */
int divoom_device_error_userdata_repair_complete(void);
void divoom_device_error_userdata_format_begin(void);
void divoom_device_error_userdata_format_end(void);
void divoom_device_error_record(int error_id);
/* SET_ALL_MODE queues all recorded errors once, without waiting for ACKs. */
void divoom_device_error_report_all(void);
#endif
