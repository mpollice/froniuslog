/*********************************************************************
 *** FILE: main.c
 *** 
 *** Copyright (C)  2009 by Jeremy Simmons
 ***
 *** This program is free software: you can redistribute it and/or modify
 *** it under the terms of the GNU General Public License as published by
 *** the Free Software Foundation, either version 3 of the License, or
 *** (at your option) any later version.
 ***
 *** This program is distributed in the hope that it will be useful,
 *** but WITHOUT ANY WARRANTY; without even the implied warranty of
 *** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *** GNU General Public License for more details.
 ***
 *** You should have received a copy of the GNU General Public License
 *** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***
 *********************************************************************/

/* SYSTEM INCLUDE FILES */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

/* INCLUDE FILES */

/* DEFINES */

/* TYPEDEFS */

// Commands supported by the inverter
typedef enum 
{
    GET_POWER_NOW             = 0x10,
    GET_ENERGY_TOTAL          = 0x11,
    GET_ENERGY_DAY            = 0x12,
    GET_ENERGY_YEAR           = 0x13,
    GET_AC_CURRENT_NOW        = 0x14,
    GET_AC_VOLTAGE_NOW        = 0x15,
    GET_AC_FREQUENCY_NOW      = 0x16,
    GET_DC_CURRENT_NOW        = 0x17,
    GET_DC_VOLTAGE_NOW        = 0x18,
    GET_YIELD_DAY             = 0x19,
    GET_MAX_POWER_DAY         = 0x1A,
    GET_MAX_AC_VOLTAGE_DAY    = 0x1B,
    GET_MIN_AC_VOLTAGE_DAY    = 0x1C,
    GET_MAX_DC_VOLTAGE_DAY    = 0x1D,
    GET_OPERATING_HOURS_DAY   = 0x1E,
    GET_YIELD_YEAR            = 0x1F,
    GET_MAX_POWER_YEAR        = 0x20,
    GET_MAX_AC_VOLTAGE_YEAR   = 0x21,
    GET_MIN_AC_VOLTAGE_YEAR   = 0x22,
    GET_MAX_DC_VOLTAGE_YEAR   = 0x23,
    GET_OPERATING_HOURS_YEAR  = 0x24,
    GET_YIELD_TOTAL           = 0x25,
    GET_MAX_POWER_TOTAL       = 0x26,
    GET_MAX_AC_VOLTAGE_TOTAL  = 0x27,
    GET_MIN_AC_VOLTAGE_TOTAL  = 0x28,
    GET_MAX_DC_VOLTAGE_TOTAL  = 0x29,
    GET_OPERATING_HOURS_TOTAL = 0x2A,
    GET_PHASE_1_CURRENT       = 0x2B,
    GET_PHASE_2_CURRENT       = 0x2C,
    GET_PHASE_3_CURRENT       = 0x2D,
    GET_PHASE_1_VOLTAGE       = 0x2E,
    GET_PHASE_2_VOLTAGE       = 0x2F,
    GET_PHASE_3_VOLTAGE       = 0x30,
    GET_AMBIENT_TEMPERATURE   = 0x31,
    GET_FRONT_LEFT_FAN_SPEED  = 0x32,
    GET_FRONT_RIGHT_FAN_SPEED = 0x33,
    GET_REAR_LEFT_FAN_SPEED   = 0x34,
    GET_REAR_RIGHT_FAN_SPEED  = 0x35
} cmd_t;

