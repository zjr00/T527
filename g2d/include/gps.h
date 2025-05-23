/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_INCLUDE_HARDWARE_GPS_H
#define ANDROID_INCLUDE_HARDWARE_GPS_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <pthread.h>

//#include <hardware/hardware.h>
#include <hardware.h>

__BEGIN_DECLS

/**
 * The id of this module
 */
#define GPS_HARDWARE_MODULE_ID "gps"


/** Milliseconds since January 1, 1970 */
typedef int64_t GpsUtcTime;

/** Maximum number of SVs for gps_sv_status_callback(). */
//#define GPS_MAX_SVS 32
/***gps_sv = 32 bds_sv = 32 sum = 64*******/
#define GPS_MAX_SVS 64

/** Requested operational mode for GPS operation. */
typedef uint32_t GpsPositionMode;
// IMPORTANT: Note that the following values must match
// constants in GpsLocationProvider.java.
/** Mode for running GPS standalone (no assistance). */
#define GPS_POSITION_MODE_STANDALONE    0
/** AGPS MS-Based mode. */
#define GPS_POSITION_MODE_MS_BASED      1
/** AGPS MS-Assisted mode. */
#define GPS_POSITION_MODE_MS_ASSISTED   2

/** Requested recurrence mode for GPS operation. */
typedef uint32_t GpsPositionRecurrence;
// IMPORTANT: Note that the following values must match
// constants in GpsLocationProvider.java.
/** Receive GPS fixes on a recurring basis at a specified period. */
#define GPS_POSITION_RECURRENCE_PERIODIC    0
/** Request a single shot GPS fix. */
#define GPS_POSITION_RECURRENCE_SINGLE      1

/** GPS status event values. */
typedef uint16_t GpsStatusValue;
// IMPORTANT: Note that the following values must match
// constants in GpsLocationProvider.java.
/** GPS status unknown. */
#define GPS_STATUS_NONE             0
/** GPS has begun navigating. */
#define GPS_STATUS_SESSION_BEGIN    1
/** GPS has stopped navigating. */
#define GPS_STATUS_SESSION_END      2
/** GPS has powered on but is not navigating. */
#define GPS_STATUS_ENGINE_ON        3
/** GPS is powered off. */
#define GPS_STATUS_ENGINE_OFF       4

/** Flags to indicate which values are valid in a GpsLocation. */
typedef uint16_t GpsLocationFlags;
// IMPORTANT: Note that the following values must match
// constants in GpsLocationProvider.java.
/** GpsLocation has valid latitude and longitude. */
#define GPS_LOCATION_HAS_LAT_LONG   0x0001
/** GpsLocation has valid altitude. */
#define GPS_LOCATION_HAS_ALTITUDE   0x0002
/** GpsLocation has valid speed. */
#define GPS_LOCATION_HAS_SPEED      0x0004
/** GpsLocation has valid bearing. */
#define GPS_LOCATION_HAS_BEARING    0x0008
/** GpsLocation has valid accuracy. */
#define GPS_LOCATION_HAS_ACCURACY   0x0010

/** Flags for the gps_set_capabilities callback. */

/** GPS HAL schedules fixes for GPS_POSITION_RECURRENCE_PERIODIC mode.
    If this is not set, then the framework will use 1000ms for min_interval
    and will start and call start() and stop() to schedule the GPS.
 */
#define GPS_CAPABILITY_SCHEDULING       0x0000001
/** GPS supports MS-Based AGPS mode */
#define GPS_CAPABILITY_MSB              0x0000002
/** GPS supports MS-Assisted AGPS mode */
#define GPS_CAPABILITY_MSA              0x0000004
/** GPS supports single-shot fixes */
#define GPS_CAPABILITY_SINGLE_SHOT      0x0000008
/** GPS supports on demand time injection */
#define GPS_CAPABILITY_ON_DEMAND_TIME   0x0000010
/** GPS supports Geofencing  */
#define GPS_CAPABILITY_GEOFENCING       0x0000020

/** Flags used to specify which aiding data to delete
    when calling delete_aiding_data(). */
