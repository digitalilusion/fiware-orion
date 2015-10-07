/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan
*/

#include "Notifier.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/string.h"
#include "ngsi10/NotifyContextRequest.h"

#include "onTimeIntervalThread.h"
#include "senderThread.h"
#include "rest/httpRequestSend.h"


/* ****************************************************************************
*
* Select 'method' to send notifications - only one can be uncommented
*/
//#define SEND_BLOCKING
#define SEND_IN_NEW_THREAD



/* ****************************************************************************
*
* ~Notifier -
*/
Notifier::~Notifier (void)
{
  // FIXME: This destructor is needed to avoid warning message.
  // Compilation fails when a warning occurs, and it is enabled 
  // compilation option -Werror "warnings being treated as errors" 
  LM_T(LmtNotImplemented, ("Notifier destructor is not implemented"));
}

/* ****************************************************************************
*
* Notifier::sendNotifyContextRequest -
*/
void Notifier::sendNotifyContextRequest(NotifyContextRequest* ncr, const std::string& url, const std::string& tenant, const std::string& xauthToken, Format format)
{
    ConnectionInfo ci;

    LM_M(("Repsol: sending notification: %s", url.c_str()));

    //
    // Creating the value of the Fiware-ServicePath HTTP header.
    // This is a comma-separated list of the service-paths in the same order as the entities come in the payload
    //
    std::string spathList;
    bool atLeastOneNotDefault = false;
    for (unsigned int ix = 0; ix < ncr->contextElementResponseVector.size(); ++ix)
    {
      EntityId* eP = &ncr->contextElementResponseVector[ix]->contextElement.entityId;

      if (spathList != "")
      {
        spathList += ",";
      }
      spathList += eP->servicePath;
      atLeastOneNotDefault = atLeastOneNotDefault || (eP->servicePath != "/");
    }
    // FIXME P8: the stuff about atLeastOneNotDefault was added after PR #729, which makes "/" the default servicePath in
    // request not having that header. However, this causes as side-effect that a
    // "Fiware-ServicePath: /" or "Fiware-ServicePath: /,/" header is added in notifications, thus breaking several tests harness.
    // Given that the "clean" implementation of Fiware-ServicePath propagation will be implemented
    // soon (it has been scheduled for version 0.19.0, see https://github.com/telefonicaid/fiware-orion/issues/714)
    // we introduce the atLeastOneNotDefault hack. Once #714 gets implemented,
    // this FIXME will be removed (and all the test harness adjusted, if needed)
    if (!atLeastOneNotDefault)
    {
      spathList = "";
    }
    
    ci.outFormat = format;
    std::string payload = ncr->render(&ci, NotifyContext, "");

    /* Parse URL */
    std::string  host;
    int          port;
    std::string  uriPath;
    std::string  protocol;
    
    if (!parseUrl(url, host, port, uriPath, protocol))
    {
      LM_W(("Bad Input (sending NotifyContextRequest: malformed URL: '%s')", url.c_str()));
      return;
    }

    /* Set Content-Type depending on the format */
    std::string content_type = (format == XML)? "application/xml" : "application/json";

#ifdef SEND_BLOCKING
    httpRequestSend(host,
                    port,
                    protocol,
                    "POST",
                    tenant,
                    spathList,
                    xauthToken,
                    uriPath,
                    content_type,
                    payload,
                    true,
                    NOTIFICATION_WAIT_MODE);
#endif

#ifdef SEND_IN_NEW_THREAD
    /* Send the message (no wait for response), in a separate thread to avoid blocking */
    pthread_t tid;
    SenderThreadParams* params = new SenderThreadParams();
    params->ip            = host;
    params->port          = port;
    params->protocol      = protocol;
    params->verb          = "POST";
    params->tenant        = tenant;
    params->servicePath   = spathList;
    params->xauthToken    = xauthToken;
    params->resource      = uriPath;
    params->content_type  = content_type;
    params->content       = payload;
    strncpy(params->transactionId, transactionId, sizeof(params->transactionId));

    int ret = pthread_create(&tid, NULL, startSenderThread, params);
    if (ret != 0)
    {
      LM_E(("Runtime Error (error creating thread: %d)", ret));
      return;
    }
    pthread_detach(tid);
#endif
}


