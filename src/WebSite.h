/*
 * WebSite.h
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#ifndef WEBSITE_H_
#define WEBSITE_H_

#include <Arduino.h>

#include "Application.h"


class WebSite : public ApplicationNS::Task
{
public:
    WebSite(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize);
    virtual ~WebSite();

    /* ApplicationNS::Task::Init */
    void Init(ApplicationNS::tTaskObjects* apTaskObjects) override;

private:

    /* ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;
};

#endif /* WEBSITE_H_ */