typedef uint16_t GpsAidingData;
// IMPORTANT: Note that the following values must match
// constants in GpsLocationProvider.java.
#define GPS_DELETE_EPHEMERIS        0x0001
#define GPS_DELETE_ALMANAC          0x0002
#define GPS_DELETE_POSITION         0x0004
#define GPS_DELETE_TIME             0x0008
#define GPS_DELETE_IONO             0x0010
#define GPS_DELETE_UTC              0x0020
#define GPS_DELETE_HEALTH           0x0040
#define GPS_DELETE_SVDIR            0x0080
#define GPS_DELETE_SVSTEER          0x0100
#define GPS_DELETE_SADATA           0x0200
#define GPS_DELETE_RTI              0x0400
#define GPS_DELETE_CELLDB_INFO      0x8000
#define GPS_DELETE_ALL              0xFFFF

/** AGPS type */
typedef uint16_t AGpsType;
#define AGPS_TYPE_SUPL          1
#define AGPS_TYPE_C2K           2

typedef uint16_t AGpsSetIDType;
#define AGPS_SETID_TYPE_NONE    0
#define AGPS_SETID_TYPE_IMSI    1
#define AGPS_SETID_TYPE_MSISDN  2

/**
 * String length constants
 */
#define GPS_NI_SHORT_STRING_MAXLEN      256
#define GPS_NI_LONG_STRING_MAXLEN       2048

/**
 * GpsNiType constants
 */
typedef uint32_t GpsNiType;
#define GPS_NI_TYPE_VOICE              1
#define GPS_NI_TYPE_UMTS_SUPL          2
#define GPS_NI_TYPE_UMTS_CTRL_PLANE    3

/**
 * GpsNiNotifyFlags constants
 */
typedef uint32_t GpsNiNotifyFlags;
/** NI requires notification */
#define GPS_NI_NEED_NOTIFY          0x0001
/** NI requires verification */
#define GPS_NI_NEED_VERIFY          0x0002
/** NI requires privacy override, no notification/minimal trace */
#define GPS_NI_PRIVACY_OVERRIDE     0x0004

/**
 * GPS NI responses, used to define the response in
 * NI structures
 */
typedef int GpsUserResponseType;
#define GPS_NI_RESPONSE_ACCEPT         1
#define GPS_NI_RESPONSE_DENY           2
#define GPS_NI_RESPONSE_NORESP         3

/**
 * NI data encoding scheme
 */
typedef int GpsNiEncodingType;
#define GPS_ENC_NONE                   0
#define GPS_ENC_SUPL_GSM_DEFAULT       1
#define GPS_ENC_SUPL_UTF8              2
#define GPS_ENC_SUPL_UCS2              3
#define GPS_ENC_UNKNOWN                -1

/** AGPS status event values. */
typedef uint16_t AGpsStatusValue;
/** GPS requests data connection for AGPS. */
#define GPS_REQUEST_AGPS_DATA_CONN  1
/** GPS releases the AGPS data connection. */
#define GPS_RELEASE_AGPS_DATA_CONN  2
/** AGPS data connection initiated */
#define GPS_AGPS_DATA_CONNECTED     3
/** AGPS data connection completed */
#define GPS_AGPS_DATA_CONN_DONE     4
/** AGPS data connection failed */
#define GPS_AGPS_DATA_CONN_FAILED   5

#define AGPS_REF_LOCATION_TYPE_GSM_CELLID   1
#define AGPS_REF_LOCATION_TYPE_UMTS_CELLID  2
#define AGPS_REG_LOCATION_TYPE_MAC          3

/** Network types for update_network_state "type" parameter */
#define AGPS_RIL_NETWORK_TYPE_MOBILE        0
#define AGPS_RIL_NETWORK_TYPE_WIFI          1
#define AGPS_RIL_NETWORK_TYPE_MOBILE_MMS    2
#define AGPS_RIL_NETWORK_TYPE_MOBILE_SUPL   3
#define AGPS_RIL_NETWORK_TTYPE_MOBILE_DUN   4
#define AGPS_RIL_NETWORK_TTYPE_MOBILE_HIPRI 5
#define AGPS_RIL_NETWORK_TTYPE_WIMAX        6

/**
 * Name for the GPS XTRA interface.
 */
#define GPS_XTRA_INTERFACE      "gps-xtra"