// Commands that we're going to send to the inverter periodically
unsigned char cmds[] = 
{
    GET_POWER_NOW             ,
    GET_ENERGY_TOTAL          ,
    GET_ENERGY_DAY            ,
    GET_ENERGY_YEAR           ,
    GET_AC_CURRENT_NOW        ,
    GET_AC_VOLTAGE_NOW        ,
    GET_AC_FREQUENCY_NOW      ,
    GET_DC_CURRENT_NOW        ,
    GET_DC_VOLTAGE_NOW        ,
    GET_YIELD_DAY             ,
    GET_MAX_POWER_DAY         ,
    GET_MAX_AC_VOLTAGE_DAY    ,
    GET_MIN_AC_VOLTAGE_DAY    ,
    GET_MAX_DC_VOLTAGE_DAY    ,
    GET_OPERATING_HOURS_DAY   ,
    GET_YIELD_YEAR            ,
    GET_MAX_POWER_YEAR        ,
    GET_MAX_AC_VOLTAGE_YEAR   ,
    GET_MIN_AC_VOLTAGE_YEAR   ,
    GET_MAX_DC_VOLTAGE_YEAR   ,
    GET_OPERATING_HOURS_YEAR  ,
    GET_YIELD_TOTAL           ,
    GET_MAX_POWER_TOTAL       ,
    GET_MAX_AC_VOLTAGE_TOTAL  ,
    GET_MIN_AC_VOLTAGE_TOTAL  ,
    GET_MAX_DC_VOLTAGE_TOTAL  ,
    GET_OPERATING_HOURS_TOTAL ,
    GET_PHASE_1_CURRENT       ,
    GET_PHASE_2_CURRENT       ,
    GET_PHASE_3_CURRENT       ,
    GET_PHASE_1_VOLTAGE       ,
    GET_PHASE_2_VOLTAGE       ,
    GET_PHASE_3_VOLTAGE       ,
    GET_AMBIENT_TEMPERATURE   ,
    GET_FRONT_LEFT_FAN_SPEED  ,
    GET_FRONT_RIGHT_FAN_SPEED ,
    GET_REAR_LEFT_FAN_SPEED   ,
    GET_REAR_RIGHT_FAN_SPEED  
};

#define CMD_COUNT (sizeof(cmds)/sizeof(cmds[0]))

// Fronius message header
typedef struct
{
    unsigned char start[3];
    unsigned char length;
    unsigned char device;
    unsigned char number;
    unsigned char command;
    unsigned char data[0];
} __attribute__((__packed__)) msgHeader_t;

/* STATIC VARIABLES */

/* GLOBAL VARIABLES */

/* FUNCTIONS */
/*********************************************************************
 *** FUNCTION: usage
 *** 
 *** DESCRIPTION:
 ***   Print help about the command line arguments, then exit
 ***
 *** RETURN VALUE:
 ***   None.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
static void usage(const char *argv0)
{
    printf("usage: %s [-f port] [-d dir]\n", argv0);
    printf("       port = the serial port to use (i.e. /dev/ttyS0)\n");
    printf("       dir  = the root directory to write the data files to\n");
    exit(0);
}

/*********************************************************************
 *** FUNCTION: initPort
 *** 
 *** DESCRIPTION:
 ***   Open the serial port, set the IO modes.
 ***
 *** RETURN VALUE:
 ***   The file descriptor to use for the serial port. .
 ***
 *** SIDE EFFECTS:
 ***   Exits on any errors.
 *********************************************************************/
int initPort(const char *port)
{
    int fd;
    struct termios t;
    int r;

    fd = open(port, O_NONBLOCK | O_RDWR);
    if (fd < 0)
    {
        printf("open failed: %s\n", strerror(errno));
        exit(0);
    }

    // Set the IO modes on the serial port
    r = tcgetattr(fd, &t);
    if (r != 0)
    {
        printf("tcgetattr failed: %s\n", strerror(errno));
        exit(0);
    }

    cfmakeraw(&t);
    r = cfsetispeed(&t, B19200);
    if (r != 0)
    {
        printf("cfsetispeed failed: %s\n", strerror(errno));
        exit(0);
    }
    
    r = cfsetospeed(&t, B19200);
    if (r != 0)
    {
        printf("cfsetospeed failed: %s\n", strerror(errno));
        exit(0);
    }

    r = tcsetattr(fd, TCSANOW, &t);
    if (r != 0)
    {
        printf("tcsetattr failed: %s\n", strerror(errno));
        exit(0);
    }

    return fd;
}

