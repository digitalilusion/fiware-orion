# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET (SOURCES
    MongoGlobal.cpp
    MongoCommonUpdate.cpp
    MongoCommonRegister.cpp
    mongoRegisterContext.cpp
    mongoDiscoverContextAvailability.cpp
    mongoSubscribeContextAvailability.cpp
    mongoUnsubscribeContextAvailability.cpp
    mongoUpdateContextAvailabilitySubscription.cpp
    mongoUpdateContext.cpp
    mongoQueryContext.cpp
    mongoSubscribeContext.cpp
    mongoUnsubscribeContext.cpp
    mongoUpdateContextSubscription.cpp
    mongoOntimeintervalOperations.cpp    
    mongoNotifyContext.cpp
    mongoNotifyContextAvailability.cpp
    mongoQueryTypes.cpp
    TriggeredSubscription.cpp
    mongoConnectionPool.cpp
    mongoGetSubscriptions.cpp
    connectionOperations.cpp
    mongoSubCache.cpp
    safeMongo.cpp
    dbFieldEncoding.cpp
    compoundResponses.cpp
)

SET (HEADERS
    dbConstants.h
    MongoGlobal.h
    MongoCommonUpdate.h
    MongoCommonRegister.h
    mongoRegisterContext.h
    mongoDiscoverContextAvailability.h
    mongoSubscribeContextAvailability.h
    mongoUnsubscribeContextAvailability.h
    mongoUpdateContextAvailabilitySubscription.h
    mongoUpdateContext.h
    mongoQueryContext.h
    mongoSubscribeContext.h
    mongoUnsubscribeContext.h
    mongoUpdateContextSubscription.h
    mongoOntimeintervalOperations.h    
    mongoNotifyContext.h
    mongoNotifyContextAvailability.h
    mongoQueryTypes.h
    TriggeredSubscription.h
    mongoConnectionPool.h
    mongoGetSubscriptions.h
    connectionOperations.h
    mongoSubCache.h
    safeMongo.h
    dbFieldEncoding.h
    compoundResponses.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(mongoBackend STATIC ${SOURCES} ${HEADERS})