/* ****************************************************************************
*
* Notifier::sendNotifyContextAvailabilityRequest -
*
* FIXME: this method is very similar to sendNotifyContextRequest and probably
* they could be refactored in the future to have a common part using a parent
* class for both types of notifications and using it as first argument
*/
void Notifier::sendNotifyContextAvailabilityRequest(NotifyContextAvailabilityRequest* ncar, const std::string& url, const std::string& tenant, Format format) {

    /* Render NotifyContextAvailabilityRequest */
    std::string payload = ncar->render(NotifyContextAvailability, format, "");

    /* Parse URL */
    std::string  host;
    int          port;
    std::string  uriPath;
    std::string  protocol;

    if (!parseUrl(url, host, port, uriPath, protocol))
    {
      LM_W(("Bad Input (sending NotifyContextAvailabilityRequest: malformed URL: '%s')", url.c_str()));
      return;
    }

    /* Set Content-Type depending on the format */
    std::string content_type = (format == XML ? "application/xml" : "application/json");

    /* Send the message (no wait for response, in a separated thread to avoid blocking response)*/
#ifdef SEND_BLOCKING
    httpRequestSend(host, port, protocol, "POST", tenant, "", "", uriPath, content_type, payload, true, NOTIFICATION_WAIT_MODE);
#endif

#ifdef SEND_IN_NEW_THREAD
    pthread_t tid;
    SenderThreadParams* params = new SenderThreadParams();

    params->ip           = host;
    params->port         = port;
    params->verb         = "POST";
    params->tenant       = tenant;
    params->resource     = uriPath;   
    params->content_type = content_type;
    params->content      = payload;
    strncpy(params->transactionId, transactionId, sizeof(params->transactionId));

    int ret = pthread_create(&tid, NULL, startSenderThread, params);
    if (ret != 0)
    {
      LM_E(("Runtime Error (error creating thread: %d)", ret));
      return;
    }
    pthread_detach(tid);
#endif
}

/* ****************************************************************************
*
* Notifier::createIntervalThread -
*/
void Notifier::createIntervalThread(const std::string& subId, int interval, const std::string& tenant) {

    /* Create params dynamically. Note that the first action that thread does
     * if */
    ThreadData td;
    td.params = new OnIntervalThreadParams();
    td.params->tenant = tenant;
    td.params->subId = subId;
    td.params->interval = interval;
    td.params->notifier = this;

    int ret = pthread_create(&(td.tid), NULL, startOnIntervalThread, td.params);
    if (ret != 0)
    {
      LM_E(("Runtime Error (error creating thread: %d)", ret));
      return;
    }

    pthread_detach(td.tid);

    /* Put the id in the list associate to subId */
    LM_T(LmtNotifier, ("created thread: %lu", (unsigned long) td.tid));
    this->threadsMap.insert(std::pair<std::string,ThreadData>(subId,td));
}

/* ****************************************************************************
*
* Notifier::destroyOntimeIntervalThreads -
*/
void Notifier::destroyOntimeIntervalThreads(const std::string& subId) {

    std::vector<pthread_t> canceled;

    /* Get all the ThreadParams associated to the given subId. Inspired in
     * http://advancedcppwithexamples.blogspot.com.es/2009/04/example-of-c-multimap.html
     */
    std::pair<std::multimap<std::string, ThreadData>::iterator, std::multimap<std::string, ThreadData>::iterator> ii;
    std::multimap<std::string, ThreadData>::iterator it;
    ii = this->threadsMap.equal_range(subId);
    for (it = ii.first; it != ii.second; ++it) {

        ThreadData td = it->second;

        /* Destroy thread */        
        int ret = pthread_cancel(td.tid);
        if (ret != 0)
        {
          LM_E(("Runtime Error (error canceling thread %lu: %d)", (unsigned long) td.tid, ret));
          return;
        }

        /* Note that we do the cancelation in parallel, storing the thread ID. This
         * vector is processed afterwards to wait for every thread to finish */
        canceled.push_back(td.tid);

        /* Release memory */
        delete td.params;

    }

    /* Remove key from the hashmap */
    threadsMap.erase(subId);

    /* Wait for all the cancelation to end */
    for (unsigned int ix = 0; ix < canceled.size(); ++ix) {
        void* res;

        /* pthread_join in blocking */
        int ret = pthread_join(canceled[ix], &res);
        if (ret != 0)
        {
          LM_E(("Runtime Error (error joining thread %lu: %d)", (unsigned long) canceled[ix], ret));
          return;
        }

        if (res == PTHREAD_CANCELED)
        {
            LM_T(LmtNotifier, ("canceled thread: %lu", (unsigned long) canceled[ix]));
        }
        else
        {
          LM_E(("Runtime Error (unexpected error: thread can not be canceled)"));
          return;
        }
    }

    canceled.clear();
}
