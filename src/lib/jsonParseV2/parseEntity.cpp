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
* Author: Ken Zangelin
*/
#include "rapidjson/document.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "jsonParseV2/jsonParseTypeNames.h"
#include "jsonParseV2/parseEntity.h"
#include "jsonParseV2/parseContextAttribute.h"

using namespace rapidjson;


/* ****************************************************************************
*
* parseEntityWithKeyValues - 
*/
std::string parseEntityWithKeyValues(ConnectionInfo* ciP, Entity* eP, bool eidInURL)
{
  Document document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    LM_W(("Bad Input (JSON parse error)"));
    eP->errorCode.fill("ParseError", "Errors found in incoming JSON buffer");
    ciP->httpStatusCode = SccBadRequest;;
    return eP->render(ciP, EntitiesRequest);
  }


  if (!document.IsObject())
  {
    LM_E(("Bad Input (JSON Parse Error)"));
    eP->errorCode.fill("ParseError", "Error parsing incoming JSON buffer");
    ciP->httpStatusCode = SccBadRequest;;
    return eP->render(ciP, EntitiesRequest);
  }

  if (document.HasMember("id"))
  {
    LM_W(("Bad Input (entity id specified in payload"));
    eP->errorCode.fill("BadRequest", "entity id specified in payload");
    ciP->httpStatusCode = SccBadRequest;;
    return eP->render(ciP, EntitiesRequest);
  }

  for (Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string       name   = iter->name.GetString();
    std::string       type   = jsonParseTypeNames[iter->value.GetType()];
    ContextAttribute* caP = new ContextAttribute();

    eP->attributeVector.push_back(caP);

    std::string r = parseContextAttribute(ciP, iter, caP);
    if (r != "OK")
    {
      LM_W(("Bad Input (parse error in context attribute)"));
      eP->errorCode.fill("BadRequest", r);
      ciP->httpStatusCode = SccBadRequest;
      return eP->render(ciP, EntitiesRequest);
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* parseEntity - 
*
* This function is used to parse two slightly different payloads, and one very different one:
* - POST /v2/entities
* - POST /v2/entities/<eid>
* - POST /v2/entities/<eid>?options=keyValues  (the very different one)
*
* In the first case, "id" MUST be in the payload, while in the other two cases,
* "id" CANNOT be in the payload.
*
* In the case of /v2/entities/<eid>, the entityId of 'Entity* eP' is set in
* the service routine postEntity.
* 
* 
*/
std::string parseEntity(ConnectionInfo* ciP, Entity* eP, bool eidInURL)
{
  if (ciP->uriParamOptions["keyValues"] == true)
  {
    return parseEntityWithKeyValues(ciP, eP, eidInURL);
  }


  Document document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    LM_W(("Bad Input (JSON parse error)"));
    eP->errorCode.fill("ParseError", "Errors found in incoming JSON buffer");
    ciP->httpStatusCode = SccBadRequest;;
    return eP->render(ciP, EntitiesRequest);
  }


  if (!document.IsObject())
  {
    LM_E(("Bad Input (JSON Parse Error)"));
    eP->errorCode.fill("ParseError", "Error parsing incoming JSON buffer");
    ciP->httpStatusCode = SccBadRequest;;
    return eP->render(ciP, EntitiesRequest);
  }


  if (eidInURL == false)
  {
    if (!document.HasMember("id"))
    {
      LM_W(("Bad Input (No entity id specified"));
      eP->errorCode.fill("BadRequest", "no entity id specified");
      ciP->httpStatusCode = SccBadRequest;;
      return eP->render(ciP, EntitiesRequest);
    }
  }


  if (eidInURL == true)
  {
    if (document.HasMember("id"))
    {
      LM_W(("Bad Input (entity id specified in payload"));
      eP->errorCode.fill("BadRequest", "entity id specified in payload");
      ciP->httpStatusCode = SccBadRequest;;
      return eP->render(ciP, EntitiesRequest);
    }

    if (document.HasMember("type"))
    {
      LM_W(("Bad Input (entity type specified in payload"));
      eP->errorCode.fill("BadRequest", "entity type specified in payload");
      ciP->httpStatusCode = SccBadRequest;;
      return eP->render(ciP, EntitiesRequest);
    }
  }


  for (Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name   = iter->name.GetString();
    std::string type   = jsonParseTypeNames[iter->value.GetType()];

    if (name == "id")
    {
      if (eidInURL == false)
      {
        if (type != "String")
        {
          LM_W(("Bad Input (invalid JSON type for entity id"));
          eP->errorCode.fill("BadRequest", "invalid JSON type for entity id");
          ciP->httpStatusCode = SccBadRequest;;
          return eP->render(ciP, EntitiesRequest);
        }

        eP->id = iter->value.GetString();
      }
      else  // "id" present in payload for /v2/entities/<eid> - not a valid payload
      {
        LM_W(("Bad Input ('id' is not a valid attribute"));
        eP->errorCode.fill("BadRequest", "invalid input, 'id' as attribute");
        ciP->httpStatusCode = SccBadRequest;;
        return eP->render(ciP, EntitiesRequest);
      }
    }
    else if (name == "type")
    {
      if (type != "String")
      {
        LM_W(("Bad Input (invalid JSON type for entity type"));
        eP->errorCode.fill("BadRequest", "invalid JSON type for entity type");
        ciP->httpStatusCode = SccBadRequest;;
        return eP->render(ciP, EntitiesRequest);
      }

      eP->type      = iter->value.GetString();
      eP->typeGiven = true;
    }
    else
    {
      std::string        r;
      ContextAttribute*  caP = new ContextAttribute();

      eP->attributeVector.push_back(caP);

      if (ciP->uriParamOptions["keyValues"] == false)
      {
        r = parseContextAttributeObject(iter->value, caP);
      }
      else
      {
        r = parseContextAttribute(ciP, iter, caP);
      }

      if (r != "OK")
      {
        LM_W(("Bad Input (parse error in context attribute)"));
        eP->errorCode.fill("BadRequest", r);
        ciP->httpStatusCode = SccBadRequest;
        return eP->render(ciP, EntitiesRequest);
      }
    }
  }

  if (eidInURL == false)
  {
    if (eP->id == "")
    {
      LM_W(("Bad Input (empty entity id"));
      eP->errorCode.fill("BadRequest", "empty entity id");
      ciP->httpStatusCode = SccBadRequest;;
      return eP->render(ciP, EntitiesRequest);
    }
  }

  eP->present("");
  return "OK";
}