/*********************************************************************
 *** FUNCTION: readMsg
 *** 
 *** DESCRIPTION:
 ***   Read a message on the serial port
 ***
 *** RETURN VALUE:
 ***   The number of bytes in the message. Returns 0 if there's an
 ***   error or timeout.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
static int readMsg(int fd, unsigned char *buf, int len)
{
    static char readbuf[100];
    int r;
    int readBufTail = 0;
    unsigned char start[] = { 0x80, 0x80, 0x80 };
    int i;

    for ( ; ; )
    {
        fd_set readFds;

        // One second timeout
        struct timeval timeout = { 1, 0 };
        
        FD_ZERO(&readFds);
        FD_SET(fd, &readFds);

        r = select(fd+1, &readFds, NULL, NULL, &timeout);
        if (r < 0)
        {
            // Error
            printf("select failed: %s\n", strerror(errno));
            return 0;
        }
        if (r == 0)
        {
            // Timed out, nothing to read
            return 0;
        }

        // We should be good to go here.
        r = read(fd, &readbuf[readBufTail], sizeof(readbuf) - readBufTail);
        readBufTail += r;

        // Figure out if there's a whole message in the readbuf
        for (i=0; i<readBufTail; i++)
        {
            unsigned char cksumRecv;
            unsigned char cksum = 0;
            msgHeader_t *hdr = (msgHeader_t *)&readbuf[i];
            int j;
            
            // Check if there's enough in the buffer before processing
            if (i+sizeof(msgHeader_t) > readBufTail)
            {
                // Loop again to read more bytes
                break;
            }
            
            if (memcmp(&readbuf[i], start, 3) != 0)
            {
                // No start flag, keep going
                break;
            }

            // Check the length field in the header
            if (sizeof(msgHeader_t) + hdr->length + 1 > readBufTail)
            {
                // Still need to read more bytes
                break;
            }

            // OK, we have the right message length. Verify the checksum.
            cksumRecv = readbuf[readBufTail - 1];
            for(j=i+3; j<readBufTail-1; j++)
            {
                cksum += readbuf[j];
            }
            if (cksum != cksumRecv)
            {
                printf("bad message checksum [%d]: %d != %d\n", readBufTail-1,
                       cksum, cksumRecv);
                return 0;
            }
            memcpy(buf, &readbuf[i], sizeof(msgHeader_t)+hdr->length+1);
            return sizeof(msgHeader_t)+hdr->length+1;
        }
    }
    return 0;
}

/*********************************************************************
 *** FUNCTION: writeMsg
 *** 
 *** DESCRIPTION:
 ***   Send a message on the serial port
 ***
 *** RETURN VALUE:
 ***   Always returns the number of bytes written. 
 ***
 *** SIDE EFFECTS:
 ***   Exits if the write fails.
 *********************************************************************/
static int writeMsg(int fd, unsigned char *msg, int len)
{
    msgHeader_t *hdr = (msgHeader_t *)msg;
    int i;
    unsigned char cksum = 0;
    int r;

    // Build the header.
    hdr->start[0] = 0x80;
    hdr->start[1] = 0x80;
    hdr->start[2] = 0x80;

    // Set the length field
    hdr->length = len;

    // Compute the checksum
    for (i=sizeof(hdr->start); i<sizeof(*hdr) + len; i++)
    {
        cksum += msg[i];
    }
    msg[i] = cksum;

    // Send the message on the serial port
    r = write(fd, msg, sizeof(*hdr) + len + 1);
    if (r != (sizeof(*hdr) + len + 1))
    {
        printf("write failed: %s", strerror(errno));
        exit(0);
    }

    return len;
}

