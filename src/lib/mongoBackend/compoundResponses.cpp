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
* Author: Fermín Galán
*/

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "mongoBackend/compoundResponses.h"

using namespace mongo;

/* ****************************************************************************
*
* addCompoundNode -
*
*/
static void addCompoundNode(orion::CompoundValueNode* cvP, const BSONElement& e)
{
  if ((e.type() != String) && (e.type() != Bool) && (e.type() != NumberDouble) && (e.type() != Object) && (e.type() != Array))
  {
    LM_T(LmtSoftError, ("unknown BSON type"));
    return;
  }

  orion::CompoundValueNode* child = new orion::CompoundValueNode(orion::ValueTypeObject);
  child->name = e.fieldName();

  switch (e.type())
  {
  case String:
    child->valueType  = orion::ValueTypeString;
    child->stringValue = e.String();
    break;

  case Bool:
    child->valueType  = orion::ValueTypeBoolean;
    child->boolValue = e.Bool();
    break;

  case NumberDouble:
    child->valueType  = orion::ValueTypeNumber;
    child->numberValue = e.Number();
    break;

  case Object:
    compoundObjectResponse(child, e);
    break;

  case Array:
    compoundVectorResponse(child, e);
    break;

  default:
    //
    // We need the default clause to avoid 'enumeration value X not handled in switch' errors
    // due to -Werror=switch at compilation time
    //
    break;
  }

  cvP->add(child);
}


/* ****************************************************************************
*
* compoundObjectResponse -
*/
void compoundObjectResponse(orion::CompoundValueNode* cvP, const BSONElement& be)
{
  BSONObj obj = be.embeddedObject();

  cvP->valueType = orion::ValueTypeObject;
  for (BSONObj::iterator i = obj.begin(); i.more();)
  {
    BSONElement e = i.next();
    addCompoundNode(cvP, e);
  }
}


/* ****************************************************************************
*
* compoundVectorResponse -
*/
void compoundVectorResponse(orion::CompoundValueNode* cvP, const BSONElement& be)
{
  std::vector<BSONElement> vec = be.Array();

  cvP->valueType = orion::ValueTypeVector;
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    BSONElement e = vec[ix];
    addCompoundNode(cvP, e);
  }
}
