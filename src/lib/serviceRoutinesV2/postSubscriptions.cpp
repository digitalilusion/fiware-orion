
/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Orion dev team
*/

#include "mongoBackend/mongoSubscribeContext.h"
#include "ngsi/ParseData.h"
#include "ngsi10/SubscribeContextResponse.h"
#include "common/statistics.h"

#include "serviceRoutinesV2/postSubscriptions.h"


/* ****************************************************************************
*
* postSubscriptions -
*/
extern std::string postSubscriptions
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{

  SubscribeContextResponse  scr;
  std::string               answer;

  if (ciP->servicePathV.size() > 1)
  {
    LM_W(("Bad Input (max *one* service-path allowed for subscriptions (%d given))", ciP->servicePathV.size()));
    scr.subscribeError.errorCode.fill(SccBadRequest, "max one service-path allowed for subscriptions");

    TIMED_RENDER(answer = scr.render(SubscribeContext, ciP->outFormat, ""));
    return answer;
  }

  TIMED_MONGO(ciP->httpStatusCode = mongoSubscribeContext(&parseDataP->scr.res, &scr, ciP->tenant, ciP->uriParam, ciP->httpHeaders.xauthToken, ciP->servicePathV));

  parseDataP->scr.res.release();

  if (scr.subscribeError.errorCode.code != SccNone )
  {
    TIMED_RENDER(answer = scr.render(SubscribeContext, JSON, ""));
    return answer;
  }
  std::string location = "/v2/subscriptions/" + scr.subscribeResponse.subscriptionId.string;
  ciP->httpHeader.push_back("Location");
  ciP->httpHeaderValue.push_back(location);

  ciP->httpStatusCode = SccCreated;

  return "";

}



