#ifndef _13_OTA_H_
#define _13_OTA_H_

#include "RFLink.h"
#ifdef RFLINK_AUTOOTA_ENABLED

#include <WString.h>
#include <ArduinoJson.h>

namespace RFLink
{
  namespace OTA
  {
    enum statusEnum {
      Idle,
      Scheduled,
      InProgress,
      PendingReboot,
      Failed,
    };

    extern statusEnum currentHttpUpdateStatus;

    /*
     * To schedule an HttpUpdate (which must happen in the main loop!)
     *
     **/
    bool scheduleHttpUpdate(const char *url, String &errmsg);

    void getHttpUpdateStatus(JsonObject &json);

    void mainLoop();

  }
} // end of RFLink namespace

#endif //RFLINK_AUTOOTA_ENABLED

#endif // _13_OTA_H_