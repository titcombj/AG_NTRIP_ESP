#ifndef UBXGPSNAVRELPOSNED_H_
#define UBXGPSNAVRELPOSNED_H_

#include <UbxGps.h>

template <class T>
class UbxGpsNavRelposned : public UbxGps<T>
{
  public:
    // Type         Name           Unit     Description (scaling)
    unsigned char   version     //          should be 0x01 for this version
    unsigned char   reserved0   //          Reserved
    unsigned short  refStationID
                                //          Reference station in use
    unsigned long   iTOW;       // ms       GPS time of week of the navigation epoch. See the description of iTOW for 
                                //          details
    long            relPosN     // cm       North component of relative position vector
    long            relPosE     // cm       East component of relative position vector
    long            relPosD     // cm       Down component of relative position vector
    long            relPosLength
                                // cm       Length of the relative position vector
    long            relPosHeading
                                // 1e-5 deg Heading of the relative position vector
    long            reserved1   //          Reserved
    signed char     relPosHPN   // 0.1mm    High precision North component of the relative position vector
    signed char     relPosHPE   // 0.1mm    High precision East component of the relative position vector
    signed char     relPosHPD   // 0.1mm    High precision Down component of the relative position vector
    signed char     relPosHPLength
                                // 0.1mm    High precision Length component of the relative positon vector
    unsigned long   accN        // 0.1mm    Accuracy of the relative position North component
    unsigned long   accE        // 0.1mm    Accuracy of the relative position East component
    unsigned long   accD        // 0.1mm    Accuracy of the relative position Down component
    unsigned long   accLength   // 0.1mm    Accuracy of the relative position Length component.
    unsigned long   accHeading  // 1e-5 deg Accuracy of the relative position Heading component.
    long            reserved2   //          Reserved
    long            flags       //          validity flags

    UbxGpsNavRelposned(T &serial) : UbxGps<T>(serial)
    {
        this->setLength(64);
    }
};

#endif