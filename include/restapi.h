// Copyright (c) 2022 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef RESTAPI_H
#define RESTAPI_H

#include <Arduino.h>
#include <WebServer.h>

// TODO MEJ WebServer could be forward declared as well
namespace inverters
{
class Inverter;
}

// TODO MEJ Pass WebServer by reference

void buildJsonRest(inverters::Inverter &inverter, WebServer * server);
void handle_setParameter(WebServer * server);

#endif