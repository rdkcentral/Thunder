#ifndef __ACCES_PROVISION_H__
#define __ACCES_PROVISION_H__

extern "C" {

/*
 * GetDeviceId - function to obtain the unique Device ID 
 *
 * Parameters
 *  MaxIdLength - holds the maximal char length of the Id parameter
 *  Id          - Buffer holds the unique Device ID upon succession, empty in case of failure
 *
 * Return value
 *  < 0 - failure, absolute value returned is the length required to store the Id
 *  > 0 - success, char length of the returned Id
 *
 * Post-condition; return value 0 should not occur
 *
 */
int GetDeviceId(unsigned short MaxIdLength, char Id[]);

/*
 * GetDRMId - function to obtain the DRM Id in the clear
 *
 * Parameters
 *  MaxIdLength - holds the maximal char length of the Id parameter
 *  Id          - Buffer holds the DRM ID upon succession, empty in case of failure
 *  label       - Name of the DRMId requested (if not supplied, label == playready)
 *
 * Return value
 *  < 0 - failure, absolute value returned is the length required to store the Id
 *  > 0 - success, char length of the returned Id
 *
 * Post-condition; return value 0 should not occur
 *
 */

int GetDRMId(const char label[], const unsigned short MaxIdLength, char Id[]);

}

#endif // __ACCES_PROVISION_H__