/*********************************************************************
 *** FUNCTION: getVersion
 *** 
 *** DESCRIPTION:
 ***   Get the software version of the interface card
 ***
 *** RETURN VALUE:
 ***   Returns 1 if the request was successful, 0 otherwise. Software
 ***   versions are returned in major, minor and release.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
static int getVersion(int fd, unsigned char *major, unsigned char *minor,
                      unsigned char *release)
{
    // The data section of the message
    typedef struct 
    {
        unsigned char major;
        unsigned char minor;
        unsigned char release;
    } __attribute__((__packed__)) version_t;
    
    unsigned char msgbuf[100];
    msgHeader_t *hdr;
    version_t *version;
    int r;
    
    hdr = (msgHeader_t *)msgbuf;

    hdr->device = 0;
    hdr->number = 0;
    hdr->command = 0x01; // Get version command

    writeMsg(fd, msgbuf, 0);
    r = readMsg(fd, msgbuf, sizeof(msgbuf));
    if (r == 0)
    {
        printf("readMsg timed out (get version)\n");
        return 0;
    }

    version = (version_t *)hdr->data;
    *major = version->major;
    *minor = version->minor;
    *release = version->release;

    return 1;
}

/*********************************************************************
 *** FUNCTION: getActiveInverter
 *** 
 *** DESCRIPTION:
 ***   Get the number of the active inverter.
 ***
 *** RETURN VALUE:
 ***   0 if no inverters are active, 1 otherwise. ID number of active
 ***   inverter is returned in active.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
static int getActiveInverter(int fd, unsigned char *active)
{
    typedef struct 
    {
        unsigned char active;
    } __attribute__((__packed__)) activeInv_t;
    
    unsigned char msgbuf[100];
    msgHeader_t *hdr;
    activeInv_t *activeInv;
    int r;
    
    hdr = (msgHeader_t *)msgbuf;

    hdr->device = 0;
    hdr->number = 0;
    hdr->command = 0x04;

    writeMsg(fd, msgbuf, 0);
    r = readMsg(fd, msgbuf, sizeof(msgbuf));
    if (r == 0)
    {
        printf("readMsg timed out (get active inverter)\n");
        return 0;
    }

    if (hdr->length == 0)
    {
        return 0;
    }
    
    activeInv = (activeInv_t *)hdr->data;
    *active = activeInv->active;

    return 1;
}

/*********************************************************************
 *** FUNCTION: typeIdToStr
 *** 
 *** DESCRIPTION:
 ***   Convert the typeId of the inverter to a string.
 ***
 *** RETURN VALUE:
 ***   Constant string name.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
static const char *typeIdToStr(unsigned char typeId)
{
    switch (typeId)
    {
        case 0xFE:
        return "FRONIUS IG 15";

        case 0xFD:
        return "FRONIUS IG 20";
        
        case 0xFC:
        return "FRONIUS IG 30";
        
        case 0xFB:
        return "FRONIUS IG 30 Dummy";
        
        case 0xFA:
        return "FRONIUS IG 40";
        
        case 0xF9:
        return "FRONIUS IG 60/IG 60 HV";
        
        case 0xF6:
        return "FRONIUS IG 300";

        case 0xF5:
        return "FRONIUS IG 400";

        case 0xF4:
        return "FRONIUS IG 500";

        case 0xF3:
        return "FRONIUS IG 60/IG 60 HV";

        case 0xEE:
        return "FRONIUS IG 2000";

        case 0xED:
        return "FRONIUS IG 3000";

        case 0xEB:
        return "FRONIUS IG 4000";

        case 0xEA:
        return "FRONIUS IG 5100";

        case 0xE5:
        return "FRONIUS IG 2500-LV";

        case 0xE3:
        return "FRONIUS IG 4500-LV";

        case 0xFF:
        default:
        return "Unknown device";
    }
}

/*********************************************************************
 *** FUNCTION: getDeviceType
 *** 
 *** DESCRIPTION:
 ***   Get the device type of the active inverter.
 ***
 *** RETURN VALUE:
 ***   1 for success, 0 for failure. Device type returned in typeId.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
static int getDeviceType(int fd, unsigned char number, unsigned char *typeId)
{
    unsigned char msgbuf[100];
    msgHeader_t *hdr;
    int r;
    
    hdr = (msgHeader_t *)msgbuf;

    hdr->device = 1;
    hdr->number = number;
    hdr->command = 0x02;

    writeMsg(fd, msgbuf, 0);
    r = readMsg(fd, msgbuf, sizeof(msgbuf));
    if (r == 0)
    {
        printf("readMsg timed out (get active inverter)\n");
        return 0;
    }

    if (hdr->length != 1)
    {
        return 0;
    }
    
    *typeId = *hdr->data;

    return 1;
}

/*********************************************************************
 *** FUNCTION: getNumeric
 *** 
 *** DESCRIPTION:
 ***   Get a numeric parameter from the inverter.
 ***
 *** RETURN VALUE:
 ***   1 for success, 0 for failure. Value returned in f.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
static int getNumeric(int fd, unsigned char number, unsigned char cmd, float *f)
{
    unsigned char msgbuf[100];
    msgHeader_t *hdr;
    int r;
    short value;
    signed char exponent;
    
    hdr = (msgHeader_t *)msgbuf;

    hdr->device  = 1;
    hdr->number  = number;
    hdr->command = cmd;

    writeMsg(fd, msgbuf, 0);
    r = readMsg(fd, msgbuf, sizeof(msgbuf));
    if (r == 0)
    {
        printf("readMsg timed out (get active inverter)\n");
        return 0;
    }

    if (hdr->length != 3)
    {
        return 0;
    }
    
    memcpy(&value, hdr->data, 2);
    value = ntohs(value);
    exponent = (char)hdr->data[2];
    if ((exponent < -3) || (exponent > 10))
        return 0;

    *f = value * powf(10, exponent);

    return 1;
}

/*********************************************************************
 *** FUNCTION: delay
 *** 
 *** DESCRIPTION:
 ***   Delay until it's time for the next event.
 ***
 *** RETURN VALUE:
 ***   None.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
static void delay(struct timeval *nextEvent)
{
    struct timeval now;
    struct timeval timetmp;
    useconds_t sleepyTime;
    struct timeval inc = { 60, 0 };

    gettimeofday(&now, NULL);

    // If it's already time to do stuff, return
    if (timercmp(&now, nextEvent, >=))
    {
        timeradd(nextEvent, &inc, &timetmp);
        *nextEvent = timetmp;
        return;
    }

    // Wait until it's time to do something else
    timersub(nextEvent, &now, &timetmp);

    sleepyTime = (timetmp.tv_sec * 100000) + timetmp.tv_usec;

    usleep(sleepyTime);
    timeradd(nextEvent, &inc, &timetmp);
    *nextEvent = timetmp;
}

/*********************************************************************
 *** FUNCTION: makePath
 *** 
 *** DESCRIPTION:
 ***   Generate the path for the CSV data file and the index.html
 ***
 *** RETURN VALUE:
 ***   Returns the complete path to the file in path.
 ***
 *** SIDE EFFECTS:
 ***   Creates directories if they don't exist.
 *********************************************************************/