/**
 * Name for the GPS DEBUG interface.
 */
#define GPS_DEBUG_INTERFACE      "gps-debug"

/**
 * Name for the AGPS interface.
 */
#define AGPS_INTERFACE      "agps"

/**
 * Name for NI interface
 */
#define GPS_NI_INTERFACE "gps-ni"

/**
 * Name for the AGPS-RIL interface.
 */
#define AGPS_RIL_INTERFACE      "agps_ril"

/**
 * Name for the GPS_Geofencing interface.
 */
#define GPS_GEOFENCING_INTERFACE   "gps_geofencing"


/** Represents a location. */
typedef struct {
    /** set to sizeof(GpsLocation) */
    size_t          size;
    /** Contains GpsLocationFlags bits. */
    uint16_t        flags;
    /** Represents latitude in degrees. */
    double          latitude;
    /** Represents longitude in degrees. */
    double          longitude;
    /** Represents altitude in meters above the WGS 84 reference
     * ellipsoid. */
    double          altitude;
    /** Represents speed in meters per second. */
    float           speed;
    /** Represents heading in degrees. */
    float           bearing;
    /** Represents expected accuracy in meters. */
    float           accuracy;
    /** Timestamp for the location fix. */
    GpsUtcTime      timestamp;
} GpsLocation;

/** Represents the status. */
typedef struct {
    /** set to sizeof(GpsStatus) */
    size_t          size;
    GpsStatusValue status;
} GpsStatus;

/** Represents SV information. */
typedef struct {
    /** set to sizeof(GpsSvInfo) */
    size_t          size;
    /** Pseudo-random number for the SV. */
    int     prn;
    /** Signal to noise ratio. */
    float   snr;
    /** Elevation of SV in degrees. */
    float   elevation;
    /** Azimuth of SV in degrees. */
    float   azimuth;
} GpsSvInfo;

/** Represents SV status. */
typedef struct {
    /** set to sizeof(GpsSvStatus) */
    size_t          size;

    /** Number of SVs currently visible. */
    int         num_svs;

    /** Contains an array of SV information. */
    GpsSvInfo   sv_list[GPS_MAX_SVS];

    /** Represents a bit mask indicating which SVs
     * have ephemeris data.
     */
    uint32_t    ephemeris_mask;

    /** Represents a bit mask indicating which SVs
     * have almanac data.
     */
    uint32_t    almanac_mask;

    /**
     * Represents a bit mask indicating which SVs
     * were used for computing the most recent position fix.
     */
    uint32_t    used_in_fix_mask;
} GpsSvStatus;

/* 2G and 3G */
/* In 3G lac is discarded */
typedef struct {
    uint16_t type;
    uint16_t mcc;
    uint16_t mnc;
    uint16_t lac;
    uint32_t cid;
} AGpsRefLocationCellID;

typedef struct {
    uint8_t mac[6];
} AGpsRefLocationMac;

/** Represents ref locations */
typedef struct {
    uint16_t type;
    union {
        AGpsRefLocationCellID   cellID;
        AGpsRefLocationMac      mac;
    } u;
} AGpsRefLocation;

/** Callback with location information.
 *  Can only be called from a thread created by create_thread_cb.
 */
typedef void (* gps_location_callback)(GpsLocation* location);

/** Callback with status information.
 *  Can only be called from a thread created by create_thread_cb.
 */
typedef void (* gps_status_callback)(GpsStatus* status);

/** Callback with SV status information.
 *  Can only be called from a thread created by create_thread_cb.
 */
typedef void (* gps_sv_status_callback)(GpsSvStatus* sv_info);

/** Callback for reporting NMEA sentences.
 *  Can only be called from a thread created by create_thread_cb.
 */
typedef void (* gps_nmea_callback)(GpsUtcTime timestamp, const char* nmea, int length);

/** Callback to inform framework of the GPS engine's capabilities.
 *  Capability parameter is a bit field of GPS_CAPABILITY_* flags.
 */
typedef void (* gps_set_capabilities)(uint32_t capabilities);

/** Callback utility for acquiring the GPS wakelock.
 *  This can be used to prevent the CPU from suspending while handling GPS events.
 */
typedef void (* gps_acquire_wakelock)();

