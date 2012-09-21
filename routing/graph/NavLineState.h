/*
 * NavLineState.h
 *
 *  Created on: Sep 18th Sep 2012
 *      Author: David Haensel
 */

#ifndef NAVLINESTATE_H_
#define NAVLINESTATE_H_


#include <time.h>
#include <iostream>

//time between a pedestrian got the information and uses the information
#define INFO_OFFSET CLOCKS_PER_SEC*3

class NavLineState 
{

public:
    NavLineState();
    NavLineState(bool open);
    NavLineState(const NavLineState & orig);
    ~NavLineState();
    
    bool closed();
    bool isShareable();
    void close();
    bool mergeDoor(NavLineState & orig);
    void print();

private:
    bool open; // aka state
    int timeFirstSeen; // number of clocks till the door was seen changed the first time
    int timeOfInformation; // number of clocks when i got the information. should be set to zero after a period of time is over (to 
    
};
#endif /* ROUTINGGRAPHSTORAGE_H_ */