static void makePath(const char *dir, const char *filename, char *path, int pathLen)
{
    struct timeval t;
    struct tm *tmTime;
    char tmp[255];

    // File path is "<dir>/Year/Month/Day/file". For example:
    // /tmp/2009/03/23/data.csv
    gettimeofday(&t, NULL);
    tmTime = localtime(&t.tv_sec);

    snprintf(path, pathLen, "%s", dir);
    mkdir(path, 0755);

    snprintf(tmp, sizeof(tmp), "/%04d", tmTime->tm_year+1900);
    strncat(path, tmp, pathLen);
    mkdir(path, 0755);

    snprintf(tmp, sizeof(tmp), "/%02d", tmTime->tm_mon+1);
    strncat(path, tmp, pathLen);
    mkdir(path, 0755);
    
    snprintf(tmp, sizeof(tmp), "/%02d", tmTime->tm_mday);
    strncat(path, tmp, pathLen);
    mkdir(path, 0755);
    
    snprintf(tmp, sizeof(tmp), "/%s", filename);
    strncat(path, tmp, pathLen);
}

/*********************************************************************
 *** FUNCTION: openFile
 *** 
 *** DESCRIPTION:
 ***   Open the CSV data file
 ***
 *** RETURN VALUE:
 ***   FILE pointer to open file. NULL if there's an error opening the
 ***   file. newFile is set to 1 if a new file was created, 0 if we are
 ***   appending to an existing file.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
static FILE *openFile(const char *dir, int *newFile)
{
    char path[255];
    struct stat statbuf;
    FILE *f;
    int r;

    *newFile = 0;
    
    makePath(dir, "data.csv", path, sizeof(path));

    r = stat(path, &statbuf);
    if (r != 0)
    {
        if (errno == ENOENT)
        {
            f = fopen(path, "a");
            if (f == NULL)
            {
                printf("fopen(%s) failed: %s\n", path, strerror(errno));
                exit(0);
            }
            *newFile = 1;
        }
        else
        {
            printf("stat(%s) failed: %s\n", path, strerror(errno));
            exit(0);
        }
    }

    if (*newFile == 0)
    {
        f = fopen(path, "a");
        if (f == NULL)
        {
            printf("fopen(%s) failed: %s", path, strerror(errno));
            exit(0);
        }
    }

    return f;
}

/*********************************************************************
 *** FUNCTION: updateHtml
 *** 
 *** DESCRIPTION:
 ***   Generate an index.html file with the current output.
 ***
 *** RETURN VALUE:
 ***   None.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
static void updateHtml(const char *dir, short *watts, int wattsCount, float energyNow,
                       float energyDay, time_t startTime)
{
    FILE *f;
    char path[255];
    int i;
    int max = 0;
    int energyNowInt = energyNow; 
    int energyDayInt = energyDay;
    struct timeval now;
    struct tm *lt, *st;
    float startX, stopX;

    gettimeofday(&now, NULL);
    lt = localtime(&now.tv_sec);

    st = localtime(&startTime);

    startX = st->tm_min;
    startX /= 60;
    startX += st->tm_hour;
    stopX = startX + 15.0;

    makePath(dir, "index.html", path, sizeof(path));

    // Open up the index.html file
    f = fopen(path, "w+");
    if (f == NULL)
    {
        printf("Failed to open %s: %s\n", path, strerror(errno));
        return;
    }
    
    for (i = 0; i < wattsCount; i++)
    {
        if (watts[i] > max)
        {
        	max = watts[i];
        }
    }

    // Write out the current output
    fprintf(f, "<html>\n");
    fprintf(f, "Current Power:      %d W<br>\n", energyNowInt);
    fprintf(f, "Today's Power:      %d kWh<br>\n", energyDayInt/1000);
    fprintf(f, "<img src=http://chart.apis.google.com/chart?cht=lc");
    fprintf(f, "&chxt=x,y&chxr=0,%g,%g|1,0,%u", startX, stopX, (max + (50 - max%50)));
    fprintf(f, "&chtt=Power+(watts)&chd=t:");
    for (i=0; i<wattsCount; i++)
    {
        fprintf(f, "%d,", watts[i]);
    }
    for ( ; i<60; i++)
    {
        fprintf(f, "0");
        if (i < 59)
            fprintf(f, ",");
    }
    fprintf(f, "&chds=0,%u&chs=800x370><br>\n", (max + (50 - max%50)));
    fprintf(f, "Raw data:           <a href=data.csv>data.csv</a><br>\n");
    fprintf(f, "Last update: %02d:%02d %d-%02d-%02d<br>\n", lt->tm_hour, lt->tm_min,
            lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday);
    fprintf(f, "</html>\n");

    fclose(f);
}

/*********************************************************************
 *** FUNCTION: main
 *** 
 *** DESCRIPTION:
 ***   Where it all starts
 ***
 *** RETURN VALUE:
 ***   None.
 ***
 *** SIDE EFFECTS:
 ***   None.
 *********************************************************************/
