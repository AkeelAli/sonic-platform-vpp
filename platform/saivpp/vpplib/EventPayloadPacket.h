/*
 * Copyright 2016 Microsoft, Inc.
 * Modifications copyright (c) 2023 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

extern "C" {
#include "sai.h"
}

#include "EventPayload.h"
#include "Buffer.h"

#include <string>

namespace saivpp
{
    class EventPayloadPacket:
        public EventPayload
    {
        public:

            EventPayloadPacket(
                    _In_ sai_object_id_t port,
                    _In_ int ifIndex,
                    _In_ const std::string& ifName,
                    _In_ const Buffer& buffer);

            virtual ~EventPayloadPacket() = default;

        public:

            sai_object_id_t getPort() const;

            int getIfIndex() const;

            const std::string& getIfName() const;

            const Buffer& getBuffer() const;

        private:

            sai_object_id_t m_port;

            int m_ifIndex;

            std::string m_ifName;

            Buffer m_buffer;
    };
}