/** Callback utility for releasing the GPS wakelock. */
typedef void (* gps_release_wakelock)();

/** Callback for requesting NTP time */
typedef void (* gps_request_utc_time)();

/** Callback for creating a thread that can call into the Java framework code.
 *  This must be used to create any threads that report events up to the framework.
 */
typedef pthread_t (* gps_create_thread)(const char* name, void (*start)(void *), void* arg);

/** GPS callback structure. */
typedef struct {
    /** set to sizeof(GpsCallbacks) */
    size_t      size;
    gps_location_callback location_cb;
    gps_status_callback status_cb;
    gps_sv_status_callback sv_status_cb;
    gps_nmea_callback nmea_cb;
    gps_set_capabilities set_capabilities_cb;
    gps_acquire_wakelock acquire_wakelock_cb;
    gps_release_wakelock release_wakelock_cb;
    gps_create_thread create_thread_cb;
    gps_request_utc_time request_utc_time_cb;
} GpsCallbacks;


/** Represents the standard GPS interface. */
typedef struct {
    /** set to sizeof(GpsInterface) */
    size_t          size;
    /**
     * Opens the interface and provides the callback routines
     * to the implemenation of this interface.
     */
    int (*init)(GpsCallbacks* callbacks);

    /** Starts navigating. */
    int (*start)(void);

    /** Stops navigating. */
    int (*stop)(void);

    /** Closes the interface. */
    void (*cleanup)(void);

    /** Injects the current time. */
    int (*inject_time)(GpsUtcTime time, int64_t timeReference,
                       int uncertainty);

    /** Injects current location from another location provider
     *  (typically cell ID).
     *  latitude and longitude are measured in degrees
     *  expected accuracy is measured in meters
     */
    int (*inject_location)(double latitude, double longitude, float accuracy);

    /**
     * Specifies that the next call to start will not use the
     * information defined in the flags. GPS_DELETE_ALL is passed for
     * a cold start.
     */
    void (*delete_aiding_data)(GpsAidingData flags);

    /**
     * min_interval represents the time between fixes in milliseconds.
     * preferred_accuracy represents the requested fix accuracy in meters.
     * preferred_time represents the requested time to first fix in milliseconds.
     */
    int (*set_position_mode)(GpsPositionMode mode, GpsPositionRecurrence recurrence,
                             uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time);

    /** Get a pointer to extension information. */
    const void* (*get_extension)(const char* name);
} GpsInterface;

/** Callback to request the client to download XTRA data.
 *  The client should download XTRA data and inject it by calling inject_xtra_data().
 *  Can only be called from a thread created by create_thread_cb.
 */
typedef void (* gps_xtra_download_request)();

/** Callback structure for the XTRA interface. */
typedef struct {
    gps_xtra_download_request download_request_cb;
    gps_create_thread create_thread_cb;
} GpsXtraCallbacks;

/** Extended interface for XTRA support. */
typedef struct {
    /** set to sizeof(GpsXtraInterface) */
    size_t          size;
    /**
     * Opens the XTRA interface and provides the callback routines
     * to the implemenation of this interface.
     */
    int (*init)(GpsXtraCallbacks* callbacks);
    /** Injects XTRA data into the GPS. */
    int (*inject_xtra_data)(char* data, int length);
} GpsXtraInterface;

/** Extended interface for DEBUG support. */
typedef struct {
    /** set to sizeof(GpsDebugInterface) */
    size_t          size;

    /**
     * This function should return any information that the native
     * implementation wishes to include in a bugreport.
     */
    size_t (*get_internal_state)(char* buffer, size_t bufferSize);
} GpsDebugInterface;

/** Represents the status of AGPS. */
typedef struct {
    /** set to sizeof(AGpsStatus) */
    size_t          size;

    AGpsType        type;
    AGpsStatusValue status;
    uint32_t        ipaddr;
} AGpsStatus;

/** Callback with AGPS status information.
 *  Can only be called from a thread created by create_thread_cb.
 */
typedef void (* agps_status_callback)(AGpsStatus* status);

/** Callback structure for the AGPS interface. */
typedef struct {
    agps_status_callback status_cb;
    gps_create_thread create_thread_cb;
} AGpsCallbacks;