int main(int argc, char *argv[])
{
    // Default serial port
    char *port = "/dev/ttyS0";

    // Default root directory
    char *dir  = ".";
    
    int i, j, k;

    // Software version from interface card
    unsigned char major, minor, release, active, typeId;

    // Serial port fd
    int fd;
    int r;
    float fval;

    // File pointer of the data file
    FILE *f = NULL;
    
    struct timeval t, timestamp, startTime;
    int firstPower = 0;
    
    int newFile = 0;
    struct tm *ltime;

    // Current usage and total kWh for the day to put on the web page
    float energyNow = 0, energyDay = 0;

    // Watts averaged every 15 minutes 
    static short watts15[100];
    static int watts15Count = 0;

    // Watts sampled every minute
    static short watts[15];
    static int wattsCount = 0;

    // Process command line arguments
    for (i=0; i<argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            if ((i+1) >= argc)
                usage(argv[0]);
            else
                port = argv[i+1];
        }
        if (strcmp(argv[i], "-d") == 0)
        {
            if ((i+1) >= argc)
                usage(argv[0]);
            else
                dir = argv[i+1];
        }
    }

    // Open the serial port
    fd = initPort(port);

    gettimeofday(&t, NULL);

    for ( ; ; )
    {
        getVersion(fd, &major, &minor, &release);
        
        r = getActiveInverter(fd, &active);
        if (r == 0)
        {
            if (f != NULL)
            {
                fclose(f);
                f = NULL;
            }

            delay(&t);
            continue;
        }
        
        r = getDeviceType(fd, active, &typeId);
        if (r != 1)
            printf("Couldn't get device type\n");

        if (f == NULL)
        {
            f = openFile(dir, &newFile);
            if (f == NULL)
            {
                printf("No file\n");
                exit(0);
            }
            if (newFile != 0)
            {
                firstPower   = 0;
                wattsCount   = 0;
                watts15Count = 0;
                energyDay    = 0;
                fprintf(f, "Software version: %d.%d.%d\n", major, minor, release);
                fprintf(f, "Inverter model: %s\n", typeIdToStr(typeId));
                fprintf(f,
                        "TIMESTAMP             ,"
                        "POWER_NOW             ,"
                        "ENERGY_TOTAL          ,"
                        "ENERGY_DAY            ,"
                        "ENERGY_YEAR           ,"
                        "AC_CURRENT_NOW        ,"
                        "AC_VOLTAGE_NOW        ,"
                        "AC_FREQUENCY_NOW      ,"
                        "DC_CURRENT_NOW        ,"
                        "DC_VOLTAGE_NOW        ,"
                        "YIELD_DAY             ,"
                        "MAX_POWER_DAY         ,"
                        "MAX_AC_VOLTAGE_DAY    ,"
                        "MIN_AC_VOLTAGE_DAY    ,"
                        "MAX_DC_VOLTAGE_DAY    ,"
                        "OPERATING_HOURS_DAY   ,"
                        "YIELD_YEAR            ,"
                        "MAX_POWER_YEAR        ,"
                        "MAX_AC_VOLTAGE_YEAR   ,"
                        "MIN_AC_VOLTAGE_YEAR   ,"
                        "MAX_DC_VOLTAGE_YEAR   ,"
                        "OPERATING_HOURS_YEAR  ,"
                        "YIELD_TOTAL           ,"
                        "MAX_POWER_TOTAL       ,"
                        "MAX_AC_VOLTAGE_TOTAL  ,"
                        "MIN_AC_VOLTAGE_TOTAL  ,"
                        "MAX_DC_VOLTAGE_TOTAL  ,"
                        "OPERATING_HOURS_TOTAL ,"
                        "PHASE_1_CURRENT       ,"
                        "PHASE_2_CURRENT       ,"
                        "PHASE_3_CURRENT       ,"
                        "PHASE_1_VOLTAGE       ,"
                        "PHASE_2_VOLTAGE       ,"
                        "PHASE_3_VOLTAGE       ,"
                        "AMBIENT_TEMPERATURE   ,"
                        "FRONT_LEFT_FAN_SPEED  ,"
                        "FRONT_RIGHT_FAN_SPEED ,"
                        "REAR_LEFT_FAN_SPEED   ,"
                        "REAR_RIGHT_FAN_SPEED\n");
            }
        }

        gettimeofday(&timestamp, NULL);
        ltime = localtime(&timestamp.tv_sec);
        fprintf(f, "%d-%02d-%02d %02d:%02d:%02d,", ltime->tm_year+1900, ltime->tm_mon+1,
                ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
        energyNow = 0;

        // Try every command on the inverter and save the result in a CSV file.
        for (j=0; j<CMD_COUNT; j++)
        {
            r = getNumeric(fd, active, cmds[j], &fval);
            if (r != 0)
            {
                // None of the data seems to have more than 1/100 precision
                fprintf(f, "%g,", fval);

                // Everything gets put in the CSV file. This lets us intercept some
                // parameters to put in the HTML file.
                switch (cmds[j])
                {
                    case GET_POWER_NOW:
                    {
                        if (firstPower == 0)
                        {
                            firstPower = 1;
                            gettimeofday(&startTime, NULL);
                        }
                        
                        // Store the current power output in a circular buffer.
                        watts[wattsCount] = fval;
                        wattsCount = (wattsCount + 1) % 15;

                        // Every 15 minutes, take the average and store it. This
                        // will be used to make the graph on the HTML page.
                        if ((ltime->tm_min % 15) == 0)
                        {
                            int wattsAvg = 0;
                            for (k=0; k<15; k++)
                            {
                                wattsAvg += watts[k];
                            }
                            wattsAvg /= 15;
                            
                            watts15[watts15Count++] = wattsAvg;
                        }
                        energyNow = fval;
                    }
                    break;

                    case GET_ENERGY_DAY:
                    {
                        // When the inverter is shutting off, energyDay gets reset
                        // to 0. 
                        if (fval >= energyDay)
                            energyDay = fval;
                    }
                    break;

                    default:
                    break;
                }
            }
            else
            {
                fprintf(f, ",");
            }
        }
        fprintf(f, "\n");
        fflush(f);

        // Update the current web page
        updateHtml(dir, watts15, watts15Count, energyNow, energyDay, startTime.tv_sec);
        
        delay(&t);
    }
    
    return 0;
}

/*********************************************************************
 *** EDIT HISTORY FOR FILE
 ***
 *** This section contains comments describing changes made to the module.
 *** Notice that changes are listed in reverse chronological order.
 *** Comments are pasted here automatically by CVS, extracted from user
 *** text comments entered during cvs commit operations.
 ***
 *** Copyright (c) 2009 by Jeremy Simmons
 ***
 *** $Log$
 *********************************************************************/
