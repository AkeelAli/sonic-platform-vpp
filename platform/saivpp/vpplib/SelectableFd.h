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

#include "swss/sal.h"
#include "swss/selectable.h"

namespace saivpp
{
    class SelectableFd:
        public swss::Selectable
    {
        private:

            SelectableFd(const SelectableFd&) = delete;
            SelectableFd& operator=(const SelectableFd&) = delete;

        public:

            SelectableFd(
                    _In_ int fd);

            virtual ~SelectableFd() = default;

            int getFd() override;

            uint64_t readData() override;

        private:

            int m_fd;
    };
}