/** Extended interface for AGPS support. */
typedef struct {
    /** set to sizeof(AGpsInterface) */
    size_t          size;

    /**
     * Opens the AGPS interface and provides the callback routines
     * to the implemenation of this interface.
     */
    void (*init)(AGpsCallbacks* callbacks);
    /**
     * Notifies that a data connection is available and sets
     * the name of the APN to be used for SUPL.
     */
    int (*data_conn_open)(const char* apn);
    /**
     * Notifies that the AGPS data connection has been closed.
     */
    int (*data_conn_closed)();
    /**
     * Notifies that a data connection is not available for AGPS.
     */
    int (*data_conn_failed)();
    /**
     * Sets the hostname and port for the AGPS server.
     */
    int (*set_server)(AGpsType type, const char* hostname, int port);
} AGpsInterface;


/** Represents an NI request */
typedef struct {
    /** set to sizeof(GpsNiNotification) */
    size_t          size;

    /**
     * An ID generated by HAL to associate NI notifications and UI
     * responses
     */
    int             notification_id;

    /**
     * An NI type used to distinguish different categories of NI
     * events, such as GPS_NI_TYPE_VOICE, GPS_NI_TYPE_UMTS_SUPL, ...
     */
    GpsNiType       ni_type;

    /**
     * Notification/verification options, combinations of GpsNiNotifyFlags constants
     */
    GpsNiNotifyFlags notify_flags;

    /**
     * Timeout period to wait for user response.
     * Set to 0 for no time out limit.
     */
    int             timeout;

    /**
     * Default response when time out.
     */
    GpsUserResponseType default_response;

    /**
     * Requestor ID
     */
    char            requestor_id[GPS_NI_SHORT_STRING_MAXLEN];

    /**
     * Notification message. It can also be used to store client_id in some cases
     */
    char            text[GPS_NI_LONG_STRING_MAXLEN];

    /**
     * Client name decoding scheme
     */
    GpsNiEncodingType requestor_id_encoding;

    /**
     * Client name decoding scheme
     */
    GpsNiEncodingType text_encoding;

    /**
     * A pointer to extra data. Format:
     * key_1 = value_1
     * key_2 = value_2
     */
    char           extras[GPS_NI_LONG_STRING_MAXLEN];

} GpsNiNotification;

/** Callback with NI notification.
 *  Can only be called from a thread created by create_thread_cb.
 */
typedef void (*gps_ni_notify_callback)(GpsNiNotification *notification);

/** GPS NI callback structure. */
typedef struct {
    /**
     * Sends the notification request from HAL to GPSLocationProvider.
     */
    gps_ni_notify_callback notify_cb;
    gps_create_thread create_thread_cb;
} GpsNiCallbacks;

/**
 * Extended interface for Network-initiated (NI) support.
 */
typedef struct {
    /** set to sizeof(GpsNiInterface) */
    size_t          size;

    /** Registers the callbacks for HAL to use. */
    void (*init)(GpsNiCallbacks *callbacks);

    /** Sends a response to HAL. */
    void (*respond)(int notif_id, GpsUserResponseType user_response);
} GpsNiInterface;

struct gps_device_t {
    struct hw_device_t common;

    /**
     * Set the provided lights to the provided values.
     *
     * Returns: 0 on succes, error code on failure.
     */
    const GpsInterface* (*get_gps_interface)(struct gps_device_t* dev);
};
const GpsInterface* gps_get_hardware_interface();

#define AGPS_RIL_REQUEST_SETID_IMSI     (1<<0L)
#define AGPS_RIL_REQUEST_SETID_MSISDN   (1<<1L)

#define AGPS_RIL_REQUEST_REFLOC_CELLID  (1<<0L)
#define AGPS_RIL_REQUEST_REFLOC_MAC     (1<<1L)

typedef void (*agps_ril_request_set_id)(uint32_t flags);
typedef void (*agps_ril_request_ref_loc)(uint32_t flags);

typedef struct {
    agps_ril_request_set_id request_setid;
    agps_ril_request_ref_loc request_refloc;
    gps_create_thread create_thread_cb;
} AGpsRilCallbacks;

