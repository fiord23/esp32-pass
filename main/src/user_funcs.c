#include "user_funcs.h"


bool mac_parsing(uint64_t mac_from_base, uint64_t mac_recieved)
{
    if(mac_from_base == mac_recieved)
    {
        return 0;
    }
    else 
    {
        return 1;
    }
}