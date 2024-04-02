// Copyright (c) 2022 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef RESTAPI_H
#define RESTAPI_H

#include <WebServer.h>

// forward declarations
class WebServer;

namespace inverter
{
class DataAdapter;
}

void buildJsonRest(const inverter::DataAdapter& dataAdapter, WebServer& server);
void handle_setParameter(WebServer& server);

#endif