/** Extended interface for AGPS_RIL support. */
typedef struct {
    /** set to sizeof(AGpsRilInterface) */
    size_t          size;
    /**
     * Opens the AGPS interface and provides the callback routines
     * to the implemenation of this interface.
     */
    void (*init)(AGpsRilCallbacks* callbacks);

    /**
     * Sets the reference location.
     */
    void (*set_ref_location)(const AGpsRefLocation *agps_reflocation, size_t sz_struct);
    /**
     * Sets the set ID.
     */
    void (*set_set_id)(AGpsSetIDType type, const char* setid);

    /**
     * Send network initiated message.
     */
    void (*ni_message)(uint8_t *msg, size_t len);

    /**
     * Notify GPS of network status changes.
     * These parameters match values in the android.net.NetworkInfo class.
     */
    void (*update_network_state)(int connected, int type, int roaming, const char* extra_info);

    /**
     * Notify GPS of network status changes.
     * These parameters match values in the android.net.NetworkInfo class.
     */
    void (*update_network_availability)(int avaiable, const char* apn);
} AGpsRilInterface;

/**
 * GPS Geofence.
 *      There are 3 states associated with a Geofence: Inside, Outside, Unknown.
 * There are 3 transitions: ENTERED, EXITED, UNCERTAIN.
 *
 * An example state diagram with confidence level: 95% and Unknown time limit
 * set as 30 secs is shown below. (confidence level and Unknown time limit are
 * explained latter)
 *                         ____________________________
 *                        |       Unknown (30 secs)   |
 *                         """"""""""""""""""""""""""""
 *                            ^ |                  |  ^
 *                   UNCERTAIN| |ENTERED     EXITED|  |UNCERTAIN
 *                            | v                  v  |
 *                        ________    EXITED     _________
 *                       | Inside | -----------> | Outside |
 *                       |        | <----------- |         |
 *                        """"""""    ENTERED    """""""""
 *
 * Inside state: We are 95% confident that the user is inside the geofence.
 * Outside state: We are 95% confident that the user is outside the geofence
 * Unknown state: Rest of the time.
 *
 * The Unknown state is better explained with an example:
 *
 *                            __________
 *                           |         c|
 *                           |  ___     |    _______
 *                           |  |a|     |   |   b   |
 *                           |  """     |    """""""
 *                           |          |
 *                            """"""""""
 * In the diagram above, "a" and "b" are 2 geofences and "c" is the accuracy
 * circle reported by the GPS subsystem. Now with regard to "b", the system is
 * confident that the user is outside. But with regard to "a" is not confident
 * whether it is inside or outside the geofence. If the accuracy remains the
 * same for a sufficient period of time, the UNCERTAIN transition would be
 * triggered with the state set to Unknown. If the accuracy improves later, an
 * appropriate transition should be triggered.  This "sufficient period of time"
 * is defined by the parameter in the add_geofence_area API.
 *     In other words, Unknown state can be interpreted as a state in which the
 * GPS subsystem isn't confident enough that the user is either inside or
 * outside the Geofence. It moves to Unknown state only after the expiry of the
 * timeout.
 *
 * The geofence callback needs to be triggered for the ENTERED and EXITED
 * transitions, when the GPS system is confident that the user has entered
 * (Inside state) or exited (Outside state) the Geofence. An implementation
 * which uses a value of 95% as the confidence is recommended. The callback
 * should be triggered only for the transitions requested by the
 * add_geofence_area call.
 *
 * Even though the diagram and explanation talks about states and transitions,
 * the callee is only interested in the transistions. The states are mentioned
 * here for illustrative purposes.
 *
 * Startup Scenario: When the device boots up, if an application adds geofences,
 * and then we get an accurate GPS location fix, it needs to trigger the
 * appropriate (ENTERED or EXITED) transition for every Geofence it knows about.
 * By default, all the Geofences will be in the Unknown state.
 *
 * When the GPS system is unavailable, gps_geofence_status_callback should be
 * called to inform the upper layers of the same. Similarly, when it becomes
 * available the callback should be called. This is a global state while the
 * UNKNOWN transition described above is per geofence.
 *
 * An important aspect to note is that users of this API (framework), will use
 * other subsystems like wifi, sensors, cell to handle Unknown case and
 * hopefully provide a definitive state transition to the third party
 * application. GPS Geofence will just be a signal indicating what the GPS
 * subsystem knows about the Geofence.
 *
 */
#define GPS_GEOFENCE_ENTERED     (1<<0L)
#define GPS_GEOFENCE_EXITED      (1<<1L)
#define GPS_GEOFENCE_UNCERTAIN   (1<<2L)

#define GPS_GEOFENCE_UNAVAILABLE (1<<0L)
#define GPS_GEOFENCE_AVAILABLE   (1<<1L)

#define GPS_GEOFENCE_OPERATION_SUCCESS           0
#define GPS_GEOFENCE_ERROR_TOO_MANY_GEOFENCES -100
#define GPS_GEOFENCE_ERROR_ID_EXISTS          -101
#define GPS_GEOFENCE_ERROR_ID_UNKNOWN         -102
#define GPS_GEOFENCE_ERROR_INVALID_TRANSITION -103
#define GPS_GEOFENCE_ERROR_GENERIC            -149

/**
 * The callback associated with the geofence.
 * Parameters:
 *      geofence_id - The id associated with the add_geofence_area.
 *      location    - The current GPS location.
 *      transition  - Can be one of GPS_GEOFENCE_ENTERED, GPS_GEOFENCE_EXITED,
 *                    GPS_GEOFENCE_UNCERTAIN.
 *      timestamp   - Timestamp when the transition was detected.
 *
 * The callback should only be called when the caller is interested in that
 * particular transition. For instance, if the caller is interested only in
 * ENTERED transition, then the callback should NOT be called with the EXITED
 * transition.
 *
 * IMPORTANT: If a transition is triggered resulting in this callback, the GPS
 * subsystem will wake up the application processor, if its in suspend state.
 */
typedef void (*gps_geofence_transition_callback)(int32_t geofence_id,  GpsLocation* location,
                                                 int32_t transition, GpsUtcTime timestamp);

/**
 * The callback associated with the availablity of the GPS system for geofencing
 * monitoring. If the GPS system determines that it cannot monitor geofences
 * because of lack of reliability or unavailability of the GPS signals, it will
 * call this callback with GPS_GEOFENCE_UNAVAILABLE parameter.
 *
 * Parameters:
 *  status - GPS_GEOFENCE_UNAVAILABLE or GPS_GEOFENCE_AVAILABLE.
 *  last_location - Last known location.
 */
typedef void (*gps_geofence_status_callback)(int32_t status, GpsLocation* last_location);

/**
 * The callback associated with the add_geofence call.
 *
 * Parameter:
 * geofence_id - Id of the geofence.
 * status - GPS_GEOFENCE_OPERATION_SUCCESS
 *          GPS_GEOFENCE_ERROR_TOO_MANY_GEOFENCES  - geofence limit has been reached.
 *          GPS_GEOFENCE_ERROR_ID_EXISTS  - geofence with id already exists
 *          GPS_GEOFENCE_ERROR_INVALID_TRANSITION - the monitorTransition contains an
 *              invalid transition
 *          GPS_GEOFENCE_ERROR_GENERIC - for other errors.
 */
typedef void (*gps_geofence_add_callback)(int32_t geofence_id, int32_t status);

/**
 * The callback associated with the remove_geofence call.
 *
 * Parameter:
 * geofence_id - Id of the geofence.
 * status - GPS_GEOFENCE_OPERATION_SUCCESS
 *          GPS_GEOFENCE_ERROR_ID_UNKNOWN - for invalid id
 *          GPS_GEOFENCE_ERROR_GENERIC for others.
 */
typedef void (*gps_geofence_remove_callback)(int32_t geofence_id, int32_t status);


/**
 * The callback associated with the pause_geofence call.
 *
 * Parameter:
 * geofence_id - Id of the geofence.
 * status - GPS_GEOFENCE_OPERATION_SUCCESS
 *          GPS_GEOFENCE_ERROR_ID_UNKNOWN - for invalid id
 *          GPS_GEOFENCE_ERROR_INVALID_TRANSITION -
 *                    when monitor_transitions is invalid
 *          GPS_GEOFENCE_ERROR_GENERIC for others.
 */
typedef void (*gps_geofence_pause_callback)(int32_t geofence_id, int32_t status);

/**
 * The callback associated with the resume_geofence call.
 *
 * Parameter:
 * geofence_id - Id of the geofence.
 * status - GPS_GEOFENCE_OPERATION_SUCCESS
 *          GPS_GEOFENCE_ERROR_ID_UNKNOWN - for invalid id
 *          GPS_GEOFENCE_ERROR_GENERIC for others.
 */
typedef void (*gps_geofence_resume_callback)(int32_t geofence_id, int32_t status);

typedef struct {
    gps_geofence_transition_callback geofence_transition_callback;
    gps_geofence_status_callback geofence_status_callback;
    gps_geofence_add_callback geofence_add_callback;
    gps_geofence_remove_callback geofence_remove_callback;
    gps_geofence_pause_callback geofence_pause_callback;
    gps_geofence_resume_callback geofence_resume_callback;
    gps_create_thread create_thread_cb;
} GpsGeofenceCallbacks;

/** Extended interface for GPS_Geofencing support */
typedef struct {
    /** set to sizeof(GpsGeofencingInterface) */
    size_t          size;

    /**
     * Opens the geofence interface and provides the callback routines
     * to the implemenation of this interface.
     */
    void (*init)(GpsGeofenceCallbacks* callbacks);

    /**
     * Add a geofence area. This api currently supports circular geofences.
     * Parameters:
     *    geofence_id - The id for the geofence. If a geofence with this id
     *       already exists, an error value (GPS_GEOFENCE_ERROR_ID_EXISTS)
     *       should be returned.
     *    latitude, longtitude, radius_meters - The lat, long and radius
     *       (in meters) for the geofence
     *    last_transition - The current state of the geofence. For example, if
     *       the system already knows that the user is inside the geofence,
     *       this will be set to GPS_GEOFENCE_ENTERED. In most cases, it
     *       will be GPS_GEOFENCE_UNCERTAIN.
     *    monitor_transition - Which transitions to monitor. Bitwise OR of
     *       GPS_GEOFENCE_ENTERED, GPS_GEOFENCE_EXITED and
     *       GPS_GEOFENCE_UNCERTAIN.
     *    notification_responsiveness_ms - Defines the best-effort description
     *       of how soon should the callback be called when the transition
     *       associated with the Geofence is triggered. For instance, if set
     *       to 1000 millseconds with GPS_GEOFENCE_ENTERED, the callback
     *       should be called 1000 milliseconds within entering the geofence.
     *       This parameter is defined in milliseconds.
     *       NOTE: This is not to be confused with the rate that the GPS is
     *       polled at. It is acceptable to dynamically vary the rate of
     *       sampling the GPS for power-saving reasons; thus the rate of
     *       sampling may be faster or slower than this.
     *    unknown_timer_ms - The time limit after which the UNCERTAIN transition
     *       should be triggered. This paramter is defined in milliseconds.
     *       See above for a detailed explanation.
     */
    void (*add_geofence_area)(int32_t geofence_id, double latitude,
                              double longitude, double radius_meters,
                              int last_transition, int monitor_transitions,
                              int notification_responsiveness_ms,
                              int unknown_timer_ms);

    /**
     * Pause monitoring a particular geofence.
     * Parameters:
     *   geofence_id - The id for the geofence.
     */
    void (*pause_geofence)(int32_t geofence_id);

    /**
     * Resume monitoring a particular geofence.
     * Parameters:
     *   geofence_id - The id for the geofence.
     *   monitor_transitions - Which transitions to monitor. Bitwise OR of
     *       GPS_GEOFENCE_ENTERED, GPS_GEOFENCE_EXITED and
     *       GPS_GEOFENCE_UNCERTAIN.
     *       This supersedes the value associated provided in the
     *       add_geofence_area call.
     */
    void (*resume_geofence)(int32_t geofence_id, int monitor_transitions);

    /**
     * Remove a geofence area. After the function returns, no notifications
     * should be sent.
     * Parameter:
     *   geofence_id - The id for the geofence.
     */
    void (*remove_geofence_area)(int32_t geofence_id);
} GpsGeofencingInterface;
__END_DECLS

#endif /* ANDROID_INCLUDE_HARDWARE_GPS_H */

